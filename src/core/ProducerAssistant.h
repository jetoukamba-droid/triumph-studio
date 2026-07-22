#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

namespace triumph::producer
{
enum class Scale
{
    major,
    minor
};

enum class Style
{
    balanced,
    chill,
    energetic
};

struct Request
{
    int rootPitchClass = 0;
    Scale scale = Scale::major;
    Style style = Style::balanced;
    int bars = 4;
    std::uint32_t variation = 1;
};

struct Note
{
    double startBeat = 0.0;
    double lengthBeats = 1.0;
    int noteNumber = 60;
    float velocity = 0.8f;
    int channel = 1;
};

struct TrackDescriptor
{
    std::string name;
    bool instrument = false;
};

struct MixMove
{
    std::size_t trackIndex = 0;
    float gain = 0.75f;
    float pan = 0.0f;
    std::string reason;
};

inline int clampedRoot (int value) noexcept
{
    value %= 12;
    return value < 0 ? value + 12 : value;
}

inline int clampedBars (int value) noexcept
{
    return std::clamp (value, 1, 16);
}

inline const std::array<int, 7>& scaleIntervals (Scale scale) noexcept
{
    static constexpr std::array<int, 7> major { 0, 2, 4, 5, 7, 9, 11 };
    static constexpr std::array<int, 7> minor { 0, 2, 3, 5, 7, 8, 10 };
    return scale == Scale::minor ? minor : major;
}

inline std::array<int, 4> chordDegrees (Scale scale, Style style) noexcept
{
    if (scale == Scale::minor)
    {
        if (style == Style::chill) return { 0, 6, 5, 6 };
        if (style == Style::energetic) return { 0, 5, 6, 6 };
        return { 0, 5, 2, 6 };
    }
    if (style == Style::chill) return { 0, 5, 3, 4 };
    if (style == Style::energetic) return { 0, 3, 4, 4 };
    return { 0, 4, 5, 3 };
}

inline int pitchForDegree (int basePitch, int degree,
                           const std::array<int, 7>& intervals) noexcept
{
    const auto octave = degree / 7;
    const auto wrapped = degree % 7;
    return basePitch + intervals[static_cast<std::size_t> (wrapped)] + octave * 12;
}

inline std::vector<Note> generateChords (const Request& request)
{
    const auto bars = clampedBars (request.bars);
    const auto& intervals = scaleIntervals (request.scale);
    const auto progression = chordDegrees (request.scale, request.style);
    auto basePitch = 60 + clampedRoot (request.rootPitchClass);
    if (basePitch > 71) basePitch -= 12;
    const auto velocity = request.style == Style::energetic ? 0.82f
                        : request.style == Style::chill ? 0.64f : 0.73f;
    const auto duration = request.style == Style::energetic ? 3.75 : 3.9;
    std::vector<Note> notes;
    notes.reserve (static_cast<std::size_t> (bars * 4));
    for (int bar = 0; bar < bars; ++bar)
    {
        auto degree = progression[static_cast<std::size_t> (bar % 4)];
        if ((request.variation & 1u) != 0u && bar % 4 == 2)
            degree = (degree + 2) % 7;
        const auto start = static_cast<double> (bar * 4);
        const auto root = pitchForDegree (basePitch, degree, intervals);
        notes.push_back ({ start, duration, std::max (0, root - 12),
                           velocity * 0.88f, 1 });
        notes.push_back ({ start, duration, root, velocity, 1 });
        notes.push_back ({ start, duration,
                           pitchForDegree (basePitch, degree + 2, intervals),
                           velocity * 0.94f, 1 });
        notes.push_back ({ start, duration,
                           pitchForDegree (basePitch, degree + 4, intervals),
                           velocity * 0.90f, 1 });
    }
    return notes;
}

inline void addDrumHit (std::vector<Note>& notes, double beat,
                        int note, float velocity)
{
    notes.push_back ({ beat, 0.10, note, velocity, 10 });
}

inline std::vector<Note> generateDrums (const Request& request)
{
    const auto bars = clampedBars (request.bars);
    std::vector<Note> notes;
    notes.reserve (static_cast<std::size_t> (bars * 24));
    for (int bar = 0; bar < bars; ++bar)
    {
        const auto start = static_cast<double> (bar * 4);
        addDrumHit (notes, start + 1.0, 38, 0.86f);
        addDrumHit (notes, start + 3.0, 38, 0.91f);

        if (request.style == Style::chill)
        {
            addDrumHit (notes, start, 36, 0.88f);
            addDrumHit (notes, start + 2.0, 36, 0.72f);
        }
        else if (request.style == Style::energetic)
        {
            for (const auto beat : { 0.0, 1.5, 2.0, 3.5 })
                addDrumHit (notes, start + beat, 36,
                            beat == 0.0 ? 0.96f : 0.80f);
        }
        else
        {
            for (const auto beat : { 0.0, 2.0, 2.5 })
                addDrumHit (notes, start + beat, 36,
                            beat == 0.0 ? 0.92f : 0.76f);
        }

        const auto hatStep = request.style == Style::energetic ? 0.25 : 0.5;
        for (double beat = 0.0; beat < 4.0 - 1.0e-9; beat += hatStep)
        {
            const auto step = static_cast<int> (std::llround (beat / hatStep));
            addDrumHit (notes, start + beat, 42,
                        step % 4 == 0 ? 0.66f : 0.48f);
        }
        if ((request.variation + static_cast<std::uint32_t> (bar)) % 3u == 0u)
            addDrumHit (notes, start + 3.75, 36, 0.42f);
    }
    return notes;
}

inline std::uint32_t nextRandom (std::uint32_t& state) noexcept
{
    if (state == 0) state = 0x6d2b79f5u;
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

inline std::vector<Note> generateMelody (const Request& request)
{
    const auto bars = clampedBars (request.bars);
    const auto& intervals = scaleIntervals (request.scale);
    auto basePitch = 60 + clampedRoot (request.rootPitchClass);
    if (basePitch > 71) basePitch -= 12;
    const auto step = request.style == Style::chill ? 1.0 : 0.5;
    const auto duration = request.style == Style::energetic ? 0.42
                        : request.style == Style::chill ? 0.88 : 0.46;
    const auto velocity = request.style == Style::energetic ? 0.84f
                        : request.style == Style::chill ? 0.66f : 0.75f;
    auto random = request.variation ^ 0xa511e9b3u;
    auto degree = static_cast<int> (nextRandom (random) % 5u) + 1;
    const auto totalBeats = static_cast<double> (bars * 4);
    std::vector<Note> notes;
    for (double beat = 0.0; beat < totalBeats - 1.0e-9; beat += step)
    {
        const auto value = nextRandom (random);
        const auto shouldRest = request.style != Style::energetic
            && (value % 11u == 0u);
        const auto movement = static_cast<int> ((value >> 8) % 5u) - 2;
        degree = std::clamp (degree + movement, 0, 10);
        if (! shouldRest)
            notes.push_back ({ beat, duration,
                               pitchForDegree (basePitch, degree, intervals),
                               velocity * (beat == std::floor (beat) ? 1.0f : 0.88f),
                               1 });
    }
    return notes;
}

inline std::string lowercase (std::string value)
{
    for (auto& character : value)
        if (character >= 'A' && character <= 'Z')
            character = static_cast<char> (character - 'A' + 'a');
    return value;
}

inline bool containsAny (const std::string& value,
                         std::initializer_list<const char*> terms)
{
    return std::any_of (terms.begin(), terms.end(), [&value] (const auto* term)
    {
        return value.find (term) != std::string::npos;
    });
}

inline std::vector<MixMove> suggestMix (
    const std::vector<TrackDescriptor>& tracks)
{
    std::vector<MixMove> moves;
    if (tracks.empty()) return moves;
    const auto headroom = std::clamp (
        0.82f / std::sqrt (std::max (1.0f,
            static_cast<float> (tracks.size()) / 2.0f)), 0.46f, 0.82f);
    auto alternate = false;
    moves.reserve (tracks.size());
    for (std::size_t index = 0; index < tracks.size(); ++index)
    {
        const auto name = lowercase (tracks[index].name);
        MixMove move { index, headroom, 0.0f, "Headroom-balanced starting point" };
        if (containsAny (name, { "kick", "bass", "sub" }))
        {
            move.gain = std::min (0.82f, headroom + 0.08f);
            move.reason = "Low-frequency anchor kept centered";
        }
        else if (containsAny (name, { "vocal", "voice", "lead" }))
        {
            move.gain = std::min (0.86f, headroom + 0.10f);
            move.reason = "Lead element kept centered and forward";
        }
        else if (containsAny (name, { "drum", "snare", "perc" }))
        {
            move.gain = std::min (0.80f, headroom + 0.05f);
            move.reason = "Rhythm anchor kept centered";
        }
        else
        {
            alternate = ! alternate;
            const auto width = containsAny (name, { "pad", "guitar", "keys", "synth" })
                ? 0.28f : 0.16f;
            move.pan = alternate ? -width : width;
            move.gain = std::max (0.42f,
                                  headroom - (tracks[index].instrument ? 0.02f : 0.0f));
            move.reason = "Supporting element spread for separation";
        }
        moves.push_back (std::move (move));
    }
    return moves;
}
}

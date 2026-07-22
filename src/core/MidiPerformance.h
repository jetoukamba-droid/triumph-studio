#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace triumph::midi
{
enum class Protocol
{
    midi1,
    midi2Ump
};

struct UmpPacket
{
    std::array<std::uint32_t, 4> words {};
    std::uint8_t wordCount = 0;
};

inline UmpPacket midi2NoteOn (std::uint8_t group, std::uint8_t channel,
                              std::uint8_t note, std::uint16_t velocity,
                              std::uint16_t attributeType = 0,
                              std::uint16_t attributeData = 0) noexcept
{
    group &= 0x0f; channel &= 0x0f; note &= 0x7f;
    return { { static_cast<std::uint32_t> (0x4u << 28)
                    | (static_cast<std::uint32_t> (group) << 24)
                    | (static_cast<std::uint32_t> (0x90u | channel) << 16)
                    | (static_cast<std::uint32_t> (note) << 8)
                    | (attributeType & 0xffu),
               (static_cast<std::uint32_t> (velocity) << 16)
                    | attributeData, 0, 0 }, 2 };
}

inline UmpPacket midi2NoteOff (std::uint8_t group, std::uint8_t channel,
                               std::uint8_t note, std::uint16_t velocity = 0) noexcept
{
    group &= 0x0f; channel &= 0x0f; note &= 0x7f;
    return { { static_cast<std::uint32_t> (0x4u << 28)
                    | (static_cast<std::uint32_t> (group) << 24)
                    | (static_cast<std::uint32_t> (0x80u | channel) << 16)
                    | (static_cast<std::uint32_t> (note) << 8),
               static_cast<std::uint32_t> (velocity) << 16, 0, 0 }, 2 };
}

inline UmpPacket midi2PerNotePitch (std::uint8_t group,
                                    std::uint8_t channel,
                                    std::uint8_t note,
                                    std::uint32_t value) noexcept
{
    group &= 0x0f; channel &= 0x0f; note &= 0x7f;
    return { { static_cast<std::uint32_t> (0x4u << 28)
                    | (static_cast<std::uint32_t> (group) << 24)
                    | (static_cast<std::uint32_t> (0x60u | channel) << 16)
                    | (static_cast<std::uint32_t> (note) << 8),
               value, 0, 0 }, 2 };
}

struct TimedEvent
{
    std::int64_t timelineSample = 0;
    std::uint32_t sampleOffset = 0;
    UmpPacket packet;
};

inline std::vector<TimedEvent> eventsForBlock (
    std::int64_t blockStartSample, std::uint32_t blockSize,
    const std::vector<TimedEvent>& source)
{
    std::vector<TimedEvent> result;
    if (blockSize == 0)
        return result;
    const auto blockEnd = blockStartSample
        + static_cast<std::int64_t> (blockSize);
    for (auto event : source)
    {
        if (event.timelineSample < blockStartSample
            || event.timelineSample >= blockEnd
            || event.packet.wordCount == 0)
            continue;
        event.sampleOffset = static_cast<std::uint32_t> (
            event.timelineSample - blockStartSample);
        result.push_back (event);
    }
    std::stable_sort (result.begin(), result.end(), [] (const auto& a,
                                                        const auto& b)
    {
        return a.sampleOffset < b.sampleOffset;
    });
    return result;
}

class MpeZoneAllocator final
{
public:
    explicit MpeZoneAllocator (std::uint8_t firstMemberChannel = 2,
                               std::uint8_t lastMemberChannel = 16)
        : first (std::clamp<std::uint8_t> (firstMemberChannel, 2, 16)),
          last (std::clamp<std::uint8_t> (lastMemberChannel, first, 16))
    {
    }

    std::optional<std::uint8_t> allocate (std::uint32_t noteId) noexcept
    {
        if (const auto existing = channelFor (noteId))
            return existing;
        for (auto channel = first; channel <= last; ++channel)
            if (owners[channel] == 0)
            {
                owners[channel] = noteId == 0 ? 1 : noteId;
                return channel;
            }
        return std::nullopt;
    }

    void release (std::uint32_t noteId) noexcept
    {
        const auto id = noteId == 0 ? 1 : noteId;
        for (auto channel = first; channel <= last; ++channel)
            if (owners[channel] == id)
                owners[channel] = 0;
    }

    std::optional<std::uint8_t> channelFor (
        std::uint32_t noteId) const noexcept
    {
        const auto id = noteId == 0 ? 1 : noteId;
        for (auto channel = first; channel <= last; ++channel)
            if (owners[channel] == id)
                return channel;
        return std::nullopt;
    }

private:
    std::array<std::uint32_t, 17> owners {};
    std::uint8_t first = 2;
    std::uint8_t last = 16;
};

enum class SyncSource
{
    internal,
    midiClock,
    midiTimeCode,
    link
};

struct SyncStatus
{
    SyncSource source = SyncSource::internal;
    bool locked = false;
    double tempoBpm = 120.0;
    double jitterSamplesRms = 0.0;
    std::uint64_t receivedPulses = 0;
    double timecodeSeconds = 0.0;
    double timecodeFramesPerSecond = 30.0;
};

class SyncFollower final
{
public:
    explicit SyncFollower (double sampleRate = 48000.0) noexcept
        : rate (std::max (1.0, sampleRate))
    {
    }

    void setSource (SyncSource newSource) noexcept
    {
        source = newSource;
        reset();
    }

    void setSampleRate (double sampleRate) noexcept
    {
        rate = std::max (1.0, sampleRate);
        reset();
    }

    bool observeMidiClock (std::int64_t samplePosition) noexcept
    {
        if (source != SyncSource::midiClock || samplePosition < 0)
            return false;
        if (lastPulse >= 0 && samplePosition > lastPulse)
        {
            const auto interval = static_cast<double> (
                samplePosition - lastPulse);
            const auto instantTempo = 60.0 * rate / (interval * 24.0);
            if (instantTempo >= 20.0 && instantTempo <= 400.0)
            {
                tempo = pulses == 0 ? instantTempo
                                    : tempo * 0.9 + instantTempo * 0.1;
                const auto expected = 60.0 * rate / (tempo * 24.0);
                const auto error = interval - expected;
                squaredError = squaredError * 0.9 + error * error * 0.1;
                locked = pulses >= 5;
            }
        }
        lastPulse = samplePosition;
        ++pulses;
        return true;
    }

    bool observeMtcQuarterFrame (std::uint8_t messageType,
                                 std::uint8_t nibble,
                                 std::int64_t samplePosition) noexcept
    {
        if (source != SyncSource::midiTimeCode || messageType > 7)
            return false;
        mtcNibbles[messageType] = nibble & 0x0f;
        mtcMask |= static_cast<std::uint8_t> (1u << messageType);
        if (mtcMask == 0xff)
        {
            const auto frames = static_cast<int> (mtcNibbles[0])
                | (static_cast<int> (mtcNibbles[1] & 0x01) << 4);
            const auto seconds = static_cast<int> (mtcNibbles[2])
                | (static_cast<int> (mtcNibbles[3] & 0x03) << 4);
            const auto minutes = static_cast<int> (mtcNibbles[4])
                | (static_cast<int> (mtcNibbles[5] & 0x03) << 4);
            const auto hours = static_cast<int> (mtcNibbles[6])
                | (static_cast<int> (mtcNibbles[7] & 0x01) << 4);
            const auto rateCode = (mtcNibbles[7] >> 1) & 0x03;
            timecodeFps = rateCode == 0 ? 24.0
                : rateCode == 1 ? 25.0
                : rateCode == 2 ? 29.97 : 30.0;
            timecode = static_cast<double> (hours * 3600
                                             + minutes * 60 + seconds)
                + static_cast<double> (frames) / timecodeFps;
            lastPulse = samplePosition;
            ++pulses;
            locked = true;
            mtcMask = 0;
            return true;
        }
        return false;
    }

    void poll (std::int64_t currentSample) noexcept
    {
        if (source == SyncSource::internal)
        {
            locked = true;
            return;
        }
        const auto timeout = source == SyncSource::midiClock
            ? static_cast<std::int64_t> (rate * 0.5)
            : static_cast<std::int64_t> (rate * 1.0);
        if (lastPulse < 0 || currentSample - lastPulse > timeout)
            locked = false;
    }

    SyncStatus status() const noexcept
    {
        return { source, locked, tempo, std::sqrt (squaredError), pulses,
                 timecode, timecodeFps };
    }

private:
    void reset() noexcept
    {
        locked = source == SyncSource::internal;
        tempo = 120.0;
        squaredError = 0.0;
        pulses = 0;
        lastPulse = -1;
        mtcMask = 0;
        mtcNibbles.fill (0);
        timecode = 0.0;
        timecodeFps = 30.0;
    }

    SyncSource source = SyncSource::internal;
    double rate = 48000.0;
    double tempo = 120.0;
    double squaredError = 0.0;
    std::uint64_t pulses = 0;
    std::int64_t lastPulse = -1;
    bool locked = true;
    std::array<std::uint8_t, 8> mtcNibbles {};
    std::uint8_t mtcMask = 0;
    double timecode = 0.0;
    double timecodeFps = 30.0;
};

template <std::size_t Capacity>
class SampleOffsetParameterQueue final
{
public:
    struct Event
    {
        std::uint32_t parameterIndex = 0;
        std::uint32_t sampleOffset = 0;
        float value = 0.0f;
    };

    bool push (Event event, std::uint32_t blockSize) noexcept
    {
        if (sizeValue >= Capacity || blockSize == 0)
            return false;
        event.sampleOffset = std::min (event.sampleOffset, blockSize - 1);
        event.value = std::clamp (event.value, 0.0f, 1.0f);
        auto position = sizeValue;
        while (position > 0
               && events[position - 1].sampleOffset > event.sampleOffset)
        {
            events[position] = events[position - 1];
            --position;
        }
        events[position] = event;
        ++sizeValue;
        return true;
    }

    void clear() noexcept { sizeValue = 0; }
    std::size_t size() const noexcept { return sizeValue; }
    const Event& operator[] (std::size_t index) const noexcept
    {
        return events[index];
    }

private:
    std::array<Event, Capacity> events {};
    std::size_t sizeValue = 0;
};
}

#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <mutex>
#include <vector>

namespace triumph
{
class MidiCapture final
{
public:
    struct Note
    {
        double startSeconds = 0.0;
        double lengthSeconds = 0.0;
        int noteNumber = 60;
        float velocity = 0.8f;
        int channel = 1;
    };

    void start (double originSeconds)
    {
        std::scoped_lock lock (mutex);
        origin = originSeconds;
        notes.clear();
        for (auto& note : active)
            note = {};
        noteOnCount.store (0);
        recording.store (true);
    }

    void processNoteOn (int channel, int noteNumber, float velocity,
                        double eventSeconds)
    {
        channel = std::clamp (channel, 1, 16);
        noteNumber = std::clamp (noteNumber, 0, 127);
        std::scoped_lock lock (mutex);
        if (! recording.load())
            return;
        const auto relative = std::max (0.0, eventSeconds - origin);
        auto& state = active[static_cast<size_t> ((channel - 1) * 128 + noteNumber)];
        if (state.held)
            closeNote (state, channel, noteNumber, relative);
        state = { true, relative, std::clamp (velocity, 0.01f, 1.0f) };
        noteOnCount.fetch_add (1);
    }

    void processNoteOff (int channel, int noteNumber, double eventSeconds)
    {
        channel = std::clamp (channel, 1, 16);
        noteNumber = std::clamp (noteNumber, 0, 127);
        std::scoped_lock lock (mutex);
        if (! recording.load())
            return;
        const auto relative = std::max (0.0, eventSeconds - origin);
        auto& state = active[static_cast<size_t> ((channel - 1) * 128 + noteNumber)];
        if (state.held)
            closeNote (state, channel, noteNumber, relative);
    }

    std::vector<Note> stop (double eventSeconds)
    {
        std::scoped_lock lock (mutex);
        if (! recording.exchange (false))
            return {};
        const auto relative = std::max (0.0, eventSeconds - origin);
        for (int channel = 1; channel <= 16; ++channel)
            for (int note = 0; note < 128; ++note)
            {
                auto& state = active[static_cast<size_t> ((channel - 1) * 128 + note)];
                if (state.held)
                    closeNote (state, channel, note, relative);
            }
        std::stable_sort (notes.begin(), notes.end(), [] (const auto& a, const auto& b)
        {
            return a.startSeconds < b.startSeconds;
        });
        return notes;
    }

    bool isRecording() const noexcept { return recording.load(); }
    int getNoteOnCount() const noexcept { return noteOnCount.load(); }

private:
    struct ActiveNote
    {
        bool held = false;
        double startSeconds = 0.0;
        float velocity = 0.0f;
    };

    void closeNote (ActiveNote& state, int channel, int noteNumber, double endSeconds)
    {
        notes.push_back ({ state.startSeconds,
                           std::max (1.0 / 1000.0, endSeconds - state.startSeconds),
                           noteNumber,
                           state.velocity,
                           channel });
        state = {};
    }

    std::array<ActiveNote, 16 * 128> active {};
    std::vector<Note> notes;
    std::mutex mutex;
    std::atomic<bool> recording { false };
    std::atomic<int> noteOnCount { 0 };
    double origin = 0.0;
};
}

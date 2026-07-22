#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace triumph::shortcuts
{
enum class Command
{
    none,
    togglePlayback,
    stop,
    rewind,
    toggleRecord,
    toggleMonitor,
    toggleMixer,
    toggleTempoAutomation,
    toggleSnap,
    zoomIn,
    zoomOut,
    newProject,
    openProject,
    saveProject,
    saveProjectAs,
    undo,
    redo,
    importAudio,
    exportMix,
    addAudioTrack,
    addInstrumentTrack,
    splitClip
};

enum Modifier : std::uint8_t
{
    noModifier = 0,
    commandModifier = 1 << 0,
    shiftModifier = 1 << 1,
    altModifier = 1 << 2
};

struct Binding
{
    Command command = Command::none;
    int keyCode = 0;
    std::uint8_t modifiers = noModifier;
    const char* label = "";
};

inline constexpr std::array<Binding, 25> standardBindings {{
    { Command::togglePlayback, ' ', noModifier, "Space" },
    { Command::stop, '.', noModifier, "." },
    { Command::rewind, 'W', noModifier, "W" },
    { Command::toggleRecord, 'R', noModifier, "R" },
    { Command::toggleMonitor, 'M', noModifier, "M" },
    { Command::toggleMixer, 'B', noModifier, "B" },
    { Command::toggleTempoAutomation, 'A', noModifier, "A" },
    { Command::toggleSnap, 'S', noModifier, "S" },
    { Command::zoomIn, '=', noModifier, "+" },
    { Command::zoomOut, '-', noModifier, "-" },
    { Command::newProject, 'N', commandModifier, "Ctrl+N" },
    { Command::openProject, 'O', commandModifier, "Ctrl+O" },
    { Command::saveProject, 'S', commandModifier, "Ctrl+S" },
    { Command::saveProjectAs, 'S',
      static_cast<std::uint8_t> (commandModifier | shiftModifier),
      "Ctrl+Shift+S" },
    { Command::undo, 'Z', commandModifier, "Ctrl+Z" },
    { Command::redo, 'Y', commandModifier, "Ctrl+Y" },
    { Command::redo, 'Z',
      static_cast<std::uint8_t> (commandModifier | shiftModifier),
      "Ctrl+Shift+Z" },
    { Command::importAudio, 'I', commandModifier, "Ctrl+I" },
    { Command::exportMix, 'E',
      static_cast<std::uint8_t> (commandModifier | shiftModifier),
      "Ctrl+Shift+E" },
    { Command::addAudioTrack, 'T', commandModifier, "Ctrl+T" },
    { Command::addInstrumentTrack, 'T',
      static_cast<std::uint8_t> (commandModifier | shiftModifier),
      "Ctrl+Shift+T" },
    { Command::splitClip, 'E', commandModifier, "Ctrl+E" },
    { Command::zoomIn, '+', noModifier, "+" },
    { Command::zoomIn, '+', shiftModifier, "+" },
    { Command::zoomIn, '=', shiftModifier, "+" }
}};

constexpr int normaliseKeyCode (int keyCode) noexcept
{
    return keyCode >= 'a' && keyCode <= 'z' ? keyCode - ('a' - 'A')
                                             : keyCode;
}

constexpr Command commandFor (int keyCode, std::uint8_t modifiers) noexcept
{
    const auto normalised = normaliseKeyCode (keyCode);
    for (const auto& binding : standardBindings)
        if (binding.keyCode == normalised && binding.modifiers == modifiers)
            return binding.command;
    return Command::none;
}

constexpr bool hasUniqueGestures() noexcept
{
    for (std::size_t left = 0; left < standardBindings.size(); ++left)
        for (std::size_t right = left + 1; right < standardBindings.size(); ++right)
            if (standardBindings[left].keyCode == standardBindings[right].keyCode
                && standardBindings[left].modifiers
                       == standardBindings[right].modifiers)
                return false;
    return true;
}

constexpr const char* labelFor (Command command) noexcept
{
    for (const auto& binding : standardBindings)
        if (binding.command == command)
            return binding.label;
    return "";
}

static_assert (hasUniqueGestures());
}

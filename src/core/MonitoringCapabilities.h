#pragma once

#include <algorithm>
#include <cstdint>

namespace triumph::monitoring
{
enum class HardwareDirectControl : std::uint8_t
{
    unavailable,
    driverManaged
};

struct Capabilities
{
    int inputChannels = 0;
    int outputChannels = 0;
    bool softwareControlRoom = false;
    bool dedicatedCueOutput = false;
    HardwareDirectControl hardwareDirectControl =
        HardwareDirectControl::unavailable;
};

inline Capabilities assessDevice (int activeInputs,
                                  int activeOutputs) noexcept
{
    Capabilities result;
    result.inputChannels = std::max (0, activeInputs);
    result.outputChannels = std::max (0, activeOutputs);
    result.softwareControlRoom = result.inputChannels > 0
                                 && result.outputChannels >= 2;
    result.dedicatedCueOutput = result.outputChannels >= 4;
    result.hardwareDirectControl = result.inputChannels > 0
        ? HardwareDirectControl::driverManaged
        : HardwareDirectControl::unavailable;
    return result;
}
}

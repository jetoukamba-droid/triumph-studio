#pragma once

#include <algorithm>
#include <cmath>

namespace triumph
{
class ParameterSmoother final
{
public:
    void reset (double sampleRate, double timeMilliseconds, float initialValue) noexcept
    {
        current = target = initialValue;
        if (sampleRate <= 0.0 || timeMilliseconds <= 0.0)
            coefficient = 0.0f;
        else
            coefficient = static_cast<float> (std::exp (
                -1.0 / (sampleRate * timeMilliseconds * 0.001)));
    }

    void setTarget (float newTarget) noexcept
    {
        target = newTarget;
    }

    float process() noexcept
    {
        current = target + coefficient * (current - target);
        return current;
    }

    float getCurrent() const noexcept { return current; }

private:
    float current = 1.0f;
    float target = 1.0f;
    float coefficient = 0.0f;
};
}

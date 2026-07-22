#include "TestSupport.h"

#include "core/AdvancedAudioEdit.h"

#include <cmath>

namespace
{
double spectralMagnitude (const std::vector<float>& samples,
                          double frequency,
                          double sampleRate)
{
    constexpr double twoPi = 6.28318530717958647692;
    double real = 0.0;
    double imaginary = 0.0;
    const auto start = samples.size() / 4;
    const auto end = samples.size() * 3 / 4;
    for (std::size_t sample = start; sample < end; ++sample)
    {
        const auto phase = twoPi * frequency
                           * static_cast<double> (sample) / sampleRate;
        real += samples[sample] * std::cos (phase);
        imaginary -= samples[sample] * std::sin (phase);
    }
    return std::sqrt (real * real + imaginary * imaginary);
}
}

int main()
{
    using namespace triumph;

    REQUIRE (std::abs (advanced::pitchRatioFromSemitones (12.0) - 2.0) < 1.0e-9);
    REQUIRE (std::abs (advanced::pitchRatioFromSemitones (-12.0) - 0.5) < 1.0e-9);
    REQUIRE (advanced::reversedPosition (0, 8) == 7);
    REQUIRE (advanced::reversedPosition (7, 8) == 0);
    REQUIRE (advanced::reversedPosition (100, 8) == 0);

    const auto gain = advanced::normalisationGain (0.5f, -1.0);
    REQUIRE (std::abs (gain * 0.5f
                       - advanced::targetAmplitudeFromDb (-1.0)) < 0.0001);
    REQUIRE (advanced::normalisationGain (0.0f, -1.0) == 1.0f);
    REQUIRE (advanced::normalisationGain (0.001f, -1.0) == 16.0f);

    constexpr double sampleRate = 48000.0;
    constexpr double sourceFrequency = 440.0;
    constexpr std::size_t sampleCount = 48000;
    advanced::AudioChannels input (2, std::vector<float> (sampleCount));
    constexpr double twoPi = 6.28318530717958647692;
    for (std::size_t sample = 0; sample < sampleCount; ++sample)
    {
        const auto value = static_cast<float> (
            0.5 * std::sin (twoPi * sourceFrequency
                            * static_cast<double> (sample) / sampleRate));
        input[0][sample] = value;
        input[1][sample] = value;
    }
    const auto shifted = advanced::pitchShiftOla (input, 12.0);
    REQUIRE (shifted.size() == 2);
    REQUIRE (shifted[0].size() == sampleCount);
    REQUIRE (spectralMagnitude (shifted[0], 880.0, sampleRate)
             > spectralMagnitude (shifted[0], 440.0, sampleRate) * 2.0);
    for (std::size_t sample = 0; sample < sampleCount; sample += 257)
        REQUIRE (std::abs (shifted[0][sample] - shifted[1][sample]) < 1.0e-6f);

    return 0;
}

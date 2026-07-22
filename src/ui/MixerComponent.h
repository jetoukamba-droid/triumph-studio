#pragma once

#include <JuceHeader.h>

#include "audio/AudioEngine.h"
#include "model/ProjectModel.h"

#include <array>
#include <memory>

namespace triumph
{
class MixerComponent final : public juce::Component,
                             private juce::Timer
{
public:
    MixerComponent (ProjectModel& projectModel, AudioEngine& sharedEngine);
    ~MixerComponent() override;

    void refreshFromModel();
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class ChannelStrip;
    class MasterStrip;
    class SpectrumDisplay;

    void timerCallback() override;
    void rebuildStrips();
    juce::String makeStructureSignature() const;
    void updateSpectrum();

    ProjectModel& project;
    AudioEngine& engine;
    juce::TextButton addReturnButton { "+ Return" };
    juce::TextButton addBusButton { "+ Bus" };
    juce::Label titleLabel { {}, "MIXER" };
    juce::Label routingStatusLabel;
    juce::Viewport viewport;
    juce::Component stripContent;
    juce::OwnedArray<ChannelStrip> strips;
    std::unique_ptr<MasterStrip> masterStrip;
    std::unique_ptr<SpectrumDisplay> spectrumDisplay;
    juce::String structureSignature;

    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window {
        fftSize, juce::dsp::WindowingFunction<float>::hann, false
    };
    std::array<float, fftSize> spectrumInput {};
    std::array<float, fftSize * 2> fftData {};
    int spectrumInputCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerComponent)
};
}

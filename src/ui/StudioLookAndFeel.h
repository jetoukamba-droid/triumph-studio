#pragma once

#include <JuceHeader.h>

namespace triumph
{
namespace StudioColours
{
// Original Triumph Studio blue palette: deep navy layers, electric-blue focus,
// cyan audio detail, and coral transport alerts.
inline const juce::Colour background { 0xff07111d };
inline const juce::Colour workspace { 0xff0b1724 };
inline const juce::Colour panel { 0xff101e2d };
inline const juce::Colour panelRaised { 0xff18283a };
inline const juce::Colour surface { 0xff132438 };
inline const juce::Colour border { 0xff34485f };
inline const juce::Colour primary { 0xff2f7df6 };
inline const juce::Colour primaryBright { 0xff63a9ff };
inline const juce::Colour selection { 0xff1b4c7c };
inline const juce::Colour text { 0xffedf5ff };
inline const juce::Colour textMuted { 0xff91a5bc };
inline const juce::Colour waveform { 0xff62aaff };
inline const juce::Colour playhead { 0xffef5263 };
inline const juce::Colour green { 0xff31c48d };
inline const juce::Colour red { 0xffef5263 };
inline const juce::Colour orange { 0xfff2aa58 };
inline const juce::Colour cyan { 0xff45d0dc };
inline const juce::Colour violet { 0xff8991ff };
inline const juce::Colour clipAmber { 0xfff2aa58 };
inline const juce::Colour clipGreen { 0xff31c48d };
inline const juce::Colour clipBlue { 0xff2f7df6 };
inline const juce::Colour clipRose { 0xffef5263 };
}

class StudioLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    StudioLookAndFeel();

    void drawButtonBackground (juce::Graphics&,
                               juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics&,
                         juce::TextButton&,
                         bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;

    void drawLinearSlider (juce::Graphics&,
                           int x,
                           int y,
                           int width,
                           int height,
                           float sliderPos,
                           float minSliderPos,
                           float maxSliderPos,
                           const juce::Slider::SliderStyle,
                           juce::Slider&) override;

    void drawRotarySlider (juce::Graphics&,
                           int x,
                           int y,
                           int width,
                           int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider&) override;

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Font getLabelFont (juce::Label&) override;
};
}

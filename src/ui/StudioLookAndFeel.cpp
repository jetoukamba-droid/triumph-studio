#include "StudioLookAndFeel.h"

#include <cmath>

namespace triumph
{
StudioLookAndFeel::StudioLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, StudioColours::background);
    setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, StudioColours::text);
    setColour (juce::TextButton::buttonColourId, StudioColours::panelRaised);
    setColour (juce::TextButton::buttonOnColourId, StudioColours::primary);
    setColour (juce::TextButton::textColourOffId, StudioColours::text);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::Slider::backgroundColourId, StudioColours::border.darker (0.18f));
    setColour (juce::Slider::trackColourId, StudioColours::primary);
    setColour (juce::Slider::thumbColourId, StudioColours::primaryBright);
    setColour (juce::Slider::textBoxTextColourId, StudioColours::text);
    setColour (juce::Slider::textBoxBackgroundColourId, StudioColours::surface);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::ScrollBar::thumbColourId, StudioColours::border.brighter (0.25f));
    setColour (juce::ScrollBar::backgroundColourId, StudioColours::panel);

    setColour (juce::ComboBox::backgroundColourId, StudioColours::surface);
    setColour (juce::ComboBox::textColourId, StudioColours::text);
    setColour (juce::ComboBox::outlineColourId, StudioColours::border);
    setColour (juce::ComboBox::arrowColourId, StudioColours::primaryBright);
    setColour (juce::ComboBox::focusedOutlineColourId, StudioColours::primary);

    setColour (juce::PopupMenu::backgroundColourId, StudioColours::panelRaised);
    setColour (juce::PopupMenu::textColourId, StudioColours::text);
    setColour (juce::PopupMenu::headerTextColourId, StudioColours::primaryBright);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, StudioColours::selection);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);

    setColour (juce::TextEditor::backgroundColourId, StudioColours::surface);
    setColour (juce::TextEditor::textColourId, StudioColours::text);
    setColour (juce::TextEditor::highlightColourId, StudioColours::selection);
    setColour (juce::TextEditor::highlightedTextColourId, juce::Colours::white);
    setColour (juce::TextEditor::outlineColourId, StudioColours::border);
    setColour (juce::TextEditor::focusedOutlineColourId, StudioColours::primary);
    setColour (juce::TextEditor::shadowColourId, juce::Colours::transparentBlack);

    setColour (juce::ListBox::backgroundColourId, StudioColours::surface);
    setColour (juce::ListBox::outlineColourId, StudioColours::border);
    setColour (juce::ToggleButton::textColourId, StudioColours::text);
    setColour (juce::ToggleButton::tickColourId, StudioColours::primaryBright);
    setColour (juce::ToggleButton::tickDisabledColourId, StudioColours::textMuted);

    setColour (juce::AlertWindow::backgroundColourId, StudioColours::panel);
    setColour (juce::AlertWindow::textColourId, StudioColours::text);
    setColour (juce::AlertWindow::outlineColourId, StudioColours::border);
    setColour (juce::TooltipWindow::backgroundColourId, StudioColours::panelRaised);
    setColour (juce::TooltipWindow::textColourId, StudioColours::text);
    setColour (juce::TooltipWindow::outlineColourId, StudioColours::border);
}

void StudioLookAndFeel::drawButtonBackground (juce::Graphics& graphics,
                                              juce::Button& button,
                                              const juce::Colour& backgroundColour,
                                              bool highlighted,
                                              bool down)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    auto colour = backgroundColour;

    if (down)
        colour = colour.darker (0.20f);
    else if (highlighted)
        colour = colour.brighter (0.08f);

    graphics.setColour (colour);
    graphics.fillRoundedRectangle (bounds, 5.0f);
    graphics.setColour (button.getToggleState() ? StudioColours::primaryBright
                                                 : StudioColours::border);
    graphics.drawRoundedRectangle (bounds, 5.0f, button.getToggleState() ? 1.4f : 1.0f);
}

void StudioLookAndFeel::drawButtonText (juce::Graphics& graphics,
                                        juce::TextButton& button,
                                        bool,
                                        bool)
{
    graphics.setFont (getTextButtonFont (button, button.getHeight()));
    graphics.setColour (button.findColour (button.getToggleState()
                                               ? juce::TextButton::textColourOnId
                                               : juce::TextButton::textColourOffId));
    graphics.drawFittedText (button.getButtonText(),
                             button.getLocalBounds().reduced (8, 2),
                             juce::Justification::centred,
                             1);
}

void StudioLookAndFeel::drawLinearSlider (juce::Graphics& graphics,
                                          int x,
                                          int y,
                                          int width,
                                          int height,
                                          float sliderPos,
                                          float minSliderPos,
                                          float maxSliderPos,
                                          const juce::Slider::SliderStyle style,
                                          juce::Slider& slider)
{
    if (style != juce::Slider::LinearHorizontal
        && style != juce::Slider::LinearVertical)
    {
        juce::LookAndFeel_V4::drawLinearSlider (graphics, x, y, width, height,
                                                sliderPos, minSliderPos, maxSliderPos, style, slider);
        return;
    }

    if (style == juce::Slider::LinearHorizontal)
    {
        const auto centreY = static_cast<float> (y) + static_cast<float> (height) * 0.5f;
        const auto startX = static_cast<float> (x) + 3.0f;
        const auto endX = static_cast<float> (x + width) - 3.0f;

        graphics.setColour (slider.findColour (juce::Slider::backgroundColourId));
        graphics.drawLine (startX, centreY, endX, centreY, 5.0f);
        graphics.setColour (slider.findColour (juce::Slider::trackColourId));
        graphics.drawLine (startX, centreY, sliderPos, centreY, 5.0f);
        graphics.setColour (slider.findColour (juce::Slider::thumbColourId));
        graphics.fillRoundedRectangle (sliderPos - 5.0f, centreY - 8.0f,
                                       10.0f, 16.0f, 3.0f);
        return;
    }

    const auto centreX = static_cast<float> (x) + static_cast<float> (width) * 0.5f;
    const auto startY = static_cast<float> (y) + 5.0f;
    const auto endY = static_cast<float> (y + height) - 20.0f;
    graphics.setColour (slider.findColour (juce::Slider::backgroundColourId));
    graphics.drawLine (centreX, startY, centreX, endY, 6.0f);
    graphics.setColour (slider.findColour (juce::Slider::trackColourId));
    graphics.drawLine (centreX, sliderPos, centreX, endY, 6.0f);
    graphics.setColour (slider.findColour (juce::Slider::thumbColourId));
    graphics.fillRoundedRectangle (centreX - 13.0f, sliderPos - 4.0f,
                                   26.0f, 8.0f, 2.0f);
    graphics.setColour (StudioColours::border);
    graphics.drawRoundedRectangle (centreX - 13.0f, sliderPos - 4.0f,
                                   26.0f, 8.0f, 2.0f, 1.0f);
}

void StudioLookAndFeel::drawRotarySlider (juce::Graphics& graphics,
                                          int x,
                                          int y,
                                          int width,
                                          int height,
                                          float sliderPosProportional,
                                          float rotaryStartAngle,
                                          float rotaryEndAngle,
                                          juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float> (
        static_cast<float> (x), static_cast<float> (y),
        static_cast<float> (width), static_cast<float> (height)).reduced (8.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle = rotaryStartAngle
        + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    graphics.setColour (StudioColours::surface);
    graphics.fillEllipse (centre.x - radius, centre.y - radius,
                          radius * 2.0f, radius * 2.0f);
    graphics.setColour (StudioColours::border);
    graphics.drawEllipse (centre.x - radius, centre.y - radius,
                          radius * 2.0f, radius * 2.0f, 1.0f);

    juce::Path valueArc;
    valueArc.addCentredArc (centre.x, centre.y,
                            radius - 4.0f, radius - 4.0f,
                            0.0f, rotaryStartAngle, angle, true);
    graphics.setColour (slider.findColour (juce::Slider::trackColourId));
    graphics.strokePath (valueArc, juce::PathStrokeType (3.0f,
                                                         juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));

    const auto pointerLength = radius - 8.0f;
    const auto pointer = juce::Line<float> (
        centre,
        centre + juce::Point<float> (std::cos (angle - juce::MathConstants<float>::halfPi),
                                     std::sin (angle - juce::MathConstants<float>::halfPi))
            * pointerLength);
    graphics.setColour (StudioColours::text);
    graphics.drawLine (pointer, 2.0f);
}

juce::Font StudioLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return juce::Font (juce::FontOptions (juce::jmin (15.0f, buttonHeight * 0.42f),
                                           juce::Font::bold));
}

juce::Font StudioLookAndFeel::getLabelFont (juce::Label& label)
{
    const auto height = label.getProperties().getWithDefault ("fontSize", 14.0f);
    const auto bold = static_cast<bool> (label.getProperties().getWithDefault ("bold", false));
    return juce::Font (juce::FontOptions (static_cast<float> (height),
                                          bold ? juce::Font::bold : juce::Font::plain));
}
}

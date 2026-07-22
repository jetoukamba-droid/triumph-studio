#pragma once

#include <JuceHeader.h>

#include "StudioLookAndFeel.h"

namespace triumph
{
enum class StudioIcon
{
    undo,
    redo,
    newProject,
    open,
    recent,
    save,
    saveAs,
    collect,
    relink,
    importAudio,
    exportMix,
    addTrack,
    addMidi,
    pianoRoll,
    plugin,
    device,
    lowLatency,
    more,
    rewind,
    play,
    pause,
    stop,
    record,
    monitor,
    mixer,
    tempo,
    edit,
    producer,
    help,
    split,
    useTake,
    detect,
    warp,
    snap
};

class StudioIconButton final : public juce::Button
{
public:
    StudioIconButton (StudioIcon iconToDraw, const juce::String& accessibleName)
        : juce::Button (accessibleName), icon (iconToDraw)
    {
        setButtonText (accessibleName);
    }

    void setTextOnly (bool shouldUseText)
    {
        textOnly = shouldUseText;
        repaint();
    }

    void paintButton (juce::Graphics& graphics,
                      bool highlighted,
                      bool down) override
    {
        auto background = findColour (getToggleState()
                                          ? juce::TextButton::buttonOnColourId
                                          : juce::TextButton::buttonColourId);
        if (down)
            background = background.darker (0.14f);
        else if (highlighted)
            background = background.brighter (0.07f);
        if (! isEnabled())
            background = background.withMultipliedAlpha (0.52f);

        const auto bounds = getLocalBounds().toFloat().reduced (0.5f);
        graphics.setColour (background);
        graphics.fillRoundedRectangle (bounds, 5.0f);
        graphics.setColour ((getToggleState() ? StudioColours::primaryBright
                                               : StudioColours::border)
                                .withMultipliedAlpha (isEnabled() ? 1.0f : 0.5f));
        graphics.drawRoundedRectangle (bounds, 5.0f,
                                       getToggleState() ? 1.4f : 1.0f);
        if (hasKeyboardFocus (true))
        {
            graphics.setColour (StudioColours::primaryBright);
            graphics.drawRoundedRectangle (bounds.reduced (2.5f), 5.0f, 1.5f);
        }

        auto foreground = findColour (getToggleState()
                                          ? juce::TextButton::textColourOnId
                                          : juce::TextButton::textColourOffId);
        if (! isEnabled())
            foreground = foreground.withMultipliedAlpha (0.42f);
        graphics.setColour (foreground);
        if (textOnly)
        {
            graphics.setFont (juce::Font (
                juce::FontOptions (13.0f, juce::Font::bold)));
            graphics.drawFittedText (getButtonText(),
                                     getLocalBounds().reduced (7, 2),
                                     juce::Justification::centred, 1, 0.76f);
        }
        else
        {
            drawIcon (graphics, getLocalBounds().toFloat().reduced (8.0f));
        }
    }

private:
    void drawIcon (juce::Graphics& graphics, juce::Rectangle<float> area) const
    {
        const auto scale = juce::jmin (area.getWidth(), area.getHeight()) / 24.0f;
        const auto originX = area.getCentreX() - 12.0f * scale;
        const auto originY = area.getCentreY() - 12.0f * scale;
        const auto point = [=] (float x, float y)
        {
            return juce::Point<float> (originX + x * scale, originY + y * scale);
        };
        const auto rect = [=] (float x, float y, float w, float h)
        {
            return juce::Rectangle<float> (originX + x * scale,
                                           originY + y * scale,
                                           w * scale, h * scale);
        };
        const auto stroke = juce::jmax (1.4f, 1.8f * scale);
        const auto line = [&] (float x1, float y1, float x2, float y2,
                               float width = 0.0f)
        {
            graphics.drawLine (juce::Line<float> (point (x1, y1),
                                                   point (x2, y2)),
                               width > 0.0f ? width : stroke);
        };
        const auto outline = [&] (const juce::Path& path)
        {
            graphics.strokePath (path, juce::PathStrokeType (
                stroke, juce::PathStrokeType::curved,
                juce::PathStrokeType::rounded));
        };

        juce::Path path;
        switch (icon)
        {
            case StudioIcon::undo:
            case StudioIcon::redo:
            {
                const auto mirrored = icon == StudioIcon::redo;
                const auto px = [mirrored] (float x) { return mirrored ? 24.0f - x : x; };
                path.startNewSubPath (point (px (18.5f), 17.5f));
                path.cubicTo (point (px (17.5f), 9.0f), point (px (10.0f), 7.5f),
                              point (px (5.5f), 11.5f));
                outline (path);
                line (px (5.5f), 11.5f, px (6.2f), 6.5f);
                line (px (5.5f), 11.5f, px (10.5f), 11.8f);
                break;
            }
            case StudioIcon::newProject:
                graphics.drawRoundedRectangle (rect (6, 3, 12, 18), 1.5f, stroke);
                line (12, 8, 12, 16); line (8, 12, 16, 12); break;
            case StudioIcon::open:
                path.startNewSubPath (point (3, 8)); path.lineTo (9, 8);
                path.lineTo (11, 5); path.lineTo (21, 5); path.lineTo (20, 18);
                path.lineTo (4, 18); path.closeSubPath(); outline (path); break;
            case StudioIcon::recent:
                graphics.drawEllipse (rect (4, 4, 16, 16), stroke);
                line (12, 7, 12, 12); line (12, 12, 16, 14); break;
            case StudioIcon::save:
            case StudioIcon::saveAs:
                graphics.drawRoundedRectangle (rect (4, 3, 16, 18), 1.4f, stroke);
                graphics.fillRect (rect (8, 4, 8, 5));
                graphics.drawRoundedRectangle (rect (7, 13, 10, 6), 1.0f, stroke);
                if (icon == StudioIcon::saveAs)
                {
                    graphics.fillEllipse (rect (15, 1, 8, 8));
                    graphics.setColour (backgroundColourForOverlay());
                    line (17, 5, 21, 5, juce::jmax (1.0f, scale));
                    line (19, 3, 19, 7, juce::jmax (1.0f, scale));
                }
                break;
            case StudioIcon::collect:
                graphics.drawRoundedRectangle (rect (4, 7, 16, 13), 1.5f, stroke);
                line (4, 10, 20, 10); line (9, 4, 12, 7); line (15, 4, 12, 7); break;
            case StudioIcon::relink:
            {
                path.addRoundedRectangle (rect (3, 8, 10, 7), 3.5f);
                path.addRoundedRectangle (rect (11, 8, 10, 7), 3.5f);
                outline (path); line (9, 11.5f, 15, 11.5f); break;
            }
            case StudioIcon::importAudio:
            case StudioIcon::exportMix:
            {
                const auto importing = icon == StudioIcon::importAudio;
                line (4, 18, 20, 18); line (5, 14, 7, 11); line (7, 11, 9, 15);
                line (9, 15, 12, 9); line (12, 9, 15, 14);
                line (17, importing ? 4 : 13, 17, importing ? 12 : 5);
                line (14, importing ? 9 : 8, 17, importing ? 12 : 5);
                line (20, importing ? 9 : 8, 17, importing ? 12 : 5);
                break;
            }
            case StudioIcon::addTrack:
            case StudioIcon::addMidi:
                if (icon == StudioIcon::addTrack)
                {
                    path.startNewSubPath (point (3, 13));
                    path.cubicTo (point (6, 6), point (9, 20), point (12, 12));
                    path.cubicTo (point (14, 8), point (16, 14), point (17, 11));
                    outline (path);
                }
                else
                {
                    line (7, 5, 7, 16); line (7, 5, 14, 3); line (14, 3, 14, 13);
                    graphics.fillEllipse (rect (3, 14, 5, 4));
                    graphics.fillEllipse (rect (10, 11, 5, 4));
                }
                graphics.drawEllipse (rect (15, 15, 8, 8), stroke);
                line (17, 19, 21, 19); line (19, 17, 19, 21); break;
            case StudioIcon::pianoRoll:
                graphics.drawRoundedRectangle (rect (3, 6, 18, 12), 1.2f, stroke);
                for (float x : { 7.5f, 12.0f, 16.5f }) line (x, 6, x, 18, stroke * 0.75f);
                graphics.fillRect (rect (6.2f, 6, 2.4f, 6));
                graphics.fillRect (rect (10.8f, 6, 2.4f, 6));
                graphics.fillRect (rect (15.2f, 6, 2.4f, 6)); break;
            case StudioIcon::plugin:
                graphics.drawRoundedRectangle (rect (6, 6, 12, 12), 2.0f, stroke);
                line (9, 3, 9, 6); line (15, 3, 15, 6);
                line (9, 18, 9, 21); line (15, 18, 15, 21);
                line (3, 9, 6, 9); line (3, 15, 6, 15);
                line (18, 9, 21, 9); line (18, 15, 21, 15); break;
            case StudioIcon::device:
                path.startNewSubPath (point (4, 11)); path.lineTo (8, 11);
                path.lineTo (13, 7); path.lineTo (13, 17); path.lineTo (8, 13);
                path.lineTo (4, 13); path.closeSubPath(); graphics.fillPath (path);
                path.clear(); path.startNewSubPath (point (16, 9));
                path.cubicTo (point (19, 11), point (19, 13), point (16, 15)); outline (path); break;
            case StudioIcon::lowLatency:
                path.startNewSubPath (point (13, 2)); path.lineTo (6, 13);
                path.lineTo (12, 13); path.lineTo (10, 22); path.lineTo (19, 10);
                path.lineTo (13, 10); path.closeSubPath(); graphics.fillPath (path); break;
            case StudioIcon::more:
                for (float x : { 5.0f, 12.0f, 19.0f }) graphics.fillEllipse (rect (x - 1.5f, 10.5f, 3, 3));
                break;
            case StudioIcon::rewind:
                path.addTriangle (point (11, 5), point (11, 19), point (3, 12));
                path.addTriangle (point (20, 5), point (20, 19), point (12, 12));
                graphics.fillPath (path); break;
            case StudioIcon::play:
                path.addTriangle (point (7, 4), point (7, 20), point (20, 12)); graphics.fillPath (path); break;
            case StudioIcon::pause:
                graphics.fillRoundedRectangle (rect (6, 4, 4, 16), 1.0f);
                graphics.fillRoundedRectangle (rect (14, 4, 4, 16), 1.0f); break;
            case StudioIcon::stop:
                graphics.fillRoundedRectangle (rect (6, 6, 12, 12), 1.2f); break;
            case StudioIcon::record:
                graphics.fillEllipse (rect (5, 5, 14, 14)); break;
            case StudioIcon::monitor:
                path.startNewSubPath (point (4, 13));
                path.cubicTo (point (4, 3), point (20, 3), point (20, 13)); outline (path);
                graphics.fillRoundedRectangle (rect (3, 12, 5, 8), 1.5f);
                graphics.fillRoundedRectangle (rect (16, 12, 5, 8), 1.5f); break;
            case StudioIcon::mixer:
                for (float x : { 6.0f, 12.0f, 18.0f }) line (x, 3, x, 21);
                graphics.fillRoundedRectangle (rect (4, 7, 4, 5), 1.0f);
                graphics.fillRoundedRectangle (rect (10, 14, 4, 5), 1.0f);
                graphics.fillRoundedRectangle (rect (16, 5, 4, 5), 1.0f); break;
            case StudioIcon::tempo:
                path.startNewSubPath (point (5, 20)); path.lineTo (9, 4);
                path.lineTo (15, 4); path.lineTo (19, 20); path.closeSubPath(); outline (path);
                line (12, 7, 16, 16); graphics.fillEllipse (rect (14, 14, 4, 4)); break;
            case StudioIcon::edit:
            case StudioIcon::split:
                graphics.drawEllipse (rect (3, 4, 6, 6), stroke);
                graphics.drawEllipse (rect (3, 14, 6, 6), stroke);
                line (8, 8, 20, 18); line (8, 16, 20, 6); break;
            case StudioIcon::producer:
                path.addStar (point (12, 12), 4, 3.0f * scale, 9.0f * scale);
                graphics.fillPath (path); graphics.fillEllipse (rect (3, 3, 3, 3)); break;
            case StudioIcon::help:
                graphics.drawEllipse (rect (4, 4, 16, 16), stroke);
                path.startNewSubPath (point (9, 9));
                path.cubicTo (point (10, 5), point (17, 6), point (15, 11));
                path.cubicTo (point (14, 13), point (12, 12), point (12, 15)); outline (path);
                graphics.fillEllipse (rect (11, 17, 2, 2)); break;
            case StudioIcon::useTake:
                line (4, 6, 20, 6); line (4, 12, 20, 12); line (4, 18, 20, 18);
                line (6, 12, 10, 16); line (10, 16, 18, 8); break;
            case StudioIcon::detect:
                path.startNewSubPath (point (2, 13)); path.lineTo (6, 13);
                path.lineTo (9, 5); path.lineTo (13, 19); path.lineTo (16, 10);
                path.lineTo (19, 13); path.lineTo (22, 13); outline (path); break;
            case StudioIcon::warp:
                line (4, 18, 8, 7); line (8, 7, 14, 16); line (14, 16, 20, 5);
                for (const auto& p : { point (4, 18), point (8, 7), point (14, 16), point (20, 5) })
                    graphics.fillEllipse (juce::Rectangle<float> (
                        p.x - 2.0f * scale, p.y - 2.0f * scale,
                        4.0f * scale, 4.0f * scale));
                break;
            case StudioIcon::snap:
                path.startNewSubPath (point (5, 4)); path.lineTo (5, 13);
                path.cubicTo (point (5, 22), point (19, 22), point (19, 13));
                path.lineTo (19, 4); outline (path);
                line (5, 8, 10, 8); line (14, 8, 19, 8); break;
        }
    }

    juce::Colour backgroundColourForOverlay() const
    {
        return findColour (getToggleState() ? juce::TextButton::buttonOnColourId
                                             : juce::TextButton::buttonColourId);
    }

    StudioIcon icon;
    bool textOnly = false;
};
}

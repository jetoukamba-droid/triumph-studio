#pragma once

#include <JuceHeader.h>

#include "core/HelpAssistant.h"

#include <functional>
#include <vector>

namespace triumph
{
class HelpAssistantComponent final : public juce::Component
{
public:
    HelpAssistantComponent();

    void focusQuestion();
    void askQuestion (juce::String question);
    void setContextSummary (juce::String summary);

    std::function<void (const juce::String&)> onActionRequested;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void performSearch();
    void showArticle (const help::Article& article);
    void showNoMatch();
    void setQuickQuestion (const juce::String& question);

    juce::Label titleLabel { {}, "TRIUMPH ASSISTANT" };
    juce::Label localLabel { {}, "OFFLINE - PRIVATE" };
    juce::Label contextLabel { {}, "Ready" };
    juce::Label questionLabel { {}, "ASK A HOW-TO QUESTION" };
    juce::TextEditor questionEditor;
    juce::TextButton askButton { "Ask" };
    juce::TextButton recordingButton { "Recording" };
    juce::TextButton mixerButton { "Mixer" };
    juce::TextButton midiButton { "MIDI & Keys" };
    juce::TextButton producerButton { "AI Producer" };
    juce::TextButton exportButton { "Export" };
    juce::TextButton shortcutsButton { "Shortcuts" };
    juce::Label resultsLabel { {}, "RELATED HELP" };
    juce::ComboBox resultsBox;
    juce::Label answerTitle;
    juce::TextEditor answerEditor;
    juce::TextButton actionButton { "Open Related Control" };
    juce::Label privacyLabel {
        {}, "Built-in guidance only. No audio, project data, or question leaves this computer." };

    std::vector<const help::Article*> resultArticles;
    juce::String currentAction;
};
}

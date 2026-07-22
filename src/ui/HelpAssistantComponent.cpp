#include "HelpAssistantComponent.h"

#include "StudioLookAndFeel.h"

namespace triumph
{
HelpAssistantComponent::HelpAssistantComponent()
{
    juce::Component* components[] = {
        &titleLabel, &localLabel, &contextLabel, &questionLabel, &questionEditor,
        &askButton, &recordingButton, &mixerButton, &midiButton, &producerButton,
        &exportButton, &shortcutsButton, &resultsLabel, &resultsBox, &answerTitle,
        &answerEditor, &actionButton, &privacyLabel
    };
    for (auto* component : components) addAndMakeVisible (*component);

    titleLabel.getProperties().set ("fontSize", 15.0f);
    titleLabel.getProperties().set ("bold", true);
    titleLabel.setColour (juce::Label::textColourId, StudioColours::text);
    localLabel.getProperties().set ("bold", true);
    localLabel.setColour (juce::Label::textColourId, StudioColours::green);
    contextLabel.setJustificationType (juce::Justification::centredRight);
    contextLabel.setColour (juce::Label::textColourId, StudioColours::textMuted);
    questionLabel.setColour (juce::Label::textColourId, StudioColours::textMuted);
    resultsLabel.setColour (juce::Label::textColourId, StudioColours::textMuted);
    answerTitle.getProperties().set ("fontSize", 15.0f);
    answerTitle.getProperties().set ("bold", true);
    answerTitle.setColour (juce::Label::textColourId, StudioColours::primary);
    privacyLabel.setColour (juce::Label::textColourId, StudioColours::textMuted);

    questionEditor.setMultiLine (false);
    questionEditor.setReturnKeyStartsNewLine (false);
    questionEditor.setTextToShowWhenEmpty (
        "Example: How do I record a microphone?", StudioColours::textMuted);
    questionEditor.setColour (juce::TextEditor::backgroundColourId,
                              StudioColours::surface);
    questionEditor.setColour (juce::TextEditor::outlineColourId,
                              StudioColours::border);
    questionEditor.setColour (juce::TextEditor::focusedOutlineColourId,
                              StudioColours::primary);
    questionEditor.onReturnKey = [this] { performSearch(); };
    askButton.onClick = [this] { performSearch(); };
    askButton.setColour (juce::TextButton::buttonColourId, StudioColours::primary);
    askButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);

    recordingButton.onClick = [this] {
        setQuickQuestion ("How do I record a microphone safely?");
    };
    mixerButton.onClick = [this] {
        setQuickQuestion ("How do I use mixer routing, sends, and sidechain?");
    };
    midiButton.onClick = [this] {
        setQuickQuestion ("How do I record and edit MIDI in Keys?");
    };
    producerButton.onClick = [this] {
        setQuickQuestion ("How do I use AI Producer?");
    };
    exportButton.onClick = [this] {
        setQuickQuestion ("How do I export a mix or stems?");
    };
    shortcutsButton.onClick = [this] {
        setQuickQuestion ("What are the keyboard shortcuts?");
    };

    resultsBox.onChange = [this]
    {
        const auto index = resultsBox.getSelectedItemIndex();
        if (index >= 0 && index < static_cast<int> (resultArticles.size()))
            showArticle (*resultArticles[static_cast<std::size_t> (index)]);
    };

    answerEditor.setMultiLine (true);
    answerEditor.setReadOnly (true);
    answerEditor.setScrollbarsShown (true);
    answerEditor.setCaretVisible (false);
    answerEditor.setPopupMenuEnabled (true);
    answerEditor.setColour (juce::TextEditor::backgroundColourId,
                            StudioColours::surface);
    answerEditor.setColour (juce::TextEditor::outlineColourId,
                            StudioColours::border);

    actionButton.onClick = [this]
    {
        if (currentAction.isNotEmpty() && onActionRequested)
            onActionRequested (currentAction);
    };
    actionButton.setColour (juce::TextButton::buttonColourId,
                            StudioColours::green);
    actionButton.setColour (juce::TextButton::textColourOffId,
                            juce::Colours::white);

    showArticle (help::articles().front());
}

void HelpAssistantComponent::focusQuestion()
{
    questionEditor.grabKeyboardFocus();
    questionEditor.selectAll();
}

void HelpAssistantComponent::askQuestion (juce::String question)
{
    questionEditor.setText (std::move (question), false);
    performSearch();
}

void HelpAssistantComponent::setContextSummary (juce::String summary)
{
    contextLabel.setText (std::move (summary), juce::dontSendNotification);
}

void HelpAssistantComponent::performSearch()
{
    const auto question = questionEditor.getText().trim();
    if (question.isEmpty())
    {
        answerTitle.setText ("Ask a question first", juce::dontSendNotification);
        answerEditor.setText (
            "Type a question about recording, devices, editing, MIDI, plug-ins, mixer routing, tempo, AI Producer, export, saving, or shortcuts.",
            false);
        return;
    }

    const auto results = help::search (question.toStdString(), 6);
    resultArticles.clear();
    resultsBox.clear (juce::dontSendNotification);
    for (const auto& result : results)
    {
        resultArticles.push_back (result.article);
        resultsBox.addItem (juce::String (result.article->title),
                            static_cast<int> (resultArticles.size()));
    }
    if (resultArticles.empty())
    {
        showNoMatch();
        return;
    }
    resultsBox.setSelectedId (1, juce::sendNotificationSync);
}

void HelpAssistantComponent::showArticle (const help::Article& article)
{
    answerTitle.setText (juce::String (article.title),
                         juce::dontSendNotification);
    answerEditor.setText (juce::String (article.summary) + "\n\n"
                              + juce::String (article.steps), false);
    answerEditor.setCaretPosition (0);
    currentAction = juce::String (article.action);
    actionButton.setButtonText (article.actionLabel.empty()
                                    ? juce::String ("Open Related Control")
                                    : juce::String (article.actionLabel));
    actionButton.setVisible (currentAction.isNotEmpty());
}

void HelpAssistantComponent::showNoMatch()
{
    resultsBox.clear (juce::dontSendNotification);
    answerTitle.setText ("No built-in answer found",
                         juce::dontSendNotification);
    answerEditor.setText (
        "Try fewer words or choose a quick topic. Triumph Assistant answers only from verified built-in help and will not invent a feature or send the question online.",
        false);
    currentAction.clear();
    actionButton.setVisible (false);
}

void HelpAssistantComponent::setQuickQuestion (const juce::String& question)
{
    questionEditor.setText (question, false);
    performSearch();
}

void HelpAssistantComponent::paint (juce::Graphics& graphics)
{
    graphics.fillAll (StudioColours::panel);
    graphics.setColour (StudioColours::border);
    graphics.drawHorizontalLine (0, 0.0f, static_cast<float> (getWidth()));
}

void HelpAssistantComponent::resized()
{
    auto bounds = getLocalBounds().reduced (16, 10);
    auto header = bounds.removeFromTop (26);
    titleLabel.setBounds (header.removeFromLeft (170));
    localLabel.setBounds (header.removeFromLeft (150));
    contextLabel.setBounds (header);

    bounds.removeFromTop (4);
    auto question = bounds.removeFromTop (38);
    questionLabel.setBounds (question.removeFromLeft (154));
    askButton.setBounds (question.removeFromRight (72).reduced (2));
    questionEditor.setBounds (question.reduced (2));

    bounds.removeFromTop (4);
    auto quick = bounds.removeFromTop (34);
    recordingButton.setBounds (quick.removeFromLeft (104).reduced (2));
    mixerButton.setBounds (quick.removeFromLeft (86).reduced (2));
    midiButton.setBounds (quick.removeFromLeft (110).reduced (2));
    producerButton.setBounds (quick.removeFromLeft (110).reduced (2));
    exportButton.setBounds (quick.removeFromLeft (86).reduced (2));
    shortcutsButton.setBounds (quick.removeFromLeft (98).reduced (2));

    bounds.removeFromTop (5);
    auto content = bounds.removeFromTop (juce::jmax (100, bounds.getHeight() - 26));
    auto related = content.removeFromLeft (240);
    resultsLabel.setBounds (related.removeFromTop (24));
    resultsBox.setBounds (related.removeFromTop (34).reduced (2));
    related.removeFromTop (8);
    actionButton.setBounds (related.removeFromTop (36).reduced (2));

    content.removeFromLeft (12);
    answerTitle.setBounds (content.removeFromTop (26));
    answerEditor.setBounds (content.reduced (0, 2));
    privacyLabel.setBounds (bounds.removeFromBottom (24));
}
}

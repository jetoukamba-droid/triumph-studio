#include <JuceHeader.h>

#include "MainComponent.h"
#include "core/ReleaseIdentity.h"
#include "core/ResponsiveLayout.h"

namespace triumph
{
class TriumphStudioApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override
    {
        return "Triumph Studio";
    }

    const juce::String getApplicationVersion() override
    {
        return release::versionLabel;
    }

    bool moreThanOneInstanceAllowed() override
    {
        return false;
    }

    void initialise (const juce::String&) override
    {
        mainWindow = std::make_unique<MainWindow> (getApplicationName());
    }

    void shutdown() override
    {
        mainWindow.reset();
    }

    void systemRequestedQuit() override
    {
        if (mainWindow != nullptr)
            mainWindow->requestApplicationQuit();
        else
            quit();
    }

    void anotherInstanceStarted (const juce::String&) override
    {
        if (mainWindow != nullptr)
            mainWindow->toFront (true);
    }

private:
    class MainWindow final : public juce::DocumentWindow
    {
    public:
        explicit MainWindow (const juce::String& name)
            : DocumentWindow (name,
                              StudioColours::background,
                              DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            content = new MainComponent();
            setContentOwned (content, true);
            setResizable (true, true);
            setResizeLimits (layout::minimumWindowWidth,
                             layout::minimumWindowHeight, 3840, 2160);

            auto initialWidth = 1440;
            auto initialHeight = 900;

            if (const auto* primaryDisplay = juce::Desktop::getInstance()
                                                   .getDisplays().getPrimaryDisplay())
            {
                const auto availableWidth = static_cast<int> (
                    primaryDisplay->userBounds.getWidth()) - 80;
                const auto availableHeight = static_cast<int> (
                    primaryDisplay->userBounds.getHeight()) - 80;
                initialWidth = juce::jlimit (
                    layout::minimumWindowWidth, 1440, availableWidth);
                initialHeight = juce::jlimit (
                    layout::minimumWindowHeight, 900, availableHeight);
            }

            centreWithSize (initialWidth, initialHeight);
            setVisible (true);
        }

        void closeButtonPressed() override
        {
            requestApplicationQuit();
        }

        void requestApplicationQuit()
        {
            if (content == nullptr)
                return;

            content->requestClose ([]
            {
                juce::JUCEApplicationBase::quit();
            });
        }

    private:
        MainComponent* content = nullptr;
    };

    std::unique_ptr<MainWindow> mainWindow;
};
}

START_JUCE_APPLICATION (triumph::TriumphStudioApplication)

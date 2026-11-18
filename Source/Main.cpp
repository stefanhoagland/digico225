#include <juce_gui_basics/juce_gui_basics.h>

#include "UI/MainComponent.h"

namespace opendesk
{

class OpenDeskApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override    { return "OpenDesk"; }
    const juce::String getApplicationVersion() override { return OPENDESK_VERSION; }
    bool moreThanOneInstanceAllowed() override          { return false; }

    void initialise (const juce::String&) override
    {
        juce::PropertiesFile::Options options;
        options.applicationName     = "OpenDesk";
        options.filenameSuffix      = ".settings";
        options.osxLibrarySubFolder = "Application Support";
        settings.setStorageParameters (options);

        mainWindow = std::make_unique<MainWindow> (getApplicationName(), *settings.getUserSettings());
    }

    void shutdown() override
    {
        mainWindow = nullptr;
        settings.closeFiles();
    }

    void systemRequestedQuit() override { quit(); }

private:
    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow (const juce::String& name, juce::PropertiesFile& settingsFile)
            : DocumentWindow (name, Colours::background, DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new MainComponent (settingsFile), true);
            setResizable (true, true);
            setResizeLimits (800, 600, 10000, 10000);
            centreWithSize (getWidth(), getHeight());
            setVisible (true);
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

    juce::ApplicationProperties settings;
    std::unique_ptr<MainWindow> mainWindow;
};

} // namespace opendesk

START_JUCE_APPLICATION (opendesk::OpenDeskApplication)

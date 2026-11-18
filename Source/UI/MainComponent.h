#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "Engine/MixerEngine.h"
#include "ChannelStripComponent.h"
#include "MasterStripComponent.h"
#include "DeskLookAndFeel.h"

namespace opendesk
{

/** The main console window: top bar, scrolling bank of input channels, master strip. */
class MainComponent : public juce::Component,
                      private juce::Timer,
                      private juce::ChangeListener
{
public:
    explicit MainComponent (juce::PropertiesFile& settings);
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void showAudioSetup();
    void updateDeviceInfo();
    void saveDeviceState();

    juce::PropertiesFile& settings;
    DeskLookAndFeel lookAndFeel;

    juce::AudioDeviceManager deviceManager;
    MixerEngine engine;

    juce::Label titleLabel, deviceLabel, cpuLabel;
    juce::TextButton audioSetupButton { "Audio Setup" };

    juce::Component channelBank;
    juce::Viewport channelViewport;
    juce::OwnedArray<ChannelStripComponent> strips;
    MasterStripComponent masterStrip { engine };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

} // namespace opendesk

#include "MainComponent.h"

namespace opendesk
{

namespace
{
    constexpr auto kDeviceStateKey = "audioDeviceState";
    constexpr int  kTopBarHeight = 40;
}

MainComponent::MainComponent (juce::PropertiesFile& settingsFile)
    : settings (settingsFile)
{
    setLookAndFeel (&lookAndFeel);

    // Top bar
    titleLabel.setText ("OpenDesk", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    addAndMakeVisible (titleLabel);

    deviceLabel.setColour (juce::Label::textColourId, Colours::textDim);
    addAndMakeVisible (deviceLabel);

    cpuLabel.setJustificationType (juce::Justification::centredRight);
    cpuLabel.setColour (juce::Label::textColourId, Colours::textDim);
    addAndMakeVisible (cpuLabel);

    audioSetupButton.onClick = [this] { showAudioSetup(); };
    addAndMakeVisible (audioSetupButton);

    // Channel bank
    for (int i = 0; i < kNumChannels; ++i)
        channelBank.addAndMakeVisible (strips.add (new ChannelStripComponent (engine, i)));

    channelViewport.setViewedComponent (&channelBank, false);
    channelViewport.setScrollBarsShown (false, true);
    addAndMakeVisible (channelViewport);
    addAndMakeVisible (masterStrip);

    // Audio: open up to 64 ins and outs, restoring the last device used.
    auto saved = settings.getXmlValue (kDeviceStateKey);
    const auto error = deviceManager.initialise (kMaxHardwareIO, kMaxHardwareIO, saved.get(), true);
    if (error.isNotEmpty())
    {
        DBG ("Audio device error: " << error);
    }

    deviceManager.addAudioCallback (&engine);
    deviceManager.addChangeListener (this);
    updateDeviceInfo();

    setSize (1280, 760);
    startTimerHz (30);
}

MainComponent::~MainComponent()
{
    stopTimer();
    saveDeviceState();
    deviceManager.removeChangeListener (this);
    deviceManager.removeAudioCallback (&engine);
    deviceManager.closeAudioDevice();
    setLookAndFeel (nullptr);
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (Colours::background);
    g.setColour (Colours::panel);
    g.fillRect (getLocalBounds().removeFromTop (kTopBarHeight));
}

void MainComponent::resized()
{
    auto b = getLocalBounds();

    auto top = b.removeFromTop (kTopBarHeight).reduced (10, 6);
    titleLabel.setBounds (top.removeFromLeft (120));
    audioSetupButton.setBounds (top.removeFromRight (110));
    top.removeFromRight (10);
    cpuLabel.setBounds (top.removeFromRight (90));
    deviceLabel.setBounds (top);

    b.reduce (4, 6);
    masterStrip.setBounds (b.removeFromRight (MasterStripComponent::kWidth));
    b.removeFromRight (6);
    channelViewport.setBounds (b);

    const auto bankHeight = b.getHeight() - channelViewport.getScrollBarThickness();
    channelBank.setSize (strips.size() * ChannelStripComponent::kWidth, bankHeight);
    for (int i = 0; i < strips.size(); ++i)
        strips[i]->setBounds (i * ChannelStripComponent::kWidth, 0, ChannelStripComponent::kWidth, bankHeight);
}

void MainComponent::timerCallback()
{
    for (auto* strip : strips)
        strip->updateMeter();
    masterStrip.updateMeters();

    cpuLabel.setText ("CPU " + juce::String (juce::roundToInt (deviceManager.getCpuUsage() * 100.0)) + "%",
                      juce::dontSendNotification);
}

void MainComponent::changeListenerCallback (juce::ChangeBroadcaster*)
{
    updateDeviceInfo();
    saveDeviceState();
}

void MainComponent::updateDeviceInfo()
{
    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        const auto ins  = device->getActiveInputChannels().countNumberOfSetBits();
        const auto outs = device->getActiveOutputChannels().countNumberOfSetBits();
        deviceLabel.setText (device->getName()
                                 + "   " + juce::String (device->getCurrentSampleRate() / 1000.0, 1) + " kHz"
                                 + "   " + juce::String (device->getCurrentBufferSizeSamples()) + " samples"
                                 + "   " + juce::String (ins) + " in / " + juce::String (outs) + " out",
                             juce::dontSendNotification);
    }
    else
    {
        deviceLabel.setText ("No audio device. Click Audio Setup.", juce::dontSendNotification);
    }
}

void MainComponent::saveDeviceState()
{
    if (auto state = deviceManager.createStateXml())
    {
        settings.setValue (kDeviceStateKey, state.get());
        settings.saveIfNeeded();
    }
}

void MainComponent::showAudioSetup()
{
    auto selector = std::make_unique<juce::AudioDeviceSelectorComponent> (
        deviceManager, 0, kMaxHardwareIO, 0, kMaxHardwareIO, false, false, true, false);
    selector->setSize (520, 480);

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned (selector.release());
    options.dialogTitle = "Audio Setup";
    options.dialogBackgroundColour = Colours::panel;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.launchAsync();
}

} // namespace opendesk

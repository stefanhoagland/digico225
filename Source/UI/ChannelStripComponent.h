#pragma once

#include "Engine/MixerEngine.h"
#include "LevelMeter.h"

namespace opendesk
{

/** One input channel on screen: patch, gain, polarity, HPF, pan, mute, solo,
    meter and fader. Controls write straight into the engine's atomics. */
class ChannelStripComponent : public juce::Component
{
public:
    static constexpr int kWidth = 84;

    ChannelStripComponent (MixerEngine& engine, int channelIndex);

    void updateMeter();

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    ChannelParams& params;
    PeakMeter& meterSource;
    const int index;

    juce::Label nameLabel;
    juce::ComboBox inputBox;
    juce::Slider gainKnob, hpfKnob, panKnob, fader;
    juce::Label gainLabel, hpfLabel, panLabel;
    juce::TextButton polarityButton { juce::String (juce::CharPointer_UTF8 ("\xc3\x98")) };
    juce::TextButton hpfButton { "HPF" }, muteButton { "MUTE" }, soloButton { "SOLO" };
    LevelMeter meter;
    juce::Label faderReadout;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChannelStripComponent)
};

/** Shared helpers for strip components. */
void setupKnob (juce::Slider& knob, juce::Label& caption, const juce::String& captionText,
                double min, double max, double defaultValue, juce::Colour colour);
void setupToggle (juce::TextButton& button, juce::Colour onColour);
void setupFader (juce::Slider& fader, juce::Label& readout);
juce::String formatDb (double db);

} // namespace opendesk

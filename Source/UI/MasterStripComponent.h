#pragma once

#include "Engine/MixerEngine.h"
#include "LevelMeter.h"

namespace opendesk
{

/** Stereo master: output patch, mute, L/R meters and fader. */
class MasterStripComponent : public juce::Component
{
public:
    static constexpr int kWidth = 120;

    explicit MasterStripComponent (MixerEngine& engine);

    void updateMeters();

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    MasterParams& params;
    PeakMeter& meterSourceL;
    PeakMeter& meterSourceR;

    juce::Label title;
    juce::Label outLabel;
    juce::ComboBox outLeftBox, outRightBox;
    juce::TextButton muteButton { "MUTE" };
    juce::Slider fader;
    juce::Label faderReadout;
    LevelMeter meterL, meterR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasterStripComponent)
};

} // namespace opendesk

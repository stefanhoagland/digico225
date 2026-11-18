#include "MasterStripComponent.h"
#include "ChannelStripComponent.h"

namespace opendesk
{

MasterStripComponent::MasterStripComponent (MixerEngine& engine)
    : params (engine.master()),
      meterSourceL (engine.masterMeterL()),
      meterSourceR (engine.masterMeterR())
{
    title.setText ("MASTER", juce::dontSendNotification);
    title.setJustificationType (juce::Justification::centred);
    title.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    title.setColour (juce::Label::backgroundColourId, Colours::accent.withAlpha (0.25f));
    addAndMakeVisible (title);

    outLabel.setText ("OUTPUT L / R", juce::dontSendNotification);
    outLabel.setJustificationType (juce::Justification::centred);
    outLabel.setFont (juce::FontOptions (10.0f));
    outLabel.setColour (juce::Label::textColourId, Colours::textDim);
    addAndMakeVisible (outLabel);

    for (auto* box : { &outLeftBox, &outRightBox })
    {
        box->addItem ("--", 1);
        for (int i = 0; i < kMaxHardwareIO; ++i)
            box->addItem ((box == &outLeftBox ? "L: Out " : "R: Out ") + juce::String (i + 1), i + 2);
        addAndMakeVisible (*box);
    }
    outLeftBox.setSelectedId (params.outputLeft.load() + 2, juce::dontSendNotification);
    outRightBox.setSelectedId (params.outputRight.load() + 2, juce::dontSendNotification);
    outLeftBox.onChange  = [this] { params.outputLeft  = outLeftBox.getSelectedId() - 2; };
    outRightBox.onChange = [this] { params.outputRight = outRightBox.getSelectedId() - 2; };

    setupToggle (muteButton, Colours::mute);
    muteButton.onClick = [this] { params.mute = muteButton.getToggleState(); };
    addAndMakeVisible (muteButton);

    setupFader (fader, faderReadout);
    fader.onValueChange = [this]
    {
        params.faderDb = (float) fader.getValue();
        faderReadout.setText (formatDb (fader.getValue()), juce::dontSendNotification);
    };
    addAndMakeVisible (fader);
    addAndMakeVisible (faderReadout);
    addAndMakeVisible (meterL);
    addAndMakeVisible (meterR);
}

void MasterStripComponent::updateMeters()
{
    meterL.setPeak (meterSourceL.readAndReset());
    meterR.setPeak (meterSourceR.readAndReset());
}

void MasterStripComponent::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (2.0f, 0.0f);
    g.setColour (Colours::panel.brighter (0.05f));
    g.fillRoundedRectangle (b, 4.0f);
    g.setColour (Colours::accent.withAlpha (0.5f));
    g.drawRoundedRectangle (b, 4.0f, 1.0f);
}

void MasterStripComponent::resized()
{
    auto b = getLocalBounds().reduced (8, 6);

    title.setBounds (b.removeFromTop (22));
    b.removeFromTop (6);
    outLabel.setBounds (b.removeFromTop (14));
    outLeftBox.setBounds (b.removeFromTop (22));
    b.removeFromTop (3);
    outRightBox.setBounds (b.removeFromTop (22));
    b.removeFromTop (8);
    muteButton.setBounds (b.removeFromTop (24));
    b.removeFromTop (8);

    faderReadout.setBounds (b.removeFromBottom (18));
    meterR.setBounds (b.removeFromRight (10));
    b.removeFromRight (2);
    meterL.setBounds (b.removeFromRight (10));
    b.removeFromRight (2);
    fader.setBounds (b);
}

} // namespace opendesk

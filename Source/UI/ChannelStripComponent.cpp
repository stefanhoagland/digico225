#include "ChannelStripComponent.h"

namespace opendesk
{

juce::String formatDb (double db)
{
    if (db <= -89.5)
        return juce::String (juce::CharPointer_UTF8 ("-\xe2\x88\x9e"));   // -inf
    return (db > 0.05 ? "+" : "") + juce::String (db, 1);
}

void setupKnob (juce::Slider& knob, juce::Label& caption, const juce::String& captionText,
                double min, double max, double defaultValue, juce::Colour colour)
{
    knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    knob.setRange (min, max, 0.1);
    knob.setValue (defaultValue, juce::dontSendNotification);
    knob.setDoubleClickReturnValue (true, defaultValue);
    knob.setColour (juce::Slider::rotarySliderFillColourId, colour);
    knob.setPopupDisplayEnabled (true, true, nullptr);

    caption.setText (captionText, juce::dontSendNotification);
    caption.setJustificationType (juce::Justification::centred);
    caption.setFont (juce::FontOptions (10.0f));
    caption.setColour (juce::Label::textColourId, Colours::textDim);
}

void setupToggle (juce::TextButton& button, juce::Colour onColour)
{
    button.setClickingTogglesState (true);
    button.setColour (juce::TextButton::buttonOnColourId, onColour);
}

void setupFader (juce::Slider& fader, juce::Label& readout)
{
    fader.setSliderStyle (juce::Slider::LinearVertical);
    fader.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    fader.setRange (-90.0, 10.0, 0.1);
    fader.setSkewFactorFromMidPoint (-12.0);
    fader.setValue (0.0, juce::dontSendNotification);
    fader.setDoubleClickReturnValue (true, 0.0);

    readout.setJustificationType (juce::Justification::centred);
    readout.setFont (juce::FontOptions (11.0f));
    readout.setText (formatDb (0.0), juce::dontSendNotification);
}

ChannelStripComponent::ChannelStripComponent (MixerEngine& engine, int channelIndex)
    : params (engine.channel (channelIndex)),
      meterSource (engine.channelMeter (channelIndex)),
      index (channelIndex)
{
    // Name (double-click to rename)
    nameLabel.setText (juce::String (index + 1), juce::dontSendNotification);
    nameLabel.setJustificationType (juce::Justification::centred);
    nameLabel.setEditable (false, true, false);
    nameLabel.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    nameLabel.setColour (juce::Label::backgroundColourId, Colours::background);
    addAndMakeVisible (nameLabel);

    // Input patch
    inputBox.addItem ("--", 1);
    for (int i = 0; i < kMaxHardwareIO; ++i)
        inputBox.addItem ("In " + juce::String (i + 1), i + 2);
    inputBox.setSelectedId (params.inputSource.load() + 2, juce::dontSendNotification);
    inputBox.onChange = [this] { params.inputSource = inputBox.getSelectedId() - 2; };
    addAndMakeVisible (inputBox);

    // Gain and polarity
    setupKnob (gainKnob, gainLabel, "GAIN", -20.0, 60.0, 0.0, Colours::accent);
    gainKnob.setTextValueSuffix (" dB");
    gainKnob.onValueChange = [this] { params.gainDb = (float) gainKnob.getValue(); };
    addAndMakeVisible (gainKnob);
    addAndMakeVisible (gainLabel);

    setupToggle (polarityButton, Colours::accent);
    polarityButton.onClick = [this] { params.polarityInvert = polarityButton.getToggleState(); };
    addAndMakeVisible (polarityButton);

    // High-pass filter
    setupKnob (hpfKnob, hpfLabel, "HPF Hz", 20.0, 500.0, 80.0, Colours::hpf);
    hpfKnob.setSkewFactorFromMidPoint (100.0);
    hpfKnob.setRange (20.0, 500.0, 1.0);
    hpfKnob.onValueChange = [this] { params.hpfFreq = (float) hpfKnob.getValue(); };
    addAndMakeVisible (hpfKnob);
    addAndMakeVisible (hpfLabel);

    setupToggle (hpfButton, Colours::hpf);
    hpfButton.onClick = [this] { params.hpfOn = hpfButton.getToggleState(); };
    addAndMakeVisible (hpfButton);

    // Pan
    setupKnob (panKnob, panLabel, "PAN", -1.0, 1.0, 0.0, Colours::text);
    panKnob.setRange (-1.0, 1.0, 0.01);
    panKnob.textFromValueFunction = [] (double v)
    {
        if (std::abs (v) < 0.005) return juce::String ("C");
        return (v < 0 ? "L" : "R") + juce::String (juce::roundToInt (std::abs (v) * 100.0));
    };
    panKnob.onValueChange = [this] { params.pan = (float) panKnob.getValue(); };
    addAndMakeVisible (panKnob);
    addAndMakeVisible (panLabel);

    // Mute and solo
    setupToggle (muteButton, Colours::mute);
    muteButton.onClick = [this] { params.mute = muteButton.getToggleState(); };
    addAndMakeVisible (muteButton);

    setupToggle (soloButton, Colours::solo);
    soloButton.onClick = [this] { params.solo = soloButton.getToggleState(); };
    addAndMakeVisible (soloButton);

    // Meter and fader
    addAndMakeVisible (meter);
    setupFader (fader, faderReadout);
    fader.onValueChange = [this]
    {
        params.faderDb = (float) fader.getValue();
        faderReadout.setText (formatDb (fader.getValue()), juce::dontSendNotification);
    };
    addAndMakeVisible (fader);
    addAndMakeVisible (faderReadout);
}

void ChannelStripComponent::updateMeter()
{
    meter.setPeak (meterSource.readAndReset());
}

void ChannelStripComponent::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (2.0f, 0.0f);
    g.setColour (Colours::panel);
    g.fillRoundedRectangle (b, 4.0f);
    g.setColour (Colours::panelEdge);
    g.drawRoundedRectangle (b, 4.0f, 1.0f);
}

void ChannelStripComponent::resized()
{
    auto b = getLocalBounds().reduced (6, 6);

    nameLabel.setBounds (b.removeFromTop (22));
    b.removeFromTop (4);
    inputBox.setBounds (b.removeFromTop (22));
    b.removeFromTop (6);

    auto knobRow = [&b] (juce::Slider& knob, juce::Label& label, juce::Button& button)
    {
        auto row = b.removeFromTop (46);
        auto left = row.removeFromLeft (row.getWidth() / 2 + 4);
        label.setBounds (left.removeFromBottom (12));
        knob.setBounds (left);
        button.setBounds (row.withSizeKeepingCentre (row.getWidth() - 2, 20));
        b.removeFromTop (4);
    };

    knobRow (gainKnob, gainLabel, polarityButton);
    knobRow (hpfKnob, hpfLabel, hpfButton);

    auto panRow = b.removeFromTop (46);
    panLabel.setBounds (panRow.removeFromBottom (12));
    panKnob.setBounds (panRow);
    b.removeFromTop (6);

    muteButton.setBounds (b.removeFromTop (24));
    b.removeFromTop (4);
    soloButton.setBounds (b.removeFromTop (24));
    b.removeFromTop (8);

    faderReadout.setBounds (b.removeFromBottom (18));
    meter.setBounds (b.removeFromRight (10));
    b.removeFromRight (2);
    fader.setBounds (b);
}

} // namespace opendesk

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace opendesk
{

namespace Colours
{
    const juce::Colour background   { 0xff15171a };
    const juce::Colour panel        { 0xff23262b };
    const juce::Colour panelEdge    { 0xff32363d };
    const juce::Colour text         { 0xffd8dce2 };
    const juce::Colour textDim      { 0xff8a919b };
    const juce::Colour accent       { 0xff3fa7ff };
    const juce::Colour mute         { 0xffe0453a };
    const juce::Colour solo         { 0xfff2c230 };
    const juce::Colour hpf          { 0xff52c47a };
    const juce::Colour meterLow     { 0xff36c26a };
    const juce::Colour meterMid     { 0xffe8c33a };
    const juce::Colour meterHigh    { 0xffe5483b };
}

/** Dark console-style look for the whole app. */
class DeskLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DeskLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle, juce::Slider&) override;

    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
};

} // namespace opendesk

#include "DeskLookAndFeel.h"

namespace opendesk
{

DeskLookAndFeel::DeskLookAndFeel()
{
    setColourScheme ({ Colours::background, Colours::panel, Colours::panel,
                       Colours::panelEdge, Colours::text, Colours::accent,
                       juce::Colours::white, Colours::accent, Colours::text });

    setColour (juce::ResizableWindow::backgroundColourId, Colours::background);
    setColour (juce::TextButton::buttonColourId, Colours::panelEdge);
    setColour (juce::TextButton::textColourOffId, Colours::text);
    setColour (juce::TextButton::textColourOnId, juce::Colours::black);
    setColour (juce::Label::textColourId, Colours::text);
    setColour (juce::ComboBox::backgroundColourId, Colours::background);
    setColour (juce::ComboBox::outlineColourId, Colours::panelEdge);
    setColour (juce::ComboBox::textColourId, Colours::text);
    setColour (juce::Slider::textBoxTextColourId, Colours::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
}

void DeskLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float startAngle, float endAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (3.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle  = startAngle + sliderPos * (endAngle - startAngle);
    const auto colour = slider.findColour (juce::Slider::rotarySliderFillColourId);

    // Track
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour (Colours::background);
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Value arc, drawn from the centre for bipolar controls like pan.
    const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0
                         && std::abs (slider.getMinimum() + slider.getMaximum()) < 1.0e-6;
    const auto fromAngle = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;
    juce::Path arc;
    arc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, juce::jmin (fromAngle, angle), juce::jmax (fromAngle, angle), true);
    g.setColour (slider.isEnabled() ? colour : Colours::textDim);
    g.strokePath (arc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Knob and pointer
    g.setColour (Colours::panelEdge);
    g.fillEllipse (juce::Rectangle<float> (radius * 1.3f, radius * 1.3f).withCentre (centre));
    juce::Path pointer;
    pointer.addRoundedRectangle (-1.0f, -radius * 0.65f, 2.0f, radius * 0.45f, 1.0f);
    g.setColour (Colours::text);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));
}

void DeskLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float minSliderPos, float maxSliderPos,
                                        juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearVertical)
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        return;
    }

    // Fader slot
    const auto cx = (float) x + (float) width * 0.5f;
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (cx - 2.0f, (float) y, 4.0f, (float) height, 2.0f);

    // Fader cap
    const auto capW = juce::jmin (36.0f, (float) width - 4.0f);
    const auto capH = 34.0f;
    auto cap = juce::Rectangle<float> (capW, capH).withCentre ({ cx, sliderPos });
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6b717a), cap.getX(), cap.getY(),
                                             juce::Colour (0xff3b4047), cap.getX(), cap.getBottom(), false));
    g.fillRoundedRectangle (cap, 3.0f);
    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.fillRect (cap.withSizeKeepingCentre (capW - 6.0f, 2.0f));
}

juce::Font DeskLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return juce::Font (juce::FontOptions (juce::jmin (13.0f, (float) buttonHeight * 0.6f), juce::Font::bold));
}

} // namespace opendesk

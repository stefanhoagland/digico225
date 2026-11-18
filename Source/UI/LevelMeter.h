#pragma once

#include "DeskLookAndFeel.h"

namespace opendesk
{

/** Vertical peak meter with a short peak hold. Feed it linear peaks from a timer. */
class LevelMeter : public juce::Component
{
public:
    void setPeak (float linearPeak)
    {
        const auto db = juce::Decibels::gainToDecibels (linearPeak, kFloorDb);
        level = juce::jmax (db, level - kFallDbPerFrame);

        if (level >= hold) { hold = level; holdFrames = kHoldFrames; }
        else if (--holdFrames <= 0) hold = juce::jmax (kFloorDb, hold - kFallDbPerFrame);

        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (b, 2.0f);

        const auto inner = b.reduced (2.0f);
        const auto h = inner.getHeight() * proportion (level);
        auto bar = inner.withTop (inner.getBottom() - h);

        // Green up to -18 dB, amber around -6 dB, red at the top.
        juce::ColourGradient grad (Colours::meterLow, 0.0f, inner.getBottom(),
                                   Colours::meterHigh, 0.0f, inner.getY(), false);
        grad.addColour ((double) proportion (-18.0f), Colours::meterLow);
        grad.addColour ((double) proportion (-6.0f), Colours::meterMid);
        g.setGradientFill (grad);
        g.fillRect (bar);

        if (hold > kFloorDb + 1.0f)
        {
            const auto y = inner.getBottom() - inner.getHeight() * proportion (hold);
            g.setColour (hold > -0.5f ? Colours::meterHigh : Colours::text);
            g.fillRect (inner.getX(), y - 1.0f, inner.getWidth(), 2.0f);
        }
    }

private:
    static constexpr float kFloorDb = -60.0f;
    static constexpr float kFallDbPerFrame = 1.2f;   // at 30 fps, ~36 dB/s
    static constexpr int   kHoldFrames = 30;

    static float proportion (float db) { return juce::jlimit (0.0f, 1.0f, (db - kFloorDb) / -kFloorDb); }

    float level = kFloorDb, hold = kFloorDb;
    int holdFrames = 0;
};

} // namespace opendesk

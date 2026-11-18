#include "MixerEngine.h"

#include <cmath>

namespace opendesk
{

namespace
{
    constexpr double kSmoothingSeconds = 0.02;   // 20 ms ramps avoid zipper noise
    constexpr int    kMinScratchSize   = 4096;

    float dbToGain (float db) noexcept
    {
        return juce::Decibels::decibelsToGain (db, -90.0f);
    }

    /** Constant-power pan law: -3 dB at centre. */
    void panGains (float pan, float& left, float& right) noexcept
    {
        const auto angle = (juce::jlimit (-1.0f, 1.0f, pan) + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
        left  = std::cos (angle);
        right = std::sin (angle);
    }

    float peakOf (const float* data, int numSamples) noexcept
    {
        auto peak = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            peak = juce::jmax (peak, std::abs (data[i]));
        return peak;
    }
}

MixerEngine::MixerEngine()
{
    // Default patch: channel N listens to hardware input N.
    for (int i = 0; i < kNumChannels; ++i)
        channels[(size_t) i].inputSource.store (i);

    prepare (sampleRate, 512);
}

void MixerEngine::prepare (double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate > 0 ? newSampleRate : 48000.0;
    const auto size = (size_t) juce::jmax (kMinScratchSize, maxBlockSize);
    scratch.assign (size, 0.0f);
    busL.assign (size, 0.0f);
    busR.assign (size, 0.0f);

    for (auto& s : states)
    {
        s.preGain.reset (sampleRate, kSmoothingSeconds);
        s.gainL.reset (sampleRate, kSmoothingSeconds);
        s.gainR.reset (sampleRate, kSmoothingSeconds);
        s.hpf.reset();
        s.hpfFreqUsed = -1.0f;
        s.hpfActive = false;
    }

    masterGain.reset (sampleRate, kSmoothingSeconds);
}

void MixerEngine::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    if (device != nullptr)
        prepare (device->getCurrentSampleRate(), device->getCurrentBufferSizeSamples());
}

void MixerEngine::audioDeviceIOCallbackWithContext (const float* const* inputs, int numInputs,
                                                    float* const* outputs, int numOutputs, int numSamples,
                                                    const juce::AudioIODeviceCallbackContext&)
{
    process (inputs, numInputs, outputs, numOutputs, numSamples);
}

bool MixerEngine::anySoloActive() const noexcept
{
    for (auto& c : channels)
        if (c.solo.load (std::memory_order_relaxed))
            return true;
    return false;
}

void MixerEngine::process (const float* const* inputs, int numInputs,
                           float* const* outputs, int numOutputs, int numSamples) noexcept
{
    for (int ch = 0; ch < numOutputs; ++ch)
        if (outputs[ch] != nullptr)
            juce::FloatVectorOperations::clear (outputs[ch], numSamples);

    const int outL = masterParams.outputLeft.load();
    const int outR = masterParams.outputRight.load();
    float* destL = (outL >= 0 && outL < numOutputs) ? outputs[outL] : nullptr;
    float* destR = (outR >= 0 && outR < numOutputs) ? outputs[outR] : nullptr;

    const auto masterTarget = masterParams.mute.load() ? 0.0f : dbToGain (masterParams.faderDb.load());
    masterGain.setTargetValue (masterTarget);

    const int chunkSize = (int) scratch.size();

    for (int offset = 0; offset < numSamples; offset += chunkSize)
    {
        const int n = juce::jmin (chunkSize, numSamples - offset);
        processChunk (inputs, numInputs, offset, n);

        // Master fader
        for (int i = 0; i < n; ++i)
        {
            const auto g = masterGain.getNextValue();
            busL[(size_t) i] *= g;
            busR[(size_t) i] *= g;
        }

        masterMeters[0].push (peakOf (busL.data(), n));
        masterMeters[1].push (peakOf (busR.data(), n));

        if (destL != nullptr)
            juce::FloatVectorOperations::copy (destL + offset, busL.data(), n);
        if (destR != nullptr && destR != destL)
            juce::FloatVectorOperations::copy (destR + offset, busR.data(), n);
    }
}

void MixerEngine::processChunk (const float* const* inputs, int numInputs, int offset, int n) noexcept
{
    juce::FloatVectorOperations::clear (busL.data(), n);
    juce::FloatVectorOperations::clear (busR.data(), n);

    const bool soloActive = anySoloActive();

    for (int c = 0; c < kNumChannels; ++c)
    {
        auto& p = channels[(size_t) c];
        auto& s = states[(size_t) c];

        // Work out where this channel should be heading this block.
        const auto pre = dbToGain (p.gainDb.load()) * (p.polarityInvert.load() ? -1.0f : 1.0f);
        const bool audible = ! p.mute.load() && (! soloActive || p.solo.load());
        const auto fader = audible ? dbToGain (p.faderDb.load()) : 0.0f;
        float panL, panR;
        panGains (p.pan.load(), panL, panR);

        s.preGain.setTargetValue (pre);
        s.gainL.setTargetValue (fader * panL);
        s.gainR.setTargetValue (fader * panR);

        const int src = p.inputSource.load();
        const float* in = (src >= 0 && src < numInputs) ? inputs[src] : nullptr;

        if (in == nullptr)
        {
            // Unpatched or missing input: keep ramps moving so they don't jump later.
            s.preGain.skip (n);
            s.gainL.skip (n);
            s.gainR.skip (n);
            continue;
        }

        auto* buf = scratch.data();

        for (int i = 0; i < n; ++i)
            buf[i] = in[offset + i] * s.preGain.getNextValue();

        // High-pass filter
        const bool hpfOn = p.hpfOn.load();
        if (hpfOn)
        {
            const auto freq = juce::jlimit (10.0f, 1000.0f, p.hpfFreq.load());
            if (! s.hpfActive || ! juce::exactlyEqual (freq, s.hpfFreqUsed))
            {
                s.hpf.setCoefficients (juce::IIRCoefficients::makeHighPass (sampleRate, freq));
                if (! s.hpfActive)
                    s.hpf.reset();
                s.hpfFreqUsed = freq;
            }
            s.hpf.processSamples (buf, n);
        }
        s.hpfActive = hpfOn;

        // Fader, mute and pan into the master bus. The meter shows post-fader mono level.
        auto peak = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            const auto gl = s.gainL.getNextValue();
            const auto gr = s.gainR.getNextValue();
            busL[(size_t) i] += buf[i] * gl;
            busR[(size_t) i] += buf[i] * gr;
            peak = juce::jmax (peak, std::abs (buf[i]) * (gl + gr) * 0.70710678f);
        }
        channelMeters[(size_t) c].push (peak);
    }
}

} // namespace opendesk

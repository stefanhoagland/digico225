#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>

#include <array>
#include <atomic>
#include <vector>

namespace opendesk
{

/** Number of input channels in this build. The spec's session-structure
    screen will make this configurable later. */
constexpr int kNumChannels = 48;

/** Highest hardware input/output index offered in the patch. */
constexpr int kMaxHardwareIO = 64;

/** Settings for one input channel. Written by the UI thread, read by the
    audio thread, so every field is atomic. */
struct ChannelParams
{
    std::atomic<int>   inputSource    { -1 };   // hardware input index, -1 = unpatched
    std::atomic<float> gainDb         { 0.0f }; // input gain / trim
    std::atomic<bool>  polarityInvert { false };
    std::atomic<bool>  hpfOn          { false };
    std::atomic<float> hpfFreq        { 80.0f };
    std::atomic<float> pan            { 0.0f }; // -1 (left) .. +1 (right)
    std::atomic<float> faderDb        { 0.0f };
    std::atomic<bool>  mute           { false };
    std::atomic<bool>  solo           { false };
};

struct MasterParams
{
    std::atomic<float> faderDb     { 0.0f };
    std::atomic<bool>  mute        { false };
    std::atomic<int>   outputLeft  { 0 };      // hardware output index
    std::atomic<int>   outputRight { 1 };
};

/** Peak meter shared between threads. The audio thread pushes peaks, the UI
    reads and clears them on each repaint. */
struct PeakMeter
{
    std::atomic<float> peak { 0.0f };

    void push (float value) noexcept
    {
        auto prev = peak.load (std::memory_order_relaxed);
        while (value > prev && ! peak.compare_exchange_weak (prev, value, std::memory_order_relaxed)) {}
    }

    float readAndReset() noexcept { return peak.exchange (0.0f, std::memory_order_relaxed); }
};

/** The mixing engine: hardware inputs -> channel strips -> stereo master ->
    hardware outputs. Runs as the audio device callback. */
class MixerEngine : public juce::AudioIODeviceCallback
{
public:
    MixerEngine();

    /** Call before processing when not driven by a real device (tests). */
    void prepare (double sampleRate, int maxBlockSize);

    /** Process one block. Same signature as the device callback minus context. */
    void process (const float* const* inputs, int numInputs,
                  float* const* outputs, int numOutputs, int numSamples) noexcept;

    ChannelParams& channel (int index)         { return channels[(size_t) index]; }
    PeakMeter&     channelMeter (int index)    { return channelMeters[(size_t) index]; }
    MasterParams&  master()                    { return masterParams; }
    PeakMeter&     masterMeterL()              { return masterMeters[0]; }
    PeakMeter&     masterMeterR()              { return masterMeters[1]; }

    bool anySoloActive() const noexcept;

    // juce::AudioIODeviceCallback
    void audioDeviceIOCallbackWithContext (const float* const* inputs, int numInputs,
                                           float* const* outputs, int numOutputs, int numSamples,
                                           const juce::AudioIODeviceCallbackContext&) override;
    void audioDeviceAboutToStart (juce::AudioIODevice* device) override;
    void audioDeviceStopped() override {}

private:
    struct ChannelState
    {
        juce::SmoothedValue<float> preGain { 1.0f };  // gain trim * polarity
        juce::SmoothedValue<float> gainL   { 0.0f };  // fader * mute * pan
        juce::SmoothedValue<float> gainR   { 0.0f };
        juce::IIRFilter hpf;
        bool  hpfActive   = false;
        float hpfFreqUsed = -1.0f;
    };

    void processChunk (const float* const* inputs, int numInputs, int offset, int numSamples) noexcept;

    std::array<ChannelParams, kNumChannels> channels;
    std::array<PeakMeter, kNumChannels>     channelMeters;
    std::array<ChannelState, kNumChannels>  states;

    MasterParams masterParams;
    std::array<PeakMeter, 2> masterMeters;
    juce::SmoothedValue<float> masterGain { 1.0f };

    double sampleRate = 48000.0;
    std::vector<float> scratch;            // one channel's working buffer
    std::vector<float> busL, busR;         // master bus

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerEngine)
};

} // namespace opendesk

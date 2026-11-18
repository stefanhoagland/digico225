// Runs the mixer engine on fake audio buffers to prove audio passes through
// and the basic controls behave. No audio hardware needed.

#include "Engine/MixerEngine.h"

#include <cmath>
#include <cstdio>
#include <vector>

using opendesk::MixerEngine;

namespace
{
    int failures = 0;

    void expectNear (const char* what, float actual, float expected, float tolerance = 1.0e-3f)
    {
        const bool ok = std::abs (actual - expected) <= tolerance;
        std::printf ("%s %-50s got %.5f, expected %.5f\n", ok ? "PASS" : "FAIL", what, actual, expected);
        if (! ok)
            ++failures;
    }

    struct Rig
    {
        static constexpr int numIn = 8, numOut = 2, block = 256;
        std::vector<std::vector<float>> in  = std::vector<std::vector<float>> (numIn, std::vector<float> (block, 0.0f));
        std::vector<std::vector<float>> out = std::vector<std::vector<float>> (numOut, std::vector<float> (block, 0.0f));
        std::vector<const float*> inPtrs;
        std::vector<float*> outPtrs;
        MixerEngine engine;

        Rig()
        {
            for (auto& c : in)  inPtrs.push_back (c.data());
            for (auto& c : out) outPtrs.push_back (c.data());
            engine.prepare (48000.0, block);

            // Start with every channel's fader down so tests control what's audible.
            for (int i = 0; i < opendesk::kNumChannels; ++i)
                engine.channel (i).faderDb = -90.0f;
        }

        void setInputDC (int channel, float value)
        {
            std::fill (in[(size_t) channel].begin(), in[(size_t) channel].end(), value);
        }

        /** Run enough blocks for all 20 ms ramps to settle, then return the last sample of each output. */
        void run (float& left, float& right)
        {
            for (int b = 0; b < 20; ++b)
                engine.process (inPtrs.data(), numIn, outPtrs.data(), numOut, block);
            left  = out[0].back();
            right = out[1].back();
        }
    };
}

int main()
{
    const float centre = std::sqrt (0.5f);   // -3 dB constant-power pan
    float l = 0, r = 0;

    {
        Rig rig;
        rig.setInputDC (0, 0.5f);
        rig.engine.channel (0).faderDb = 0.0f;
        rig.run (l, r);
        expectNear ("input 1 passes to master L at unity, centre pan", l, 0.5f * centre);
        expectNear ("input 1 passes to master R at unity, centre pan", r, 0.5f * centre);

        rig.engine.channel (0).pan = -1.0f;
        rig.run (l, r);
        expectNear ("hard left pan: left output", l, 0.5f);
        expectNear ("hard left pan: right output", r, 0.0f);

        rig.engine.channel (0).pan = 0.0f;
        rig.engine.channel (0).gainDb = 6.0f;
        rig.run (l, r);
        expectNear ("+6 dB gain", l, 0.5f * centre * juce::Decibels::decibelsToGain (6.0f));

        rig.engine.channel (0).gainDb = 0.0f;
        rig.engine.channel (0).polarityInvert = true;
        rig.run (l, r);
        expectNear ("polarity invert", l, -0.5f * centre);
        rig.engine.channel (0).polarityInvert = false;

        rig.engine.channel (0).mute = true;
        rig.run (l, r);
        expectNear ("channel mute silences output", l, 0.0f);
        rig.engine.channel (0).mute = false;

        rig.engine.master().mute = true;
        rig.run (l, r);
        expectNear ("master mute silences output", l, 0.0f);
        rig.engine.master().mute = false;

        rig.engine.channel (0).hpfOn = true;
        rig.run (l, r);
        expectNear ("high-pass filter removes DC", l, 0.0f, 1.0e-2f);
        rig.engine.channel (0).hpfOn = false;
    }

    {
        Rig rig;
        rig.setInputDC (0, 0.5f);
        rig.setInputDC (1, 0.25f);
        rig.engine.channel (0).faderDb = 0.0f;
        rig.engine.channel (1).faderDb = 0.0f;
        rig.run (l, r);
        expectNear ("two channels sum on the master bus", l, 0.75f * centre);

        rig.engine.channel (1).solo = true;
        rig.run (l, r);
        expectNear ("solo channel 2 leaves only channel 2", l, 0.25f * centre);
        rig.engine.channel (1).solo = false;

        rig.engine.channel (0).inputSource = 1;
        rig.run (l, r);
        expectNear ("re-patch channel 1 to input 2", l, 0.5f * centre);

        rig.engine.channel (0).inputSource = -1;
        rig.run (l, r);
        expectNear ("unpatched channel is silent", l, 0.25f * centre);
    }

    {
        Rig rig;
        rig.setInputDC (0, 0.5f);
        rig.engine.channel (0).faderDb = 0.0f;
        rig.run (l, r);
        const auto meter = rig.engine.channelMeter (0).readAndReset();
        expectNear ("channel meter reads mono post-fader level", meter, 0.5f, 1.0e-2f);
        const auto masterMeter = rig.engine.masterMeterL().readAndReset();
        expectNear ("master meter L", masterMeter, 0.5f * centre, 1.0e-2f);
    }

    std::printf ("\n%s (%d failure%s)\n", failures == 0 ? "ALL PASSED" : "FAILED", failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}

#pragma once

#include <juce_dsp/juce_dsp.h>
#include <cmath>

/**
    Wow & flutter pitch instability.

    Models the mechanical speed variations of a turntable / tape transport as a
    fractional, time-varying delay line. Three modulation sources are summed:

      - "Wow"     : a slow sine (~0.6 Hz) for the long, drifting pitch sag.
      - "Flutter" : a faster sine (~7 Hz) for the shimmering, nervous warble.
      - "Drift"   : a slewed random walk that keeps the motion from sounding
                    perfectly periodic.

    The two channels are modulated with a small phase offset so the warble
    decorrelates slightly across the stereo field, the way a real record does.
*/
class WowFlutter
{
public:
    WowFlutter() = default;

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        numChannels = static_cast<int> (spec.numChannels);

        const auto maxDelaySamples =
            static_cast<int> ((baseDelayMs + maxWowMs + maxFlutterMs + 4.0) * 0.001 * sampleRate) + 4;

        delay.setMaximumDelayInSamples (maxDelaySamples);
        delay.prepare (spec);

        // Per-channel random drift state.
        drift.assign (static_cast<size_t> (juce::jmax (1, numChannels)), 0.0f);
        driftTarget = drift;

        reset();
    }

    void reset()
    {
        delay.reset();
        wowPhase = 0.0f;
        flutterPhase = 0.0f;
        driftCounter = 0;
        std::fill (drift.begin(), drift.end(), 0.0f);
        std::fill (driftTarget.begin(), driftTarget.end(), 0.0f);
    }

    /** @param wowAmount     0..1 amount of slow drift. */
    /** @param flutterAmount 0..1 amount of fast warble. */
    void setDepth (float wowAmount, float flutterAmount) noexcept
    {
        wowDepthMs     = juce::jlimit (0.0f, 1.0f, wowAmount)     * maxWowMs;
        flutterDepthMs = juce::jlimit (0.0f, 1.0f, flutterAmount) * maxFlutterMs;
    }

    /** Advances the shared modulators by one sample. Call once per frame
        before reading channels. */
    void updateModulation() noexcept
    {
        const auto wowInc     = juce::MathConstants<float>::twoPi * wowRateHz     / static_cast<float> (sampleRate);
        const auto flutterInc = juce::MathConstants<float>::twoPi * flutterRateHz / static_cast<float> (sampleRate);

        wowPhase += wowInc;
        if (wowPhase >= juce::MathConstants<float>::twoPi)
            wowPhase -= juce::MathConstants<float>::twoPi;

        flutterPhase += flutterInc;
        if (flutterPhase >= juce::MathConstants<float>::twoPi)
            flutterPhase -= juce::MathConstants<float>::twoPi;

        // Pick a fresh random drift target a few times per second, then slew
        // toward it so the random component stays smooth (no zipper noise).
        if (--driftCounter <= 0)
        {
            driftCounter = static_cast<int> (sampleRate * 0.20); // ~5 Hz update
            for (auto& t : driftTarget)
                t = random.nextFloat() * 2.0f - 1.0f;
        }

        const float slew = 1.0f / (0.05f * static_cast<float> (sampleRate)); // ~50 ms
        for (size_t i = 0; i < drift.size(); ++i)
            drift[i] += (driftTarget[i] - drift[i]) * slew;
    }

    float processSample (int channel, float input) noexcept
    {
        const auto chOffset = (channel == 0 ? 0.0f : channelPhaseOffset);

        const float wow     = std::sin (wowPhase + chOffset);
        const float flutter = std::sin (flutterPhase + chOffset * 3.0f);
        const float d       = drift[static_cast<size_t> (juce::jlimit (0, numChannels - 1, channel))];

        // Combined delay in milliseconds → samples.
        float delayMs = baseDelayMs
                      + wow     * wowDepthMs
                      + flutter * flutterDepthMs
                      + d       * (wowDepthMs * 0.5f);

        delayMs = juce::jlimit (1.0f, baseDelayMs + maxWowMs + maxFlutterMs + 3.0f, delayMs);

        const float delaySamples = delayMs * 0.001f * static_cast<float> (sampleRate);

        delay.pushSample (channel, input);
        return delay.popSample (channel, delaySamples, true);
    }

private:
    using Delay = juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd>;

    Delay delay;
    juce::Random random;

    double sampleRate = 44100.0;
    int numChannels = 2;

    // Modulator state.
    float wowPhase = 0.0f;
    float flutterPhase = 0.0f;
    std::vector<float> drift, driftTarget;
    int driftCounter = 0;

    // Tuning constants.
    static constexpr float baseDelayMs = 30.0f;
    static constexpr float maxWowMs = 7.0f;
    static constexpr float maxFlutterMs = 1.6f;
    static constexpr float wowRateHz = 0.6f;
    static constexpr float flutterRateHz = 7.0f;
    static constexpr float channelPhaseOffset = 0.6f;

    float wowDepthMs = 0.0f;
    float flutterDepthMs = 0.0f;
};

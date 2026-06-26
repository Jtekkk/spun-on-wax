#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cmath>

/**
    Surface noise of a record: a continuous bed of hiss plus randomly
    triggered crackle/pops.

      - Hiss    : white noise gently low-passed (a one-pole) so it sits as a
                  warm bed rather than a bright fizz.
      - Crackle : sparse impulses with exponential decay. The trigger
                  probability scales with the "amount" so the surface goes from
                  the occasional tick to a well-worn, dusty groove.

    Each channel is generated independently so the noise is decorrelated and
    spreads naturally in stereo.
*/
class VinylNoise
{
public:
    VinylNoise() = default;

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        const auto channels = static_cast<size_t> (juce::jmax (1u, spec.numChannels));

        hissState.assign (channels, 0.0f);
        crackleValue.assign (channels, 0.0f);

        // One-pole low-pass coefficient for the hiss (~6 kHz).
        const double cutoff = 6000.0;
        const double x = std::exp (-2.0 * juce::MathConstants<double>::pi * cutoff / sampleRate);
        hissCoeff = static_cast<float> (x);

        // Decay applied to each crackle impulse (~2 ms time constant).
        crackleDecay = std::exp (-1.0f / (0.002f * static_cast<float> (sampleRate)));
    }

    void reset()
    {
        std::fill (hissState.begin(), hissState.end(), 0.0f);
        std::fill (crackleValue.begin(), crackleValue.end(), 0.0f);
    }

    /** @param hiss    0..1 level of the continuous noise bed. */
    /** @param crackle 0..1 density/level of the pops. */
    void setAmount (float hiss, float crackle) noexcept
    {
        hissLevel = juce::jlimit (0.0f, 1.0f, hiss);
        crackleAmount = juce::jlimit (0.0f, 1.0f, crackle);
    }

    /** Returns one sample of surface noise for the given channel. */
    float processSample (int channel) noexcept
    {
        const auto ch = static_cast<size_t> (juce::jlimit (0, static_cast<int> (hissState.size()) - 1, channel));

        // --- Hiss --------------------------------------------------------
        const float white = random.nextFloat() * 2.0f - 1.0f;
        hissState[ch] = white + hissCoeff * (hissState[ch] - white);
        const float hiss = hissState[ch] * hissLevel * hissGain;

        // --- Crackle -----------------------------------------------------
        // Probability of a new pop this sample, scaled by amount^2 for a
        // musical taper, normalised against sample rate.
        const float density = crackleAmount * crackleAmount * 0.04f;
        const float threshold = density * (44100.0f / static_cast<float> (sampleRate));

        if (random.nextFloat() < threshold)
        {
            const float sign = (random.nextBool() ? 1.0f : -1.0f);
            // Mostly small ticks with the occasional louder pop.
            const float mag = random.nextFloat();
            const float amp = (mag > 0.92f ? 1.0f : mag * 0.5f);
            crackleValue[ch] = sign * amp;
        }

        const float crackle = crackleValue[ch] * crackleGain;
        crackleValue[ch] *= crackleDecay;

        return hiss + crackle;
    }

private:
    juce::Random random;
    double sampleRate = 44100.0;

    std::vector<float> hissState;
    std::vector<float> crackleValue;

    float hissCoeff = 0.0f;
    float crackleDecay = 0.0f;

    float hissLevel = 0.0f;
    float crackleAmount = 0.0f;

    // Overall trim so the noise sits under the program material at unity.
    static constexpr float hissGain = 0.10f;
    static constexpr float crackleGain = 0.45f;
};

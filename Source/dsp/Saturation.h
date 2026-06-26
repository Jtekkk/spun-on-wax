#pragma once

#include <juce_core/juce_core.h>
#include <cmath>

/**
    Soft, asymmetric saturation for the worn-needle "grit".

    A drive control pushes the signal into a tanh curve; a touch of asymmetry
    adds even harmonics for warmth. Output is trimmed so increasing drive does
    not run away in level.
*/
class Saturation
{
public:
    /** @param amount 0..1, mapped internally to a useful drive range. */
    void setDrive (float amount) noexcept
    {
        const float a = juce::jlimit (0.0f, 1.0f, amount);
        drive = 1.0f + a * 11.0f;          // 1x .. 12x
        makeup = 1.0f / std::tanh (drive); // keep peaks roughly in check
        bias = a * 0.10f;                  // a little even-harmonic asymmetry
    }

    float processSample (float x) const noexcept
    {
        return std::tanh (drive * x + bias) * makeup - std::tanh (bias) * makeup;
    }

private:
    float drive = 1.0f;
    float makeup = 1.0f / std::tanh (1.0f);
    float bias = 0.0f;
};

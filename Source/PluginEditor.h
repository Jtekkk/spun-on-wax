#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

/** Dark, vinyl-themed look: warm amber knobs on a charcoal background. */
class VinylLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VinylLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;
};

class SpunOnWaxAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SpunOnWaxAudioProcessorEditor (SpunOnWaxAudioProcessor&);
    ~SpunOnWaxAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    Knob& addKnob (const juce::String& paramID, const juce::String& text);

    SpunOnWaxAudioProcessor& processor;
    VinylLookAndFeel lookAndFeel;

    std::vector<std::unique_ptr<Knob>> knobs;

    // Rotation of the record-glyph marker in the header.
    float discAngle = -0.5f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpunOnWaxAudioProcessorEditor)
};

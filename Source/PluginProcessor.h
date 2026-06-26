#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "dsp/WowFlutter.h"
#include "dsp/VinylNoise.h"
#include "dsp/Saturation.h"

/**
    Spun on Wax — a vinyl / lo-fi colouring effect.

    Signal flow (per channel, wet path):
        input → wow & flutter → saturation → tone (age) low-pass → + surface noise
    The wet path is then stereo-widened and blended with the dry signal, and a
    final output trim is applied.
*/
class SpunOnWaxAudioProcessor : public juce::AudioProcessor
{
public:
    SpunOnWaxAudioProcessor();
    ~SpunOnWaxAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept { return apvts; }

    // Parameter identifiers, shared with the editor.
    struct ParamID
    {
        static constexpr const char* wow     = "wow";
        static constexpr const char* flutter = "flutter";
        static constexpr const char* hiss    = "hiss";
        static constexpr const char* crackle = "crackle";
        static constexpr const char* drive   = "drive";
        static constexpr const char* tone    = "tone";
        static constexpr const char* width   = "width";
        static constexpr const char* mix     = "mix";
        static constexpr const char* output  = "output";
    };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;

    // Cached atomic parameter pointers (read on the audio thread).
    std::atomic<float>* wowParam     = nullptr;
    std::atomic<float>* flutterParam = nullptr;
    std::atomic<float>* hissParam    = nullptr;
    std::atomic<float>* crackleParam = nullptr;
    std::atomic<float>* driveParam   = nullptr;
    std::atomic<float>* toneParam    = nullptr;
    std::atomic<float>* widthParam   = nullptr;
    std::atomic<float>* mixParam     = nullptr;
    std::atomic<float>* outputParam  = nullptr;

    // DSP blocks.
    WowFlutter wowFlutter;
    VinylNoise vinylNoise;
    Saturation saturation;
    juce::dsp::StateVariableTPTFilter<float> toneFilter;

    // Smoothed control values.
    juce::SmoothedValue<float> mixSmoothed;
    juce::SmoothedValue<float> widthSmoothed;
    juce::SmoothedValue<float> outputSmoothed;

    juce::AudioBuffer<float> dryBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpunOnWaxAudioProcessor)
};

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    // Tone control maps 0..1 → low-pass cutoff. At 1 the high end is open
    // (bright, new pressing); toward 0 the record sounds dull and worn.
    float toneToCutoff (float t) noexcept
    {
        const float norm = juce::jlimit (0.0f, 1.0f, t);
        // Exponential sweep from ~1.2 kHz up to ~18 kHz.
        return 1200.0f * std::pow (18000.0f / 1200.0f, norm);
    }
}

SpunOnWaxAudioProcessor::SpunOnWaxAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    wowParam     = apvts.getRawParameterValue (ParamID::wow);
    flutterParam = apvts.getRawParameterValue (ParamID::flutter);
    hissParam    = apvts.getRawParameterValue (ParamID::hiss);
    crackleParam = apvts.getRawParameterValue (ParamID::crackle);
    driveParam   = apvts.getRawParameterValue (ParamID::drive);
    toneParam    = apvts.getRawParameterValue (ParamID::tone);
    widthParam   = apvts.getRawParameterValue (ParamID::width);
    mixParam     = apvts.getRawParameterValue (ParamID::mix);
    outputParam  = apvts.getRawParameterValue (ParamID::output);
}

juce::AudioProcessorValueTreeState::ParameterLayout SpunOnWaxAudioProcessor::createParameterLayout()
{
    using Range = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto percent = [] (float v) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; };

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::wow, 1 }, "Wow",
        Range (0.0f, 1.0f, 0.001f), 0.25f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([percent] (float v, int) { return percent (v); })));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::flutter, 1 }, "Flutter",
        Range (0.0f, 1.0f, 0.001f), 0.20f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([percent] (float v, int) { return percent (v); })));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::hiss, 1 }, "Hiss",
        Range (0.0f, 1.0f, 0.001f), 0.20f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([percent] (float v, int) { return percent (v); })));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::crackle, 1 }, "Crackle",
        Range (0.0f, 1.0f, 0.001f), 0.30f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([percent] (float v, int) { return percent (v); })));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::drive, 1 }, "Drive",
        Range (0.0f, 1.0f, 0.001f), 0.30f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([percent] (float v, int) { return percent (v); })));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::tone, 1 }, "Tone",
        Range (0.0f, 1.0f, 0.001f), 0.65f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (
            [] (float v, int) { return juce::String (juce::roundToInt (toneToCutoff (v))) + " Hz"; })));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::width, 1 }, "Width",
        Range (0.0f, 2.0f, 0.001f), 1.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (
            [] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; })));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::mix, 1 }, "Mix",
        Range (0.0f, 1.0f, 0.001f), 1.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([percent] (float v, int) { return percent (v); })));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::output, 1 }, "Output",
        Range (-24.0f, 12.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (
            [] (float v, int) { return juce::String (v, 1) + " dB"; })));

    return { params.begin(), params.end() };
}

void SpunOnWaxAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32> (juce::jmax (1, getTotalNumOutputChannels()));

    wowFlutter.prepare (spec);
    vinylNoise.prepare (spec);

    toneFilter.prepare (spec);
    toneFilter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    toneFilter.setCutoffFrequency (toneToCutoff (toneParam->load()));

    const double smoothSeconds = 0.02;
    mixSmoothed.reset (sampleRate, smoothSeconds);
    widthSmoothed.reset (sampleRate, smoothSeconds);
    outputSmoothed.reset (sampleRate, smoothSeconds);

    mixSmoothed.setCurrentAndTargetValue (mixParam->load());
    widthSmoothed.setCurrentAndTargetValue (widthParam->load());
    outputSmoothed.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outputParam->load()));

    dryBuffer.setSize (getTotalNumInputChannels(), samplesPerBlock, false, false, true);
}

void SpunOnWaxAudioProcessor::releaseResources()
{
    wowFlutter.reset();
    vinylNoise.reset();
    toneFilter.reset();
}

bool SpunOnWaxAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& main = layouts.getMainOutputChannelSet();

    if (main != juce::AudioChannelSet::mono() && main != juce::AudioChannelSet::stereo())
        return false;

    // Input and output layouts must match.
    return layouts.getMainInputChannelSet() == main;
}

void SpunOnWaxAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    // Clear any output channels that don't carry input.
    for (int ch = getTotalNumInputChannels(); ch < numChannels; ++ch)
        buffer.clear (ch, 0, numSamples);

    // --- Pull current parameter values -----------------------------------
    wowFlutter.setDepth (wowParam->load(), flutterParam->load());
    vinylNoise.setAmount (hissParam->load(), crackleParam->load());
    saturation.setDrive (driveParam->load());
    toneFilter.setCutoffFrequency (toneToCutoff (toneParam->load()));

    mixSmoothed.setTargetValue (mixParam->load());
    widthSmoothed.setTargetValue (widthParam->load());
    outputSmoothed.setTargetValue (juce::Decibels::decibelsToGain (outputParam->load()));

    // --- Keep a dry copy for the wet/dry blend ---------------------------
    dryBuffer.makeCopyOf (buffer, true);

    // --- Wet path: wow/flutter → saturation → tone → noise ---------------
    for (int n = 0; n < numSamples; ++n)
    {
        wowFlutter.updateModulation();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float x = buffer.getSample (ch, n);
            x = wowFlutter.processSample (ch, x);
            x = saturation.processSample (x);
            x = toneFilter.processSample (ch, x);
            x += vinylNoise.processSample (ch);
            buffer.setSample (ch, n, x);
        }
    }

    // --- Stereo width (mid/side) on the wet signal -----------------------
    if (numChannels == 2)
    {
        auto* left  = buffer.getWritePointer (0);
        auto* right = buffer.getWritePointer (1);

        for (int n = 0; n < numSamples; ++n)
        {
            const float width = widthSmoothed.getNextValue();
            const float mid  = 0.5f * (left[n] + right[n]);
            const float side = 0.5f * (left[n] - right[n]) * width;
            left[n]  = mid + side;
            right[n] = mid - side;
        }
    }
    else
    {
        widthSmoothed.skip (numSamples);
    }

    // --- Wet/dry blend + output trim -------------------------------------
    for (int n = 0; n < numSamples; ++n)
    {
        const float mix = mixSmoothed.getNextValue();
        const float gain = outputSmoothed.getNextValue();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float dry = (ch < dryBuffer.getNumChannels() ? dryBuffer.getSample (ch, n) : 0.0f);
            const float wet = buffer.getSample (ch, n);
            buffer.setSample (ch, n, (dry * (1.0f - mix) + wet * mix) * gain);
        }
    }
}

juce::AudioProcessorEditor* SpunOnWaxAudioProcessor::createEditor()
{
    return new SpunOnWaxAudioProcessorEditor (*this);
}

void SpunOnWaxAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        std::unique_ptr<juce::XmlElement> xml (state.createXml());
        copyXmlToBinary (*xml, destData);
    }
}

void SpunOnWaxAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpunOnWaxAudioProcessor();
}

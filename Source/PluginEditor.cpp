#include "PluginEditor.h"

//==============================================================================
namespace Palette
{
    const juce::Colour background { 0xff1a1614 };
    const juce::Colour panel      { 0xff241f1c };
    const juce::Colour amber      { 0xffe0a060 };
    const juce::Colour amberDim   { 0xff6b5238 };
    const juce::Colour text       { 0xffd8cfc6 };
    const juce::Colour vinyl      { 0xff0d0b0a };
}

//==============================================================================
VinylLookAndFeel::VinylLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, Palette::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, Palette::text);
}

void VinylLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                         juce::Slider&)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (6.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle  = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto lineW  = juce::jmax (2.0f, radius * 0.12f);

    // Track arc.
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (Palette::amberDim);
    g.strokePath (track, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Value arc.
    juce::Path value;
    value.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                         rotaryStartAngle, angle, true);
    g.setColour (Palette::amber);
    g.strokePath (value, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Knob body (a little record).
    const auto knobRadius = radius * 0.62f;
    g.setColour (Palette::vinyl);
    g.fillEllipse (centre.x - knobRadius, centre.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);
    g.setColour (Palette::amberDim.withAlpha (0.4f));
    g.drawEllipse (centre.x - knobRadius, centre.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f, 1.0f);

    // Pointer.
    juce::Path pointer;
    const auto pointerLength = knobRadius * 0.85f;
    pointer.addRoundedRectangle (-lineW * 0.4f, -pointerLength, lineW * 0.8f, pointerLength, lineW * 0.4f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
    g.setColour (Palette::amber);
    g.fillPath (pointer);
}

//==============================================================================
SpunOnWaxAudioProcessorEditor::SpunOnWaxAudioProcessorEditor (SpunOnWaxAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);

    using ID = SpunOnWaxAudioProcessor::ParamID;
    addKnob (ID::wow,     "WOW");
    addKnob (ID::flutter, "FLUTTER");
    addKnob (ID::crackle, "CRACKLE");
    addKnob (ID::hiss,    "HISS");
    addKnob (ID::drive,   "DRIVE");
    addKnob (ID::tone,    "TONE");
    addKnob (ID::width,   "WIDTH");
    addKnob (ID::mix,     "MIX");
    addKnob (ID::output,  "OUTPUT");

    setSize (560, 320);
}

SpunOnWaxAudioProcessorEditor::~SpunOnWaxAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

SpunOnWaxAudioProcessorEditor::Knob& SpunOnWaxAudioProcessorEditor::addKnob (const juce::String& paramID,
                                                                            const juce::String& text)
{
    auto knob = std::make_unique<Knob>();

    knob->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    addAndMakeVisible (knob->slider);

    knob->label.setText (text, juce::dontSendNotification);
    knob->label.setJustificationType (juce::Justification::centred);
    knob->label.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    addAndMakeVisible (knob->label);

    knob->attachment = std::make_unique<SliderAttachment> (processor.getValueTreeState(), paramID, knob->slider);

    knobs.push_back (std::move (knob));
    return *knobs.back();
}

void SpunOnWaxAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (Palette::background);

    auto header = getLocalBounds().removeFromTop (54).toFloat();

    // Spinning record glyph on the left of the header.
    {
        const float r = 18.0f;
        const auto centre = juce::Point<float> (header.getX() + 32.0f, header.getCentreY());
        g.setColour (Palette::vinyl);
        g.fillEllipse (centre.x - r, centre.y - r, r * 2.0f, r * 2.0f);
        g.setColour (Palette::amberDim.withAlpha (0.5f));
        for (float rr = r * 0.45f; rr < r; rr += 3.5f)
            g.drawEllipse (centre.x - rr, centre.y - rr, rr * 2.0f, rr * 2.0f, 0.6f);
        // Label spot + a marker so the spin reads.
        g.setColour (Palette::amber);
        g.fillEllipse (centre.x - r * 0.28f, centre.y - r * 0.28f, r * 0.56f, r * 0.56f);
        juce::Path marker;
        marker.addRectangle (-1.0f, -r, 2.0f, r * 0.5f);
        marker.applyTransform (juce::AffineTransform::rotation (discAngle).translated (centre.x, centre.y));
        g.setColour (Palette::background);
        g.fillPath (marker);
    }

    g.setColour (Palette::text);
    g.setFont (juce::Font (juce::FontOptions (22.0f, juce::Font::bold)));
    g.drawText ("SPUN ON WAX", header.withTrimmedLeft (60.0f), juce::Justification::centredLeft, false);

    g.setColour (Palette::amber.withAlpha (0.7f));
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.drawText ("vinyl / lo-fi", header.withTrimmedLeft (60.0f).translated (0.0f, 20.0f),
                juce::Justification::centredLeft, false);

    g.setColour (Palette::amberDim.withAlpha (0.5f));
    g.drawHorizontalLine (54, 0.0f, (float) getWidth());
}

void SpunOnWaxAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (60);
    area.reduce (12, 8);

    const int cols = 3;
    const int rows = 3;
    const int cellW = area.getWidth() / cols;
    const int cellH = area.getHeight() / rows;

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const int col = static_cast<int> (i) % cols;
        const int row = static_cast<int> (i) / cols;

        auto cell = juce::Rectangle<int> (area.getX() + col * cellW,
                                          area.getY() + row * cellH,
                                          cellW, cellH).reduced (6);

        knobs[i]->label.setBounds (cell.removeFromTop (16));
        knobs[i]->slider.setBounds (cell);
    }
}

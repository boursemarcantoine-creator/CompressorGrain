#include "PluginEditor.h"

CompressorGrainAudioProcessorEditor::CompressorGrainAudioProcessorEditor (CompressorGrainAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p),
      compressionAttachment (p.apvts, CompressorGrainAudioProcessor::amountParamID, compressionSlider),
      grainAttachment (p.apvts, CompressorGrainAudioProcessor::grainParamID, grainSlider),
      hiCutAttachment (p.apvts, CompressorGrainAudioProcessor::hiCutParamID, hiCutButton)
{
    setLookAndFeel (&vintageLookAndFeel);

    for (auto* s : { &compressionSlider, &grainSlider })
    {
        s->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s->setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (135.0f) + juce::MathConstants<float>::twoPi, true);
        s->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible (s);
    }

    setupLabel (compressionLabel);
    setupLabel (grainLabel);

    hiCutLabel.setJustificationType (juce::Justification::centred);
    hiCutLabel.setFont (juce::Font (10.0f, juce::Font::bold));
    hiCutLabel.setColour (juce::Label::textColourId, juce::Colour (0xffece7d8));
    addAndMakeVisible (hiCutLabel);

    hiCutButton.setClickingTogglesState (true);
    hiCutButton.setButtonText ({});
    addAndMakeVisible (hiCutButton);

    outputMeter.setBacklightColour (juce::Colour (0xffffa542));  // warm vintage orange
    eqDisplay.setBacklightColour (juce::Colour (0xffffa542));

    addAndMakeVisible (outputMeter);
    addAndMakeVisible (eqDisplay);

    setSize (460, 312);
    startTimerHz (30);
}

CompressorGrainAudioProcessorEditor::~CompressorGrainAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void CompressorGrainAudioProcessorEditor::setupLabel (juce::Label& label)
{
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (13.0f, juce::Font::bold));
    label.setColour (juce::Label::textColourId, juce::Colour (0xffece7d8));
    addAndMakeVisible (label);
}

void CompressorGrainAudioProcessorEditor::timerCallback()
{
    outputMeter.pushLevel (processor.getOutputLevel());
    eqDisplay.setGrainAmount ((float) grainSlider.getValue() / 100.0f);
}

void CompressorGrainAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    VintageLookAndFeel::drawBrushedPanel (g, bounds);

    // dark trim strip at very top, like the amp's top edge
    g.setColour (juce::Colour (0xff000000));
    g.fillRect (bounds.removeFromTop (6.0f));

    // title: straight sans-serif, bold, no italic
    g.setColour (juce::Colour (0xffece7d8));
    g.setFont (juce::Font (24.0f, juce::Font::bold));
    g.drawText ("CompressorGrain", getLocalBounds().removeFromTop (46).withTrimmedTop (10),
                juce::Justification::centred);

    g.setFont (juce::Font (11.0f, juce::Font::italic));
    g.drawText ("by ELWRAY", getLocalBounds().removeFromTop (58).removeFromBottom (14),
                juce::Justification::centred);
}

void CompressorGrainAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (24);
    area.removeFromTop (44); // title space

    // VU meters
    auto meterRow = area.removeFromTop (70);
    auto meterWidth = meterRow.getWidth() / 2 - 10;

    outputMeter.setBounds (meterRow.removeFromLeft (meterWidth));
    meterRow.removeFromLeft (20);
    eqDisplay.setBounds (meterRow.removeFromLeft (meterWidth));

    area.removeFromTop (20);

    // single big knob each side: compression (amount) | grain, with the
    // hi-cut button in a narrow column right between them
    auto knobRow = area.removeFromTop (130);
    const int centreW = 48;
    const int sideW = (knobRow.getWidth() - centreW) / 2;

    auto leftKnobArea = knobRow.removeFromLeft (sideW);
    auto centreArea = knobRow.removeFromLeft (centreW);
    auto rightKnobArea = knobRow;

    compressionLabel.setBounds (leftKnobArea.removeFromTop (18));
    compressionSlider.setBounds (leftKnobArea.reduced (24, 4));

    grainLabel.setBounds (rightKnobArea.removeFromTop (18));
    grainSlider.setBounds (rightKnobArea.reduced (24, 4));

    hiCutLabel.setBounds (centreArea.removeFromTop (18));
    centreArea.removeFromTop (28); // vertically align the square button with the knobs
    auto buttonSize = juce::jmin (centreArea.getWidth(), 24);
    hiCutButton.setBounds (centreArea.withSizeKeepingCentre (buttonSize, buttonSize).removeFromTop (buttonSize));
}

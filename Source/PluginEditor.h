#pragma once

#include "PluginProcessor.h"
#include "VUMeter.h"
#include "EQCurveDisplay.h"
#include "CustomLookAndFeel.h"

class CompressorGrainAudioProcessorEditor : public juce::AudioProcessorEditor,
                                             private juce::Timer
{
public:
    explicit CompressorGrainAudioProcessorEditor (CompressorGrainAudioProcessor&);
    ~CompressorGrainAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setupLabel (juce::Label& label);

    CompressorGrainAudioProcessor& processor;

    VintageLookAndFeel vintageLookAndFeel;

    juce::Slider compressionSlider;
    juce::Slider grainSlider;

    juce::Label compressionLabel { {}, "compression" };
    juce::Label grainLabel { {}, "grain" };
    juce::Label hiCutLabel { {}, "hi-cut" };

    juce::TextButton hiCutButton;

    VUMeter outputMeter { "output" };
    EQCurveDisplay eqDisplay;

    juce::AudioProcessorValueTreeState::SliderAttachment compressionAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment grainAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment hiCutAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorGrainAudioProcessorEditor)
};

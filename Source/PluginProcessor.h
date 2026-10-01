#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class CompressorGrainAudioProcessor : public juce::AudioProcessor
{
public:
    CompressorGrainAudioProcessor();
    ~CompressorGrainAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "CompressorGrain"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    juce::AudioProcessorValueTreeState apvts;

    static constexpr auto grainParamID = "grain";
    static constexpr auto amountParamID = "amount"; // big knob under "compression": overall compression intensity
    static constexpr auto hiCutParamID = "hicut";   // SSL-style square button: 9kHz high-cut on/off

    float getInputLevel() const  { return inputLevel.load(); }
    float getOutputLevel() const { return outputLevel.load(); }

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Remembers the last-used knob settings across sessions, independently
    // of whatever the host DAW does with its own project-recall state.
    juce::File getSettingsFile() const;
    void loadLastUsedState();
    void saveLastUsedState();

    // Manual compressor with filtered sidechain detection (SSL/Neve-style
    // bass-pumping reduction: the detector "listens" through a high-pass
    // filter while the full-bandwidth audio still gets compressed)
    std::array<juce::dsp::IIR::Filter<float>, 2> scHighpass;
    float detectorGainReductionDb = 0.0f;

    static constexpr float thresholdMaxDb = 0.0f;     // amount = 0%  -> essentially no compression
    static constexpr float thresholdMinDb = -30.0f;   // amount = 100% -> heavy compression

    static constexpr float fixedRatio = 4.0f;      // ratio locked at 4:1
    static constexpr float fixedAttackMs = 5.0f;   // locked attack
    static constexpr float fixedReleaseMs = 500.0f; // locked release
    static constexpr float scHighpassFreqHz = 100.0f;

    // Light fixed tube-style saturation, applied after compression.
    // A small asymmetry (bias) before the tanh() breaks the symmetry so
    // even-order harmonics appear too (the "warm" tube character), on top
    // of the odd-order ones tanh() already produces.
    static constexpr float tubeDrive = 1.7f;
    static constexpr float tubeAsymmetry = 0.06f;
    std::array<juce::dsp::IIR::Filter<float>, 2> dcBlocker; // removes the DC the asymmetry introduces

    // Grain EQ: two resonant bumps + a parallel high-passed "sizzle" band
    std::array<juce::dsp::IIR::Filter<float>, 2> lowBump;
    std::array<juce::dsp::IIR::Filter<float>, 2> midBump;
    std::array<juce::dsp::IIR::Filter<float>, 2> hpBand;

    // Optional high-cut: three cascaded 2-pole lowpass stages at 7kHz
    // (12dB/octave each => ~36dB/octave combined), toggled by the SSL-style button
    static constexpr float hiCutFreqHz = 7000.0f;
    std::array<juce::dsp::IIR::Filter<float>, 2> hiCut1, hiCut2, hiCut3;

    double currentSampleRate = 44100.0;

    std::atomic<float> inputLevel { 0.0f };
    std::atomic<float> outputLevel { 0.0f };

    void updateGrainFilters (float grainAmount);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorGrainAudioProcessor)
};

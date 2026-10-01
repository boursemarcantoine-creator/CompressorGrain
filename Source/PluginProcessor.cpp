#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

CompressorGrainAudioProcessor::CompressorGrainAudioProcessor()
    : AudioProcessor (BusesProperties()
                           .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                           .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    loadLastUsedState();
}

CompressorGrainAudioProcessor::~CompressorGrainAudioProcessor()
{
    saveLastUsedState();
}

juce::File CompressorGrainAudioProcessor::getSettingsFile() const
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("CompressorGrain");
    dir.createDirectory();
    return dir.getChildFile ("lastUsedSettings.xml");
}

void CompressorGrainAudioProcessor::loadLastUsedState()
{
    auto file = getSettingsFile();
    if (! file.existsAsFile())
        return;

    if (auto xml = juce::XmlDocument::parse (file))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

void CompressorGrainAudioProcessor::saveLastUsedState()
{
    if (auto state = apvts.copyState(); state.isValid())
        if (std::unique_ptr<juce::XmlElement> xml (state.createXml()); xml != nullptr)
            xml->writeTo (getSettingsFile());
}

juce::AudioProcessorValueTreeState::ParameterLayout CompressorGrainAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Compression amount (0-100%): drives the threshold. Ratio, attack and
    // release are locked internally (4:1, 5ms, 500ms) — this single knob
    // controls how much of the signal gets caught and compressed.
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        amountParamID, "Compression",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 40.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel ("%")
            .withStringFromValueFunction ([] (float v, int) { return juce::String (v, 0) + "%"; })));

    // Grain is 0-100%, drives the bass/mid bumps and the top-end sizzle
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        grainParamID, "Grain",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel ("%")
            .withStringFromValueFunction ([] (float v, int) { return juce::String (v, 0) + "%"; })));

    // Hi-cut: SSL-style on/off square button, 9kHz / 36dB per octave
    params.push_back (std::make_unique<juce::AudioParameterBool>(
        hiCutParamID, "Hi-cut", false));

    return { params.begin(), params.end() };
}

void CompressorGrainAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    juce::ignoreUnused (samplesPerBlock);

    auto scCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (
        currentSampleRate, scHighpassFreqHz, 0.707f);

    for (auto& f : scHighpass)
    {
        f.reset();
        *f.coefficients = *scCoeffs;
    }

    // DC blocker: very low cutoff so it only removes the bias offset from the
    // asymmetric saturation, without slowing down transient response at all
    auto dcCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (
        currentSampleRate, 5.0f, 0.707f);

    for (auto& f : dcBlocker)
    {
        f.reset();
        *f.coefficients = *dcCoeffs;
    }

    detectorGainReductionDb = 0.0f;

    for (auto& f : lowBump)  f.reset();
    for (auto& f : midBump)  f.reset();
    for (auto& f : hpBand)   f.reset();

    auto hiCutCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (
        currentSampleRate, hiCutFreqHz, 0.707f);
    for (auto* stage : { &hiCut1, &hiCut2, &hiCut3 })
        for (auto& f : *stage)
        {
            f.reset();
            *f.coefficients = *hiCutCoeffs;
        }

    updateGrainFilters (0.0f);
}

bool CompressorGrainAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void CompressorGrainAudioProcessor::updateGrainFilters (float grainAmount)
{
    // grainAmount is 0..1
    const float lowGainDb = juce::jmap (grainAmount, 0.0f, 1.0f, 0.0f, 7.0f);
    const float midGainDb = juce::jmap (grainAmount, 0.0f, 1.0f, 0.0f, 5.0f);

    auto lowCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
        currentSampleRate, 72.0f, 1.1f, juce::Decibels::decibelsToGain (lowGainDb));
    auto midCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
        currentSampleRate, 621.0f, 1.2f, juce::Decibels::decibelsToGain (midGainDb));
    auto hpCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (
        currentSampleRate, 6000.0f, 0.707f);

    for (auto& f : lowBump) *f.coefficients = *lowCoeffs;
    for (auto& f : midBump) *f.coefficients = *midCoeffs;
    for (auto& f : hpBand)  *f.coefficients = *hpCoeffs;
}

void CompressorGrainAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numChannels = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    const float amountPercent = *apvts.getRawParameterValue (amountParamID);
    const float grainPercent = *apvts.getRawParameterValue (grainParamID);
    const float grain = grainPercent / 100.0f;
    const bool hiCutOn = *apvts.getRawParameterValue (hiCutParamID) > 0.5f;

    // Compression amount knob drives the threshold; ratio/attack/release are locked
    const float thresholdDb = juce::jmap (amountPercent, 0.0f, 100.0f, thresholdMaxDb, thresholdMinDb);

    updateGrainFilters (grain);

    // Input level (peak) before any processing
    inputLevel.store (buffer.getMagnitude (0, numSamples));

    // One-pole envelope coefficients from the locked attack/release times
    const float attackCoeff  = std::exp (-1.0f / (0.001f * fixedAttackMs  * (float) currentSampleRate));
    const float releaseCoeff = std::exp (-1.0f / (0.001f * fixedReleaseMs * (float) currentSampleRate));

    auto* left  = buffer.getWritePointer (0);
    auto* right = numChannels > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        // --- Sidechain detection: high-passed copy of the signal, so low-end
        //     energy doesn't dominate the gain-reduction decision (less bass pumping)
        float scL = scHighpass[0].processSample (left[i]);
        float scR = right != nullptr ? scHighpass[1].processSample (right[i]) : scL;
        float detectLevel = juce::jmax (std::abs (scL), std::abs (scR));
        float detectDb = juce::Decibels::gainToDecibels (detectLevel, -60.0f);

        float overshoot = detectDb - thresholdDb;
        float targetGR = overshoot > 0.0f ? overshoot * (1.0f - 1.0f / fixedRatio) : 0.0f;

        float coeff = targetGR > detectorGainReductionDb ? attackCoeff : releaseCoeff;
        detectorGainReductionDb = targetGR + coeff * (detectorGainReductionDb - targetGR);

        float gainLinear = juce::Decibels::decibelsToGain (-detectorGainReductionDb);

        // --- Apply gain reduction to the full-bandwidth audio
        float outL = left[i] * gainLinear;
        float outR = right != nullptr ? right[i] * gainLinear : 0.0f;

        // --- Light fixed tube-style saturation, after compression.
        //     Asymmetric (biased) tanh() => odd AND even harmonics, for a
        //     warmer, less "transistor-clean" character than a plain tanh().
        auto tubeSaturate = [] (float x)
        {
            float shaped = std::tanh ((x + tubeAsymmetry) * tubeDrive) - std::tanh (tubeAsymmetry * tubeDrive);
            return shaped / std::tanh (tubeDrive);
        };

        outL = tubeSaturate (outL);
        outL = dcBlocker[0].processSample (outL);
        if (right != nullptr)
        {
            outR = tubeSaturate (outR);
            outR = dcBlocker[1].processSample (outR);
        }

        // --- Grain: bass/mid bumps in series, plus a parallel high-passed
        //     "sizzle" band blended back in on top, amount scaling with grain.
        const float sizzleMix = grain * 0.6f;

        outL = lowBump[0].processSample (outL);
        outL = midBump[0].processSample (outL);
        outL += hpBand[0].processSample (outL) * sizzleMix;
        left[i] = outL;

        if (right != nullptr)
        {
            outR = lowBump[1].processSample (outR);
            outR = midBump[1].processSample (outR);
            outR += hpBand[1].processSample (outR) * sizzleMix;
            right[i] = outR;
        }

        // --- Optional hi-cut, 9kHz / ~36dB per octave (3 cascaded 2-pole stages)
        if (hiCutOn)
        {
            left[i] = hiCut3[0].processSample (hiCut2[0].processSample (hiCut1[0].processSample (left[i])));
            if (right != nullptr)
                right[i] = hiCut3[1].processSample (hiCut2[1].processSample (hiCut1[1].processSample (right[i])));
        }
    }

    // Output level (peak) after processing
    outputLevel.store (buffer.getMagnitude (0, numSamples));
}


void CompressorGrainAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        std::unique_ptr<juce::XmlElement> xml (state.createXml());
        copyXmlToBinary (*xml, destData);
    }
}

void CompressorGrainAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* CompressorGrainAudioProcessor::createEditor()
{
    return new CompressorGrainAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CompressorGrainAudioProcessor();
}

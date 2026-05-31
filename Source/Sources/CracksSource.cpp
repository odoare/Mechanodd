/*
  ==============================================================================

    CracksSource.cpp

  ==============================================================================
*/

#include "CracksSource.h"

void CracksSource::prepareImpl (const juce::dsp::ProcessSpec& s)
{
    generator.prepare (s);
}

void CracksSource::addParametersToLayout (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                                          const juce::String& prefix)
{
    addCommonParameters (params, prefix);
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        makeId (prefix, "density"), prefix + " Density",
        juce::NormalisableRange<float> (1.0f, 5000.0f, 1.0f, 0.4f), 200.0f));
}

void CracksSource::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    assignCommonParameters (apvts, prefix);
    pDensity = apvts.getRawParameterValue (makeId (prefix, "density"));
}

void CracksSource::checkParameters()
{
    checkCommonParameters();
    if (pDensity != nullptr)
        generator.setDensity ((int) pDensity->load());
}

void CracksSource::renderSource (juce::AudioBuffer<float>& outBuffer, int startSample, int numSamples)
{
    auto* out = outBuffer.getWritePointer (0);
    for (int i = 0; i < numSamples; ++i)
        out[startSample + i] = generator.nextSample();
}

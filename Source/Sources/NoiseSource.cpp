/*
  ==============================================================================

    NoiseSource.cpp

  ==============================================================================
*/

#include "NoiseSource.h"

void NoiseSource::addParametersToLayout (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                                         const juce::String& prefix)
{
    addCommonParameters (params, prefix);
}

void NoiseSource::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    assignCommonParameters (apvts, prefix);
}

void NoiseSource::checkParameters()
{
    checkCommonParameters();
}

void NoiseSource::renderSource (juce::AudioBuffer<float>& outBuffer, int startSample, int numSamples)
{
    auto* out = outBuffer.getWritePointer (0);
    for (int i = 0; i < numSamples; ++i)
        out[startSample + i] = random.nextFloat() - 0.5f;
}

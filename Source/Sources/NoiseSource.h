/*
  ==============================================================================

    NoiseSource.h

    White-noise source. Tone shaping comes from the base-class resonant LPF.

  ==============================================================================
*/

#pragma once

#include "Source.h"

class NoiseSource : public Source
{
public:
    static constexpr const char* typeName = "Noise";

    void addParametersToLayout (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                                const juce::String& prefix) override;
    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix) override;
    void checkParameters() override;

protected:
    void renderSource (juce::AudioBuffer<float>& outBuffer, int startSample, int numSamples) override;

private:
    juce::Random random;
};

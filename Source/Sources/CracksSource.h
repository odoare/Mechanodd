/*
  ==============================================================================

    CracksSource.h

    Random click / crackle generator (reuses FxmeJuceTools' CracksGenerator).
    Tone shaping comes from the base-class resonant LPF + ADSR.

  ==============================================================================
*/

#pragma once

#include "Source.h"
#include "CracksGenerator.h"

class CracksSource : public Source
{
public:
    static constexpr const char* typeName = "Cracks";

    void addParametersToLayout (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                                const juce::String& prefix) override;
    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix) override;
    void checkParameters() override;

protected:
    void prepareImpl (const juce::dsp::ProcessSpec& spec) override;
    void renderSource (juce::AudioBuffer<float>& outBuffer, int startSample, int numSamples) override;

private:
    CracksGenerator generator;
    std::atomic<float>* pDensity { nullptr };
};

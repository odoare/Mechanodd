/*
  ==============================================================================

    CracksSource.h

    Random click / crackle generator (reuses FxmeTools' CracksGenerator).
    Tone shaping comes from the base-class resonant LPF + ADSR.

  ==============================================================================
*/

#pragma once

#include "Source.h"
#include <FxmeTools/dsp/CracksGenerator.h>

class CracksSource : public Source
{
public:
    static constexpr const char* typeName = "Cracks";

    void addParametersToLayout (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                                const juce::String& prefix) override;
    void assignParameters (ParamSource& apvts, const juce::String& prefix) override;
    void checkParameters() override;

protected:
    void prepareImpl (const juce::dsp::ProcessSpec& spec) override;
    void renderSource (juce::AudioBuffer<float>& outBuffer, int startSample, int numSamples) override;

private:
    fxme::CracksGenerator generator;
    std::atomic<float>* pDensity { nullptr };
};

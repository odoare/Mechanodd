/*
  ==============================================================================

    BlockResonator.h

    Perfectly blocking "resonator": output is always silent. Useful in the
    feedback matrix to mute a slot's contribution entirely.

  ==============================================================================
*/

#pragma once

#include "Resonator.h"

class BlockResonator : public Resonator
{
public:
    static constexpr const char* typeName = "Block";

    float processSample (float) override { return 0.0f; }

    void addParametersToLayout (std::vector<std::unique_ptr<juce::RangedAudioParameter>>&, const juce::String&) override {}
    void assignParameters (juce::AudioProcessorValueTreeState&, const juce::String&) override {}
    void checkParameters() override {}
};

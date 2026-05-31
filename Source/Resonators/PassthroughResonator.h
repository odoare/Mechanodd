/*
  ==============================================================================

    PassthroughResonator.h

    Perfectly transparent "resonator": output equals input. Useful in the
    feedback matrix to route a signal through a slot unchanged.

  ==============================================================================
*/

#pragma once

#include "Resonator.h"

class PassthroughResonator : public Resonator
{
public:
    static constexpr const char* typeName = "Passthrough";

    float processSample (float input) override { return input; }

    void addParametersToLayout (std::vector<std::unique_ptr<juce::RangedAudioParameter>>&, const juce::String&) override {}
    void assignParameters (juce::AudioProcessorValueTreeState&, const juce::String&) override {}
    void checkParameters() override {}
};

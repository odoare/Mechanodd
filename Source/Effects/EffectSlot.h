/*
  ==============================================================================

    EffectSlot.h

    One effect slot in a chain. Pre-instantiates every registered effect and
    selects the active one by a "_type" choice ("Off" + effect names). Only the
    active effect processes; Off bypasses.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Effect.h"

class EffectSlot
{
public:
    EffectSlot();

    void prepare (double sampleRate, int numChannels, int blockSize);
    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& slotPrefix);
    void checkParameters();
    void process (juce::AudioBuffer<float>& buffer);

    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& slotPrefix);

    static juce::String typeParamId (const juce::String& slotPrefix) { return slotPrefix + "_type"; }
    static juce::String perTypePrefix (const juce::String& slotPrefix, const juce::String& typeName) { return slotPrefix + "_" + typeName; }

private:
    std::vector<std::unique_ptr<Effect>> effects;   // aligned with EffectFactory::types()
    std::atomic<float>* typeParam { nullptr };
    int activeType { 0 };   // 0 == Off
};

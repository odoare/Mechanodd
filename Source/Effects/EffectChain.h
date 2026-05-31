/*
  ==============================================================================

    EffectChain.h

    A series of 4 effect slots. Used twice: once for the master output, once for
    the send bus. Slot parameter ids are "<chainPrefix>_fx{n}_...".

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "EffectSlot.h"

class EffectChain
{
public:
    static constexpr int numSlots = 4;

    void prepare (double sampleRate, int numChannels, int blockSize);
    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& chainPrefix);
    void checkParameters();
    void process (juce::AudioBuffer<float>& buffer);

    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& chainPrefix);

    static juce::String slotPrefix (const juce::String& chainPrefix, int slot) { return chainPrefix + "_fx" + juce::String (slot); }

private:
    std::array<EffectSlot, numSlots> slots;
};

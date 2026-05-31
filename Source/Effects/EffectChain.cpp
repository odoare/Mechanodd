/*
  ==============================================================================

    EffectChain.cpp

  ==============================================================================
*/

#include "EffectChain.h"

void EffectChain::prepare (double sampleRate, int numChannels, int blockSize)
{
    for (auto& slot : slots)
        slot.prepare (sampleRate, numChannels, blockSize);
}

void EffectChain::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& chainPrefix)
{
    for (int i = 0; i < numSlots; ++i)
        slots[(size_t) i].assignParameters (apvts, slotPrefix (chainPrefix, i));
}

void EffectChain::checkParameters()
{
    for (auto& slot : slots)
        slot.checkParameters();
}

void EffectChain::process (juce::AudioBuffer<float>& buffer)
{
    for (auto& slot : slots)
        slot.process (buffer);
}

void EffectChain::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& chainPrefix)
{
    for (int i = 0; i < numSlots; ++i)
        EffectSlot::addParameters (params, slotPrefix (chainPrefix, i));
}

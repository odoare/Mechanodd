/*
  ==============================================================================

    SourceSlot.h

    One source slot. Pre-instantiates a DSP instance of every registered type
    and selects the active one by a "_type" choice parameter. Switching type is
    just an index change on the audio thread (no allocation, no locks).

    Parameter id scheme:
      <slotPrefix>_type                choice: Off / <type names...>
      <slotPrefix>_<TypeName>_<param>  that type's parameters

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Source.h"

class SourceSlot
{
public:
    SourceSlot();

    void prepare (const juce::dsp::ProcessSpec& spec);
    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& slotPrefix);
    void checkParameters();
    void process (juce::AudioBuffer<float>& outBuffer, int startSample, int numSamples);
    void noteOn (float velocity);
    void noteOff();
    void setNoteFrequency (float hz);
    void setInputPointers (const float* l, const float* r);
    bool isActive() const;

    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& slotPrefix);

    static juce::String typeParamId  (const juce::String& slotPrefix)                          { return slotPrefix + "_type"; }
    static juce::String perTypePrefix (const juce::String& slotPrefix, const juce::String& typeName) { return slotPrefix + "_" + typeName; }

private:
    // Aligned with SourceFactory::types(); active source is sources[activeType - 1].
    std::vector<std::unique_ptr<Source>> sources;
    std::atomic<float>* typeParam { nullptr };
    int activeType { 0 };   // 0 == Off
};

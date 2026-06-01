/*
  ==============================================================================

    ResonatorSlot.h

    One resonator slot. Pre-instantiates a DSP instance of every registered type
    and selects the active one by a "_type" choice parameter. A per-slot toggle
    chooses whether the slot is per-voice (tuned to the played note) or global
    (tuned to a base-frequency parameter, independent of the note).

    The same slot prefix backs several DSP instances: one per voice and one
    global. They all read the same parameters; callers (voice / processor /
    feedback matrix) use isGlobal() to decide which instances are live.

    Parameter id scheme:
      <slotPrefix>_type                  choice: type names
      <slotPrefix>_global                bool: per-voice (0) / global (1)
      <slotPrefix>_globalFreq            Hz: base frequency when global
      <slotPrefix>_<TypeName>_<param>    that type's parameters

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Resonator.h"

class ResonatorSlot
{
public:
    static constexpr int numSlots = 4;
    static juce::String slotPrefix (int slot) { return "res" + juce::String (slot); }

    ResonatorSlot();

    void prepare (const juce::dsp::ProcessSpec& spec);
    void assignParameters (ParamSource& apvts, const juce::String& slotPrefix);
    void checkParameters();
    float processSample (float input) { return active()->processSample (input); }
    void reset();

    // Note gate forwarded to every type instance so the active one is correct
    // regardless of a mid-note type switch.
    void noteOn()  { for (auto& r : resonators) r->noteOn(); }
    void noteOff() { for (auto& r : resonators) r->noteOff(); }

    // Fundamental used when the slot is per-voice (set by the owning voice).
    void setVoiceFrequency (float hz) { voiceFrequency = hz; }
    bool isGlobal() const { return global; }

    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& slotPrefix);

    static juce::String typeParamId       (const juce::String& slotPrefix) { return slotPrefix + "_type"; }
    static juce::String globalParamId     (const juce::String& slotPrefix) { return slotPrefix + "_global"; }
    static juce::String globalFreqParamId (const juce::String& slotPrefix) { return slotPrefix + "_globalFreq"; }
    static juce::String perTypePrefix     (const juce::String& slotPrefix, const juce::String& typeName) { return slotPrefix + "_" + typeName; }

private:
    Resonator* active() { return resonators[(size_t) activeType].get(); }

    // Aligned with ResonatorFactory::types().
    std::vector<std::unique_ptr<Resonator>> resonators;

    std::atomic<float>* typeParam       { nullptr };
    std::atomic<float>* globalParam     { nullptr };
    std::atomic<float>* globalFreqParam { nullptr };

    int   activeType     { 0 };
    bool  global         { false };
    float voiceFrequency { 110.0f };
};

/*
  ==============================================================================

    ModEngine.h

    Modulation engine: a bank of LFO modulators that each target one parameter.

    Modulation is applied by overwriting the target parameter's raw-value atomic
    (what every module, including the reused FxmeFX effects, reads) each block,
    BEFORE the modules read their parameters. The authoritative base value is
    read from the parameter object (getValue), so host automation and the GUI are
    never fought: each block we recompute base + modulation from scratch.

    Targets are resolved by APVTS id and are always valid (parameters are static
    in this plugin), so a removed/changed element can never produce a dangling
    target.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class ModEngine
{
public:
    static constexpr int numModulators = 12;

    enum Shape { sine = 0, triangle, square, sawUp, sawDown };
    enum ModType { typeLfo = 0, typeAdsr };

    void prepare (double sampleRate);
    void reset();

    // Builds the target/parameter mapping. Call after the APVTS exists.
    void assignParameters (juce::AudioProcessorValueTreeState& apvts);

    // Advance the modulators and write modulated values into the target raw-value atomics.
    // gate/noteOnThisBlock drive the (global) ADSR modulators: a note-on retriggers the
    // envelope, and gate stays high while any note is held.
    void process (int numSamples, double bpm, double ppqPosition, bool isPlaying,
                  bool gate, bool noteOnThisBlock);

    // Adds modulator parameters. `targetChoices` must be "None" followed by every
    // modulatable parameter id, in a stable order.
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                               const juce::StringArray& targetChoices);

    static juce::String modPrefix    (int i) { return "mod" + juce::String (i); }
    static juce::String targetId      (int i) { return modPrefix (i) + "_target"; }
    static juce::String typeId        (int i) { return modPrefix (i) + "_type"; }
    static juce::String shapeId       (int i) { return modPrefix (i) + "_shape"; }
    static juce::String rateId        (int i) { return modPrefix (i) + "_rate"; }
    static juce::String syncId        (int i) { return modPrefix (i) + "_sync"; }
    static juce::String syncRateId    (int i) { return modPrefix (i) + "_syncrate"; }
    static juce::String depthId       (int i) { return modPrefix (i) + "_depth"; }
    static juce::String attackId      (int i) { return modPrefix (i) + "_attack"; }
    static juce::String decayId       (int i) { return modPrefix (i) + "_decay"; }
    static juce::String sustainId     (int i) { return modPrefix (i) + "_sustain"; }
    static juce::String releaseId     (int i) { return modPrefix (i) + "_release"; }
    static juce::String amountId      (int i) { return modPrefix (i) + "_amount"; }
    static juce::String polarityId    (int i) { return modPrefix (i) + "_polarity"; }

    static juce::StringArray typeChoices()     { return { "LFO", "ADSR" }; }
    static juce::StringArray shapeChoices()    { return { "Sine", "Tri", "Square", "Saw Up", "Saw Dn" }; }
    static juce::StringArray syncRateChoices() { return { "1/1", "1/2", "1/4", "1/8", "1/16", "1/8T", "1/16T" }; }
    static juce::StringArray polarityChoices() { return { "-", "+" }; }

private:
    static float evalLfo (int shape, float phase);
    static float syncRateBeats (int index);   // beats per LFO cycle

    double sampleRate { 44100.0 };

    struct Modulator
    {
        std::atomic<float>* target   { nullptr };
        std::atomic<float>* type     { nullptr };
        std::atomic<float>* shape    { nullptr };
        std::atomic<float>* rate     { nullptr };
        std::atomic<float>* sync     { nullptr };
        std::atomic<float>* syncRate { nullptr };
        std::atomic<float>* depth    { nullptr };
        std::atomic<float>* attack   { nullptr };
        std::atomic<float>* decay    { nullptr };
        std::atomic<float>* sustain  { nullptr };
        std::atomic<float>* release  { nullptr };
        std::atomic<float>* amount   { nullptr };
        std::atomic<float>* polarity { nullptr };
        float phase { 0.0f };
        juce::ADSR adsr;
    };
    std::array<Modulator, numModulators> mods;

    // Index 0 == "None"; others map to a modulatable float parameter.
    std::vector<juce::RangedAudioParameter*> targetParams;
    std::vector<std::atomic<float>*>         targetAtomics;

    std::vector<float> offset;        // per target, accumulator (sized to targets)
    std::vector<int>   touchedNow;    // target indices modulated this block
    std::vector<int>   prevTouched;   // target indices modulated last block
    bool               prevGate { false };   // global ADSR gate state last block
};

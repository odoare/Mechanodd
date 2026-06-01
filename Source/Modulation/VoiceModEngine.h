/*
  ==============================================================================

    VoiceModEngine.h

    Per-voice modulation: each SynthVoice owns one. It mirrors the global
    ModEngine's ADSR modulators, but the envelope phase is per-voice (gated by
    that voice's own note) and the result is written into per-voice "shadow"
    parameter atomics that this voice's modules read instead of the shared APVTS
    atomics (see ParamSource).

    Each block, every shadow is refreshed from its global atomic (which already
    carries any global LFO modulation), then the voice's ADSR offsets are added
    on top. Only per-voice targets (sources / resonators / per-voice matrix) get
    shadows; global targets are left to the global ModEngine.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "ModEngine.h"

class ParamSource;

class VoiceModEngine
{
public:
    void prepare (double sampleRate);
    void reset();

    // Build the shadows + register them as overrides in `src`, and resolve the
    // (shared) modulator setting atomics from the APVTS. Call before the voice's
    // modules assign their parameters, so they pick up the shadow pointers.
    void assign (juce::AudioProcessorValueTreeState& apvts, ParamSource& src);

    void noteOn();
    void noteOff();

    // Refresh shadows from the global atomics and apply this voice's ADSR offsets.
    void process (int numSamples);

private:
    double sampleRate { 44100.0 };

    struct Shadow
    {
        std::unique_ptr<std::atomic<float>> value;          // per-voice atomic (the override)
        std::atomic<float>*                 global { nullptr };   // global base (incl. LFO)
        juce::RangedAudioParameter*         param  { nullptr };   // for range + getValue()
        std::atomic<float>*                 resGlobalFlag { nullptr };  // res slot _global, else null
    };
    std::vector<Shadow> shadows;
    std::vector<float>  accum;            // per-shadow ADSR offset accumulator (normalised)

    struct Mod
    {
        std::atomic<float>* target   { nullptr };
        std::atomic<float>* type     { nullptr };
        std::atomic<float>* attack   { nullptr };
        std::atomic<float>* decay    { nullptr };
        std::atomic<float>* sustain  { nullptr };
        std::atomic<float>* release  { nullptr };
        std::atomic<float>* amount   { nullptr };
        std::atomic<float>* polarity { nullptr };
        juce::ADSR adsr;
    };
    std::array<Mod, ModEngine::numModulators> mods;

    // Flat target index -> shadow index, or -1 if the target is not per-voice.
    std::vector<int> targetToShadow;
};

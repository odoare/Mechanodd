/*
  ==============================================================================

    Resonator.h

    Abstract base for all resonators (waveguide, plate, membrane, ...).
    A resonator is a mono input -> mono output process. The base owns tuning
    only: a fundamental (set externally to the MIDI note for per-voice slots,
    or a Hz parameter for global slots) plus fine [-1,1] and coarse [-12,12]
    semitone offsets. Routing, level, pan and send are handled by the
    FeedbackMatrix, not here.

    Modular parameter contract mirrors the sources: addParametersToLayout /
    assignParameters / checkParameters, all prefixed.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class ParamSource;

class Resonator
{
public:
    virtual ~Resonator() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    // Mono input -> mono output, one sample at a time. Per-sample granularity is
    // required so the feedback matrix can resolve resonator-to-resonator coupling
    // with a single-sample delay.
    virtual float processSample (float input) = 0;

    // Convenience block helper built on processSample.
    void processBlock (const float* input, float* output, int numSamples);

    // Fundamental frequency before fine/coarse offsets (Hz).
    void setBaseFrequency (float hz);

    // Per-voice note gate. Resonator types that expose On/Off parameter pairs
    // crossfade them with the gate value (0 = note off, 1 = note on). Global
    // (never-gated) slots keep the gate pinned at 1, so they always use the
    // "On" tuning.
    void noteOn();
    void noteOff();

    virtual void addParametersToLayout (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                                        const juce::String& prefix) = 0;
    virtual void assignParameters (ParamSource& apvts, const juce::String& prefix) = 0;
    virtual void checkParameters() = 0;

    static juce::String makeId (const juce::String& prefix, const juce::String& name) { return prefix + "_" + name; }

protected:
    virtual void prepareImpl (const juce::dsp::ProcessSpec& /*spec*/) {}
    virtual void resetImpl() {}

    static void addCommonParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                                     const juce::String& prefix);
    void assignCommonParameters (ParamSource& apvts, const juce::String& prefix);
    // Returns true if the tuned fundamental changed since the last call.
    bool checkCommonParameters();

    // Gate parameters (Note Rel time). Opt-in: only types that use On/Off
    // crossfade need to register them. The note-on transition snaps; only the
    // release is ramped.
    static void addGateParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                                   const juce::String& prefix);
    void assignGateParameters (ParamSource& apvts, const juce::String& prefix);
    void checkGateParameters();              // recompute release coefficient

    float advanceGate() noexcept;            // step the gate one sample, return [0,1]
    float currentGate() const noexcept { return gateValue; }

    float getTunedFrequency() const;

    juce::dsp::ProcessSpec spec { 44100.0, 512, 1 };
    float baseFrequency { 110.0f };
    float fine   { 0.0f };   // semitones [-1,1]
    float coarse { 0.0f };   // semitones [-12,12]

private:
    std::atomic<float>* pFine   { nullptr };
    std::atomic<float>* pCoarse { nullptr };
    float lastTuned { -1.0f };

    std::atomic<float>* pGateRel { nullptr };
    float gateValue    { 1.0f };
    float gateTarget   { 1.0f };
    float gateRelCoeff { 1.0f };
};

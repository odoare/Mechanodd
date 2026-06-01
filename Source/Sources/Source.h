/*
  ==============================================================================

    Source.h

    Abstract base for all sound sources (oscillator, noise, cracks, ...).
    Owns the behaviour shared by every source: an ADSR envelope, a resonant
    low-pass filter, an output level, and velocity links to level/cutoff.

    Modular parameter contract (mirrors the FxmeFX module pattern): every
    concrete source registers its parameters under a string `prefix`, caches
    the atomic pointers in assignParameters(), and reads them once per block in
    checkParameters(). This is what makes source slots swappable and keeps the
    audio thread allocation/lock free.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class ParamSource;

class Source
{
public:
    virtual ~Source() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);

    // Generates this source's contribution and ADDS it into outBuffer
    // (mono content written to every channel), applying the subclass signal,
    // then the resonant LPF, ADSR envelope and output level.
    void process (juce::AudioBuffer<float>& outBuffer, int startSample, int numSamples);

    void noteOn (float velocity);
    void noteOff();
    bool isActive() const { return adsr.isActive(); }

    // The played note's fundamental (Hz). Pitched sources (oscillator) use it;
    // others ignore it.
    virtual void setNoteFrequency (float hz) { juce::ignoreUnused (hz); }

    // Pointers to the current sub-block's host audio input (already offset to
    // the sub-block start), valid only for the imminent process() call. Only
    // the "Plugin Input L/R" sources use them; others ignore.
    virtual void setInputPointers (const float* l, const float* r) { juce::ignoreUnused (l, r); }

    // ---- Modular parameter contract -------------------------------------------------
    // Subclasses must call addCommonParameters / assignCommonParameters /
    // checkCommonParameters from their own override, then add their specifics.
    virtual void addParametersToLayout (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                                        const juce::String& prefix) = 0;
    virtual void assignParameters (ParamSource& apvts, const juce::String& prefix) = 0;
    virtual void checkParameters() = 0;

    static juce::String makeId (const juce::String& prefix, const juce::String& name) { return prefix + "_" + name; }

protected:
    // Template-method hooks implemented by concrete sources.
    virtual void prepareImpl (const juce::dsp::ProcessSpec& /*spec*/) {}
    virtual void onNoteOn() {}   // called at note start (after envelope retrigger)
    // Fill outBuffer[start..start+n) channel 0 with the raw (pre-filter) mono signal.
    virtual void renderSource (juce::AudioBuffer<float>& outBuffer, int startSample, int numSamples) = 0;

    // Common parameter helpers for subclasses.
    static void addCommonParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                                     const juce::String& prefix);
    void assignCommonParameters (ParamSource& apvts, const juce::String& prefix);
    void checkCommonParameters();

    juce::dsp::ProcessSpec spec { 44100.0, 512, 1 };

    // Shared DSP state
    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams { 0.01f, 0.1f, 1.0f, 0.2f };
    juce::dsp::StateVariableTPTFilter<float> lpf;

    float level        { 1.0f };   // linear gain
    float cutoff       { 20000.0f };
    float resonance    { 0.70710678f };
    float velToLevel   { 0.0f };    // 0..1: amount velocity scales level
    float velToCutoff  { 0.0f };    // 0..1: amount velocity scales cutoff
    float velocity     { 1.0f };

private:
    // Cached APVTS pointers for the common parameters.
    std::atomic<float>* pAttack    { nullptr };
    std::atomic<float>* pDecay     { nullptr };
    std::atomic<float>* pSustain   { nullptr };
    std::atomic<float>* pRelease   { nullptr };
    std::atomic<float>* pCutoff    { nullptr };
    std::atomic<float>* pResonance { nullptr };
    std::atomic<float>* pLevel     { nullptr };
    std::atomic<float>* pVelLevel  { nullptr };
    std::atomic<float>* pVelCutoff { nullptr };

    juce::AudioBuffer<float> scratch;  // mono render scratch
};

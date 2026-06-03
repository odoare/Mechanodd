/*
  ==============================================================================

    WaveguideResonator.h

    Dispersive digital-waveguide string. A bidirectional pair of rails (split at
    the input position into 4 fractional delay lines) forms the loop; the output
    is read as a fractional tap (sum of both rails) at the output position. The
    loop reflection at the "bridge" applies a feedback gain, a one-pole low-pass
    (frequency-dependent decay) and a chain of first-order all-pass sections that
    produce phase dispersion (bending-stiffness inharmonicity), controlled by a
    single Dispersion parameter. The all-pass group delay is compensated in the
    rail lengths so the played pitch stays correct.

  ==============================================================================
*/

#pragma once

#include "Resonator.h"

class WaveguideResonator : public Resonator
{
public:
    static constexpr const char* typeName = "Waveguide";

    float processSample (float input) override;

    void addParametersToLayout (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                                const juce::String& prefix) override;
    void assignParameters (ParamSource& apvts, const juce::String& prefix) override;
    void checkParameters() override;

protected:
    void prepareImpl (const juce::dsp::ProcessSpec& spec) override;
    void resetImpl() override;

private:
    static constexpr int   numAllpass = 8;
    static constexpr float minFrequency = 20.0f;

    // The loop's feedback gain and low-pass act once per round trip, so their effect
    // per second scales with the played frequency: without compensation high notes
    // decay faster and darker. We normalise both against a reference pitch so sustain
    // (T60) and timbre stay consistent across the range. The knobs read true at f_ref.
    static constexpr float referenceFrequency = 261.626f;   // middle C

    void updateGeometry();

    using DelayLine = juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd>;

    // upA: nut->input, upB: input->bridge (right-going);
    // dnB: bridge->input, dnA: input->nut (left-going).
    DelayLine upA, upB, dnB, dnA;

    // Loop one-pole low-pass state and coefficient.
    float lpState { 0.0f };
    float lpCoeff { 1.0f };

    // First-order all-pass dispersion chain.
    std::array<float, numAllpass> apState {};
    float apCoeff { 0.0f };
    float targetApCoeff { 0.0f };

    // Geometry (samples) and clamped positions. oneWayLength and apCoeff are
    // smoothed per-sample toward the targets set by updateGeometry(), so rapid
    // tuning changes - knob / modulation sweeps, portamento - don't step the
    // fractional delay reads or the dispersion coefficient and click. lenA/lenB
    // (nut->input / input->bridge) are derived from the smoothed length each
    // sample. snapGeometry forces an immediate jump (note start / prepare).
    float oneWayLength { 100.0f };
    float targetOneWayLength { 100.0f };
    float cInPos  { 0.12f };
    float cOutPos { 0.85f };
    float geoSmoothCoeff { 1.0f };
    bool  snapGeometry { true };

    // Parameters. Feedback gain and feedback cutoff each have a note-on and a
    // note-off value, crossfaded per-sample by the gate (see processSample).
    float fbGainOn    { 0.995f };
    float fbGainOff   { 0.9f };
    float fbCutoffOn  { 8000.0f };
    float fbCutoffOff { 2000.0f };

    // Pitch-normalised versions of the four feedback values above, recomputed from the
    // played frequency in updateGeometry() and used by processSample().
    float fbGainOnNorm    { 0.995f };
    float fbGainOffNorm   { 0.9f };
    float fbCutoffOnNorm  { 8000.0f };
    float fbCutoffOffNorm { 2000.0f };

    float inPos       { 0.12f };
    float outPos      { 0.85f };
    float dispersion  { 0.0f };

    float lastLpCutoff { -1.0f };   // last cutoff used to compute lpCoeff (recompute throttle)

    std::atomic<float>* pFbGainOn    { nullptr };
    std::atomic<float>* pFbGainOff   { nullptr };
    std::atomic<float>* pFbCutoffOn  { nullptr };
    std::atomic<float>* pFbCutoffOff { nullptr };
    std::atomic<float>* pInPos       { nullptr };
    std::atomic<float>* pOutPos      { nullptr };
    std::atomic<float>* pDispersion  { nullptr };
};

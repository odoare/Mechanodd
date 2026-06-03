/*
  ==============================================================================

    BeamStringResonator.h

    Pinned-pinned (simply supported) 1D resonator that morphs between an ideal
    tensioned string and a flexural Euler-Bernoulli beam. A beam under axial
    tension keeps exactly sinusoidal mode shapes sin(n*pi*x) in every case, and
    its eigenfrequencies follow

        omega_n^2  ~  (1 - b) (n*pi)^2  +  b (n*pi)^4

    where b is the string->beam control. Normalised so f(1) == fundamental:

        f_n = fundamental * n * sqrt( (1 - b) + b * n^2 )

      - b = 0 : f_n = n * f0          pure harmonic string
      - b = 1 : f_n = n^2 * f0        pure flexural beam
      - 0<b<1 : the classic stiff-string law (inharmonic / stretched partials)

    Reuses the whole modal core (biquad bank, damping, gain compensation, gate);
    it only swaps the 2D rectangular geometry for a 1D one, so the aspect ratio
    becomes a string<->beam morph and the four in/out X/Y taps become a single
    in/out position along the structure (like the waveguide).

  ==============================================================================
*/

#pragma once

#include "ModalResonator.h"

class BeamStringResonator : public ModalResonator
{
public:
    static constexpr const char* typeName    = "BeamString";
    static constexpr const char* displayName = "Beam / String";

protected:
    int  collectCandidates (float fundamental, Candidate* out, int maxOut) const override;

    void addGeometryParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix) override;
    void assignGeometryParameters (ParamSource& apvts, const juce::String& prefix) override;
    bool checkGeometryParameters() override;

private:
    float mix    { 0.0f };    // 0 = string (harmonic), 1 = beam (n^2)
    float inPos  { 0.12f };
    float outPos { 0.85f };

    std::atomic<float>* pMix    { nullptr };
    std::atomic<float>* pInPos  { nullptr };
    std::atomic<float>* pOutPos { nullptr };
};

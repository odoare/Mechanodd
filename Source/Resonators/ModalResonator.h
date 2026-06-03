/*
  ==============================================================================

    ModalResonator.h

    Shared base for plate and membrane resonators. Models the structure as a
    bank of damped modal oscillators (one RBJ band-pass biquad each). The mode
    set is the `numModes` lowest eigenfrequencies of a 2D structure indexed by
    (m, n); plate and membrane differ only in the frequency law (modeFrequency).

    For an input forced at (inX,inY) and read at (outX,outY):
        out = sum_i  phi_i(in) * phi_i(out) * H_i(in_signal)
    where phi_i is the mode shape and H_i is the i-th resonant biquad.

    Subclass responsibilities:
      - modeFrequency(m, n, fundamental, aspect): the eigenfrequency law.
      - modeShape (optional): defaults to the simply-supported rectangular
        shape sin(m*pi*x) sin(n*pi*y); override for other geometries.

  ==============================================================================
*/

#pragma once

#include "Resonator.h"
#include "Biquad.h"

class ModalResonator : public Resonator
{
public:
    static constexpr int maxModes = 128;

    float processSample (float input) override;

    void addParametersToLayout (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                                const juce::String& prefix) override;
    void assignParameters (ParamSource& apvts, const juce::String& prefix) override;
    void checkParameters() override;

protected:
    void prepareImpl (const juce::dsp::ProcessSpec& spec) override;
    void resetImpl() override;

    static constexpr int maxCandidates = 1024;

    // One candidate mode: its frequency (Hz) and geometric amplitude, i.e. the
    // mode-shape product phi_in * phi_out (damping-independent).
    struct Candidate { float freq; float amp; };

    // Build the candidate mode set into `out` (at most `maxOut`); return the
    // count. The base selects the lowest `numModes` audible candidates from it,
    // so subclasses may emit more (sorting/range filtering happens in the base).
    // Default: the rectangular 2D set (plate/membrane). 1D subclasses override.
    virtual int collectCandidates (float fundamental, Candidate* out, int maxOut) const;

    // Rectangular 2D mode set helper, shared by plate/membrane: indexes modes by
    // (m, n), uses modeFrequency + modeShape + the aspect / in-out positions.
    int collect2DCandidates (float fundamental, Candidate* out, int maxOut) const;

    // Eigenfrequency of mode (m, n) in Hz, given the fundamental and aspect ratio
    // (b/a >= 1). Rectangular subclasses override; ignored by subclasses that
    // override collectCandidates.
    virtual float modeFrequency (int /*m*/, int /*n*/, float fundamental, float /*aspectRatio*/) const { return fundamental; }
    // Mode shape value at normalised position (x, y) in [0,1]^2 (rectangular).
    float modeShape (int m, int n, float x, float y) const;
    // Per-mode level compensation for the band-pass bandwidth varying with zeta.
    // Subclasses with dense mode spectra (membrane) override to boost more
    // aggressively at low damping where mode interference reduces total RMS.
    virtual float modeGainCompensation (float zeta) const noexcept;

    // Subclasses call these for the shared modal set (modes + resonance). The
    // geometry parameters (aspect / positions) are added through the geometry
    // hooks below so 1D subclasses can substitute their own.
    void addModalParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix);
    void assignModalParameters (ParamSource& apvts, const juce::String& prefix);
    // Returns true if anything affecting the mode set changed.
    bool checkModalParameters();

    // Geometry parameter hooks. Default: the rectangular set (aspect + in/out X/Y).
    // 1D subclasses override all three to swap in their own controls.
    virtual void addGeometryParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix);
    virtual void assignGeometryParameters (ParamSource& apvts, const juce::String& prefix);
    virtual bool checkGeometryParameters();   // returns true if the geometry changed

    int numModesValue() const noexcept { return numModes; }   // for subclass candidate bounds

private:
    void recomputeModes();
    void updateDamping();      // cheap: re-tune only the Q of the active modes
    float modeDamping (float freq, float fundamental) const;
    void refreshCurrentDamping (float gate);

    std::array<Biquad, maxModes> filters {};
    std::array<float,  maxModes> amp {};        // ampGeom[k] * gain compensation for current damping
    std::array<float,  maxModes> ampGeom {};    // mode shape product phi_in * phi_out (damping-independent)
    std::array<float,  maxModes> modeFreq {};   // active mode frequencies (for Q re-tuning)
    std::array<Candidate, maxCandidates> candidates {};
    int activeModes { 0 };

    // Parameters. Damping and damping-slope each have a note-on and note-off
    // value, crossfaded by the gate (curDamp0/curDamp1 are the current blend).
    int   numModes { 32 };
    float aspect   { 1.0f };   // b/a, in [1,5]
    float damp0On  { 1.0e-4f }, damp0Off { 1.0e-2f };   // constant damping ratio
    float damp1On  { 1.0e-4f }, damp1Off { 1.0e-3f };   // damping growth with frequency
    float curDamp0 { 1.0e-4f }, curDamp1 { 1.0e-4f };
    float inX { 0.30f }, inY { 0.30f }, outX { 0.70f }, outY { 0.60f };

    float gateAtUpdate { -1.0f };   // gate value when damping was last recomputed

    std::atomic<float>* pNumModes    { nullptr };
    std::atomic<float>* pAspect      { nullptr };
    std::atomic<float>* pResOn       { nullptr };
    std::atomic<float>* pResOff      { nullptr };
    std::atomic<float>* pResSlopeOn  { nullptr };
    std::atomic<float>* pResSlopeOff { nullptr };
    std::atomic<float>* pInX  { nullptr };
    std::atomic<float>* pInY  { nullptr };
    std::atomic<float>* pOutX { nullptr };
    std::atomic<float>* pOutY { nullptr };
};

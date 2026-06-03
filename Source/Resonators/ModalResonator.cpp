/*
  ==============================================================================

    ModalResonator.cpp

  ==============================================================================
*/

#include "ModalResonator.h"
#include "../Modulation/ParamSource.h"
#include <algorithm>

void ModalResonator::prepareImpl (const juce::dsp::ProcessSpec&)
{
    for (auto& f : filters)
        f.reset();
    recomputeModes();
}

void ModalResonator::resetImpl()
{
    for (auto& f : filters)
        f.reset();
}

float ModalResonator::modeShape (int m, int n, float x, float y) const
{
    constexpr float pi = juce::MathConstants<float>::pi;
    // Normalised simply-supported rectangular eigenfunction (peak 2 on unit square).
    return 2.0f * std::sin ((float) m * pi * x) * std::sin ((float) n * pi * y);
}

float ModalResonator::modeDamping (float freq, float fundamental) const
{
    const float ratio = (fundamental > 0.0f) ? (freq / fundamental) : 1.0f;
    const float zeta = curDamp0 + curDamp1 * (ratio - 1.0f);
    return juce::jlimit (1.0e-6f, 0.99f, zeta);
}

// A 0 dB-peak band-pass passes energy proportional to its bandwidth (~ zeta),
// so low-damping (narrow) modes contribute far less output than wide ones. We
// scale each mode's amplitude by sqrt(zetaRef / zeta) to keep the perceived
// level roughly constant across the damping range. The clamp window must be
// wide enough to cover the full dB-resonance parameter range (0..100 dB →
// zeta 1e-1..1e-6), where the exact compensation spans 0.316x .. 100x.
// Subclasses with dense mode spectra (membrane) override this for a stronger
// curve.
float ModalResonator::modeGainCompensation (float zeta) const noexcept
{
    constexpr float zetaRef = 0.01f;
    const float g = std::sqrt (zetaRef / juce::jmax (zeta, 1.0e-7f));
    return juce::jlimit (0.5f, 32.0f, g);
}

void ModalResonator::refreshCurrentDamping (float gate)
{
    curDamp0 = damp0Off + gate * (damp0On - damp0Off);
    curDamp1 = damp1Off + gate * (damp1On - damp1Off);
    gateAtUpdate = gate;
}

void ModalResonator::updateDamping()
{
    const float fundamental = getTunedFrequency();
    for (int k = 0; k < activeModes; ++k)
    {
        const float zeta = modeDamping (modeFreq[(size_t) k], fundamental);
        const float Q    = juce::jlimit (0.5f, 1.0e5f, 1.0f / (2.0f * zeta));
        filters[(size_t) k].setBandPass (spec.sampleRate, modeFreq[(size_t) k], Q);
        amp[(size_t) k]  = ampGeom[(size_t) k] * modeGainCompensation (zeta);
    }
}

int ModalResonator::collectCandidates (float fundamental, Candidate* out, int maxOut) const
{
    return collect2DCandidates (fundamental, out, maxOut);
}

int ModalResonator::collect2DCandidates (float fundamental, Candidate* out, int maxOut) const
{
    // Generate candidate (m, n) modes. Frequency grows with m and with n/aspect,
    // so a bounded grid contains the lowest `numModes` modes with margin.
    const int kmax = (int) std::ceil (std::sqrt ((double) juce::jmax (1, numModes))) + 2;
    const int mmax = kmax;
    const int nmax = juce::jmin (maxOut / juce::jmax (1, mmax),
                                 (int) std::ceil (kmax * aspect) + 2);

    int count = 0;
    for (int m = 1; m <= mmax && count < maxOut; ++m)
        for (int n = 1; n <= nmax && count < maxOut; ++n)
            out[count++] = { modeFrequency (m, n, fundamental, aspect),
                             modeShape (m, n, inX, inY) * modeShape (m, n, outX, outY) };

    return count;
}

void ModalResonator::recomputeModes()
{
    refreshCurrentDamping (currentGate());

    const float fundamental = getTunedFrequency();
    const double nyquist = spec.sampleRate * 0.49;

    int count = collectCandidates (fundamental, candidates.data(), maxCandidates);
    count = juce::jlimit (0, maxCandidates, count);

    std::sort (candidates.begin(), candidates.begin() + count,
               [] (const Candidate& a, const Candidate& b) { return a.freq < b.freq; });

    activeModes = 0;
    for (int i = 0; i < count && activeModes < numModes; ++i)
    {
        const auto& c = candidates[(size_t) i];
        if (c.freq < 20.0f || c.freq > (float) nyquist)
            continue;

        const float zeta = modeDamping (c.freq, fundamental);
        const float Q    = juce::jlimit (0.5f, 1.0e5f, 1.0f / (2.0f * zeta));

        filters[(size_t) activeModes].setBandPass (spec.sampleRate, c.freq, Q);
        ampGeom[(size_t) activeModes]  = c.amp;
        amp[(size_t) activeModes]      = ampGeom[(size_t) activeModes] * modeGainCompensation (zeta);
        modeFreq[(size_t) activeModes] = c.freq;
        ++activeModes;
    }
}

float ModalResonator::processSample (float input)
{
    // Morph damping between the note-off and note-on values. Re-tuning every
    // mode is costly, so only do it when the gate has moved by a small step.
    // The step has to be fine: each re-tune rewrites every biquad's coefficients
    // while they ring, so coarse steps grain/click the decay. 0.005 keeps the
    // gate's worth of updates spread thinly over the release (and over the short
    // note-on attack ramp) so the Q glides instead of jumping.
    const float g = advanceGate();
    if (std::abs (g - gateAtUpdate) > 0.005f)
    {
        refreshCurrentDamping (g);
        updateDamping();
    }

    float y = 0.0f;
    for (int k = 0; k < activeModes; ++k)
        y += amp[(size_t) k] * filters[(size_t) k].process (input);
    return y;
}

void ModalResonator::addParametersToLayout (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                                            const juce::String& prefix)
{
    addCommonParameters (params, prefix);
    addGateParameters (params, prefix);
    addModalParameters (params, prefix);
}

void ModalResonator::assignParameters (ParamSource& apvts, const juce::String& prefix)
{
    assignCommonParameters (apvts, prefix);
    assignGateParameters (apvts, prefix);
    assignModalParameters (apvts, prefix);
}

void ModalResonator::checkParameters()
{
    const bool tuningChanged = checkCommonParameters();
    checkGateParameters();
    const bool modalChanged  = checkModalParameters();
    if (tuningChanged || modalChanged)
        recomputeModes();
}

void ModalResonator::addModalParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix)
{
    using FloatParam = juce::AudioParameterFloat;

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        makeId (prefix, "modes"), prefix + " Modes", 1, maxModes, 32));

    // Resonance is exposed as a linear dB scale (0..100): 0 dB corresponds to
    // damping = 1e-1 (very damped), 100 dB to damping = 1e-6 (essentially
    // permanent ring). Internally we convert back to a damping ratio in
    // checkModalParameters via 1e-1 * 10^(-dB/20). Linear-in-dB gives an even
    // perceptual response across the whole knob travel.
    juce::NormalisableRange<float> resRange (0.0f, 100.0f, 0.1f);
    const auto resAttrs = juce::AudioParameterFloatAttributes()
                              .withLabel ("dB")
                              .withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1); });

    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "resOn"),       prefix + " Res On",        resRange, 60.0f, resAttrs));
    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "resOff"),      prefix + " Res Off",       resRange, 20.0f, resAttrs));
    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "resSlopeOn"),  prefix + " Res Slope On",  resRange, 60.0f, resAttrs));
    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "resSlopeOff"), prefix + " Res Slope Off", resRange, 40.0f, resAttrs));

    addGeometryParameters (params, prefix);
}

void ModalResonator::addGeometryParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix)
{
    using FloatParam = juce::AudioParameterFloat;

    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "aspect"), prefix + " Aspect",
        juce::NormalisableRange<float> (1.0f, 5.0f, 1e-2f), 1.0f));
    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "inX"),  prefix + " In X",  juce::NormalisableRange<float> (0.0f, 1.0f, 1e-3f), 0.30f));
    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "inY"),  prefix + " In Y",  juce::NormalisableRange<float> (0.0f, 1.0f, 1e-3f), 0.30f));
    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "outX"), prefix + " Out X", juce::NormalisableRange<float> (0.0f, 1.0f, 1e-3f), 0.70f));
    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "outY"), prefix + " Out Y", juce::NormalisableRange<float> (0.0f, 1.0f, 1e-3f), 0.60f));
}

void ModalResonator::assignModalParameters (ParamSource& apvts, const juce::String& prefix)
{
    pNumModes    = apvts.getRawParameterValue (makeId (prefix, "modes"));
    pResOn       = apvts.getRawParameterValue (makeId (prefix, "resOn"));
    pResOff      = apvts.getRawParameterValue (makeId (prefix, "resOff"));
    pResSlopeOn  = apvts.getRawParameterValue (makeId (prefix, "resSlopeOn"));
    pResSlopeOff = apvts.getRawParameterValue (makeId (prefix, "resSlopeOff"));
    assignGeometryParameters (apvts, prefix);
}

void ModalResonator::assignGeometryParameters (ParamSource& apvts, const juce::String& prefix)
{
    pAspect = apvts.getRawParameterValue (makeId (prefix, "aspect"));
    pInX    = apvts.getRawParameterValue (makeId (prefix, "inX"));
    pInY    = apvts.getRawParameterValue (makeId (prefix, "inY"));
    pOutX   = apvts.getRawParameterValue (makeId (prefix, "outX"));
    pOutY   = apvts.getRawParameterValue (makeId (prefix, "outY"));
}

bool ModalResonator::checkModalParameters()
{
    if (pNumModes == nullptr)
        return false;

    // dB -> damping ratio: 0 dB = 1e-1 (heavy), 100 dB = 1e-6 (resonant).
    const auto dbToDamping = [] (float dB) { return 0.1f * std::pow (10.0f, -dB / 20.0f); };

    const int   newNumModes = (int) pNumModes->load();
    const float newDamp0On  = dbToDamping (pResOn      ->load());
    const float newDamp0Off = dbToDamping (pResOff     ->load());
    const float newDamp1On  = dbToDamping (pResSlopeOn ->load());
    const float newDamp1Off = dbToDamping (pResSlopeOff->load());

    bool changed =
           newNumModes != numModes
        || ! juce::approximatelyEqual (newDamp0On,  damp0On)
        || ! juce::approximatelyEqual (newDamp0Off, damp0Off)
        || ! juce::approximatelyEqual (newDamp1On,  damp1On)
        || ! juce::approximatelyEqual (newDamp1Off, damp1Off);

    numModes = newNumModes;
    damp0On = newDamp0On; damp0Off = newDamp0Off;
    damp1On = newDamp1On; damp1Off = newDamp1Off;

    changed |= checkGeometryParameters();
    return changed;
}

bool ModalResonator::checkGeometryParameters()
{
    if (pAspect == nullptr)
        return false;

    const float newAspect = pAspect->load();
    const float newInX  = pInX->load();
    const float newInY  = pInY->load();
    const float newOutX = pOutX->load();
    const float newOutY = pOutY->load();

    const bool changed =
           ! juce::approximatelyEqual (newAspect, aspect)
        || ! juce::approximatelyEqual (newInX,  inX)
        || ! juce::approximatelyEqual (newInY,  inY)
        || ! juce::approximatelyEqual (newOutX, outX)
        || ! juce::approximatelyEqual (newOutY, outY);

    aspect = newAspect;
    inX = newInX; inY = newInY; outX = newOutX; outY = newOutY;
    return changed;
}

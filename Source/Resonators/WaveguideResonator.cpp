/*
  ==============================================================================

    WaveguideResonator.cpp

  ==============================================================================
*/

#include "WaveguideResonator.h"
#include "../Modulation/ParamSource.h"

namespace
{
    // Group delay (samples) of a first-order all-pass (a + z^-1)/(1 + a z^-1) at w.
    inline float allpassGroupDelay (float a, float w) noexcept
    {
        return (1.0f - a * a) / (1.0f + 2.0f * a * std::cos (w) + a * a);
    }
}

void WaveguideResonator::prepareImpl (const juce::dsp::ProcessSpec& s)
{
    const int maxDelay = (int) (s.sampleRate / minFrequency) + 8;

    for (auto* dl : { &upA, &upB, &dnB, &dnA })
    {
        dl->setMaximumDelayInSamples (maxDelay);
        dl->prepare (s);
        dl->reset();
    }

    lpState = 0.0f;
    apState.fill (0.0f);

    // ~5 ms one-pole smoothing for the per-sample geometry glide.
    constexpr float geoSmoothSeconds = 0.005f;
    geoSmoothCoeff = 1.0f - std::exp (-1.0f / (geoSmoothSeconds * (float) s.sampleRate));

    snapGeometry = true;
    updateGeometry();
}

void WaveguideResonator::resetImpl()
{
    upA.reset(); upB.reset(); dnB.reset(); dnA.reset();
    lpState = 0.0f;
    apState.fill (0.0f);

    // Next geometry update jumps straight to the new note's tuning (no glide).
    snapGeometry = true;
}

void WaveguideResonator::updateGeometry()
{
    const float f  = juce::jlimit (minFrequency, (float) (spec.sampleRate * 0.45), getTunedFrequency());
    const float w0 = juce::MathConstants<float>::twoPi * f / (float) spec.sampleRate;

    // Normalise the per-round-trip feedback to the played frequency. Gain: raise to
    // (f_ref / f) so the per-second decay (T60) is pitch-independent - at low f there
    // are fewer round trips per second, so each must lose more. Cutoff: scale by
    // (f / f_ref) so it stays at a fixed harmonic ratio, keeping timbre consistent.
    const float gExp   = referenceFrequency / f;
    const float cRatio = f / referenceFrequency;
    fbGainOnNorm    = std::pow (juce::jlimit (0.0f, 1.0f, fbGainOn),  gExp);
    fbGainOffNorm   = std::pow (juce::jlimit (0.0f, 1.0f, fbGainOff), gExp);
    fbCutoffOnNorm  = fbCutoffOn  * cRatio;
    fbCutoffOffNorm = fbCutoffOff * cRatio;

    // (lpCoeff is recomputed per-sample in processSample from the gated cutoff.)

    // Dispersion all-pass coefficient (negative -> high partials less delayed -> stiffness).
    // Inharmonicity scales steeply with |apCoeff| (the per-section group delay goes like
    // (1-a)/(1+a) near the fundamental), so we drive it hard; the clamp below is what
    // keeps the loop in tune rather than a timid coefficient.
    float ap = -0.85f * juce::jlimit (0.0f, 1.0f, dispersion);

    const float roundTrip = (float) spec.sampleRate / f;

    // The chain's delay is compensated out of the rail length to keep the pitch; but on
    // short round trips (high notes) it can exceed the whole loop and detune it. Cap the
    // chain to a fraction of the round trip by clamping the coefficient to the largest
    // magnitude that fits: from N*(1-a)/(1+a) <= maxDisp, a >= (1-r)/(1+r) with r = maxDisp/N.
    const float maxDisp = 0.6f * roundTrip;
    const float r       = maxDisp / (float) numAllpass;
    const float aCap    = juce::jmin (0.0f, (1.0f - r) / (1.0f + r));
    ap = juce::jmax (ap, aCap);
    targetApCoeff = ap;

    // The chain's *frequency dependence* (more delay low, less high) is what bends the
    // partials; its delay at the fundamental is compensated out so the pitch stays put.
    const float dispDelay = (float) numAllpass * allpassGroupDelay (ap, w0);
    const float lpDelay   = 1.0f;
    targetOneWayLength = juce::jmax (8.0f, 0.5f * (roundTrip - dispDelay - lpDelay));

    cInPos  = juce::jlimit (0.02f, 0.98f, inPos);
    cOutPos = juce::jlimit (0.02f, 0.98f, outPos);

    // Note start / prepare: jump straight to the target so the new note is in tune
    // from sample 0. Otherwise processSample glides there.
    if (snapGeometry)
    {
        oneWayLength = targetOneWayLength;
        apCoeff      = targetApCoeff;
        snapGeometry = false;
    }
}

float WaveguideResonator::processSample (float input)
{
    // Glide the pitch-determining length and the dispersion coefficient toward
    // their targets so rapid tuning changes (sweeps, portamento) stay click-free;
    // derive the rail split from the smoothed length.
    oneWayLength += geoSmoothCoeff * (targetOneWayLength - oneWayLength);
    apCoeff      += geoSmoothCoeff * (targetApCoeff - apCoeff);

    const float L    = oneWayLength;
    const float lenA = juce::jlimit (2.0f, L - 2.0f, cInPos * L);
    const float lenB = juce::jmax (2.0f, L - lenA);

    // Crossfade feedback gain / cutoff between the note-off and note-on values.
    const float g           = advanceGate();
    const float curFbGain   = fbGainOffNorm   + g * (fbGainOnNorm   - fbGainOffNorm);
    const float curFbCutoff = fbCutoffOffNorm + g * (fbCutoffOnNorm - fbCutoffOffNorm);
    if (! juce::approximatelyEqual (curFbCutoff, lastLpCutoff))
    {
        const float fc = juce::jlimit (50.0f, (float) (spec.sampleRate * 0.45), curFbCutoff);
        lpCoeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * fc / (float) spec.sampleRate);
        lastLpCutoff = curFbCutoff;
    }

    // 1) Bridge reflection: right-going arriving via upB -> low-pass -> dispersion -> left-going.
    const float rb = upB.popSample (0, lenB);
    lpState += lpCoeff * (rb - lpState);
    float bridge = -curFbGain * lpState;
    for (int k = 0; k < numAllpass; ++k)
    {
        const float v = bridge - apCoeff * apState[(size_t) k];
        const float y = apCoeff * v + apState[(size_t) k];
        apState[(size_t) k] = v;
        bridge = y;
    }
    dnB.pushSample (0, bridge);

    // 2) Input node: waves pass through; inject half the force into each direction.
    const float la  = dnB.popSample (0, lenB);   // left-going arriving from bridge
    const float ra  = upA.popSample (0, lenA);   // right-going arriving from nut
    const float inj = 0.5f * input;
    upB.pushSample (0, ra + inj);
    dnA.pushSample (0, la + inj);

    // 3) Nut: rigid reflection (sign inversion).
    const float ln = dnA.popSample (0, lenA);
    upA.pushSample (0, -ln);

    // 4) Output tap (no scattering): right-going + left-going at outPos.
    if (cOutPos <= cInPos)
    {
        const float ageRight = juce::jlimit (0.0f, lenA, cOutPos * L);
        const float ageLeft  = juce::jlimit (0.0f, lenA, (cInPos - cOutPos) * L);
        return upA.popSample (0, ageRight, false) + dnA.popSample (0, ageLeft, false);
    }

    const float ageRight = juce::jlimit (0.0f, lenB, (cOutPos - cInPos) * L);
    const float ageLeft  = juce::jlimit (0.0f, lenB, (1.0f - cOutPos) * L);
    return upB.popSample (0, ageRight, false) + dnB.popSample (0, ageLeft, false);
}

void WaveguideResonator::addParametersToLayout (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                                                const juce::String& prefix)
{
    using FloatParam = juce::AudioParameterFloat;

    addCommonParameters (params, prefix);
    addGateParameters (params, prefix);

    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "fbGainOn"), prefix + " Feedback On",
        juce::NormalisableRange<float> (0.0f, 1.0f, 1e-4f, 3.0f), 0.995f));
    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "fbGainOff"), prefix + " Feedback Off",
        juce::NormalisableRange<float> (0.0f, 1.0f, 1e-4f, 3.0f), 0.9f));
    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "fbCutoffOn"), prefix + " FB Cutoff On",
        juce::NormalisableRange<float> (200.0f, 20000.0f, 0.1f, 0.25f), 8000.0f));
    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "fbCutoffOff"), prefix + " FB Cutoff Off",
        juce::NormalisableRange<float> (200.0f, 20000.0f, 0.1f, 0.25f), 2000.0f));
    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "inPos"),  prefix + " In Pos",  juce::NormalisableRange<float> (0.02f, 0.98f, 1e-3f), 0.12f));
    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "outPos"), prefix + " Out Pos", juce::NormalisableRange<float> (0.02f, 0.98f, 1e-3f), 0.85f));
    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "dispersion"), prefix + " Dispersion",
        juce::NormalisableRange<float> (0.0f, 1.0f, 1e-3f), 0.0f));
}

void WaveguideResonator::assignParameters (ParamSource& apvts, const juce::String& prefix)
{
    assignCommonParameters (apvts, prefix);
    assignGateParameters (apvts, prefix);
    pFbGainOn    = apvts.getRawParameterValue (makeId (prefix, "fbGainOn"));
    pFbGainOff   = apvts.getRawParameterValue (makeId (prefix, "fbGainOff"));
    pFbCutoffOn  = apvts.getRawParameterValue (makeId (prefix, "fbCutoffOn"));
    pFbCutoffOff = apvts.getRawParameterValue (makeId (prefix, "fbCutoffOff"));
    pInPos       = apvts.getRawParameterValue (makeId (prefix, "inPos"));
    pOutPos      = apvts.getRawParameterValue (makeId (prefix, "outPos"));
    pDispersion  = apvts.getRawParameterValue (makeId (prefix, "dispersion"));
}

void WaveguideResonator::checkParameters()
{
    checkCommonParameters();
    checkGateParameters();

    if (pFbGainOn != nullptr)
    {
        fbGainOn    = pFbGainOn->load();
        fbGainOff   = pFbGainOff->load();
        fbCutoffOn  = pFbCutoffOn->load();
        fbCutoffOff = pFbCutoffOff->load();
        inPos       = pInPos->load();
        outPos      = pOutPos->load();
        dispersion  = pDispersion->load();
    }

    lastLpCutoff = -1.0f;   // force lpCoeff recompute on next sample
    updateGeometry();
}

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
    updateGeometry();
}

void WaveguideResonator::resetImpl()
{
    upA.reset(); upB.reset(); dnB.reset(); dnA.reset();
    lpState = 0.0f;
    apState.fill (0.0f);
}

void WaveguideResonator::updateGeometry()
{
    const float f  = juce::jlimit (minFrequency, (float) (spec.sampleRate * 0.45), getTunedFrequency());
    const float w0 = juce::MathConstants<float>::twoPi * f / (float) spec.sampleRate;

    // (lpCoeff is recomputed per-sample in processSample from the gated cutoff.)

    // Dispersion all-pass coefficient (negative -> high partials less delayed -> stiffness).
    apCoeff = -0.4f * juce::jlimit (0.0f, 1.0f, dispersion);

    // Compensate loop filter delay so the round trip stays at fs/f.
    const float dispDelay = (float) numAllpass * allpassGroupDelay (apCoeff, w0);
    const float lpDelay   = 1.0f;
    const float roundTrip = (float) spec.sampleRate / f;

    oneWayLength = juce::jmax (8.0f, 0.5f * (roundTrip - dispDelay - lpDelay));

    cInPos  = juce::jlimit (0.02f, 0.98f, inPos);
    cOutPos = juce::jlimit (0.02f, 0.98f, outPos);
    lenA = juce::jlimit (2.0f, oneWayLength - 2.0f, cInPos * oneWayLength);
    lenB = juce::jmax (2.0f, oneWayLength - lenA);
}

float WaveguideResonator::processSample (float input)
{
    const float L = oneWayLength;

    // Crossfade feedback gain / cutoff between the note-off and note-on values.
    const float g           = advanceGate();
    const float curFbGain   = fbGainOff   + g * (fbGainOn   - fbGainOff);
    const float curFbCutoff = fbCutoffOff + g * (fbCutoffOn - fbCutoffOff);
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

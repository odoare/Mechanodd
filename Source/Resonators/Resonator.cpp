/*
  ==============================================================================

    Resonator.cpp

  ==============================================================================
*/

#include "Resonator.h"
#include "../Modulation/ParamSource.h"

void Resonator::prepare (const juce::dsp::ProcessSpec& newSpec)
{
    spec = newSpec;
    prepareImpl (spec);
}

void Resonator::reset()
{
    resetImpl();
}

void Resonator::processBlock (const float* input, float* output, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
        output[i] = processSample (input[i]);
}

void Resonator::setBaseFrequency (float hz)
{
    baseFrequency = hz;
}

void Resonator::noteOn()
{
    // Snap straight to the "on" tuning. The note starts on a freshly reset (zero
    // energy) loop / filter bank, so the parameter jump has no signal to act on -
    // there is no click to smooth here. Snapping is also what keeps the level
    // consistent: the source's attack transient is always injected into the loop
    // at full feedback / resonance, regardless of how far the gate had released
    // since the previous note. Ramping the gate up instead made the captured
    // energy (hence the sustained level) depend on the gate's starting value, so
    // a quick retrigger - gate still high - rang far louder than a fresh note.
    gateValue  = 1.0f;
    gateTarget = 1.0f;
}

void Resonator::noteOff()
{
    gateTarget = 0.0f;
}

float Resonator::advanceGate() noexcept
{
    // Only the release ramps (exponential, over the Note Rel time, doubling as
    // the musical decay); note-on snaps in noteOn().
    gateValue += gateRelCoeff * (gateTarget - gateValue);
    return gateValue;
}

float Resonator::getTunedFrequency() const
{
    return baseFrequency * std::pow (2.0f, (coarse + fine) / 12.0f);
}

void Resonator::addCommonParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                                     const juce::String& prefix)
{
    using FloatParam = juce::AudioParameterFloat;
    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "coarse"), prefix + " Coarse",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 1.0f), 0.0f));
    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "fine"),   prefix + " Fine",
        juce::NormalisableRange<float> (-1.0f, 1.0f, 1e-3f), 0.0f));
}

void Resonator::assignCommonParameters (ParamSource& apvts, const juce::String& prefix)
{
    pFine   = apvts.getRawParameterValue (makeId (prefix, "fine"));
    pCoarse = apvts.getRawParameterValue (makeId (prefix, "coarse"));
}

bool Resonator::checkCommonParameters()
{
    if (pFine != nullptr)
    {
        fine   = pFine->load();
        coarse = pCoarse->load();
    }

    const float tuned = getTunedFrequency();
    if (! juce::approximatelyEqual (tuned, lastTuned))
    {
        lastTuned = tuned;
        return true;
    }
    return false;
}

void Resonator::addGateParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                                   const juce::String& prefix)
{
    using FloatParam = juce::AudioParameterFloat;
    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "noteRel"), prefix + " Note Rel",
        juce::NormalisableRange<float> (1.0f, 10000.0f, 0.1f, 0.3f), 300.0f));
}

void Resonator::assignGateParameters (ParamSource& apvts, const juce::String& prefix)
{
    pGateRel = apvts.getRawParameterValue (makeId (prefix, "noteRel"));
}

void Resonator::checkGateParameters()
{
    if (pGateRel == nullptr)
        return;

    const float t = juce::jmax (1.0e-4f, pGateRel->load() * 1.0e-3f);
    gateRelCoeff = 1.0f - std::exp (-1.0f / (t * (float) spec.sampleRate));
}

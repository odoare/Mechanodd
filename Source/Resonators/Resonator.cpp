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
    // Snap directly to the "on" tuning: the resonator must be in its sustaining
    // state from sample 0 so the source's attack transient sees the intended
    // damping. Morphing in over an attack ramp leaves the filters in the heavier
    // "off" state during the transient, which absorbs most of the input energy.
    gateValue  = 1.0f;
    gateTarget = 1.0f;
}

void Resonator::noteOff()
{
    gateTarget = 0.0f;
}

float Resonator::advanceGate() noexcept
{
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

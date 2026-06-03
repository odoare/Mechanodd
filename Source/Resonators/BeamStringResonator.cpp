/*
  ==============================================================================

    BeamStringResonator.cpp

  ==============================================================================
*/

#include "BeamStringResonator.h"
#include "../Modulation/ParamSource.h"

int BeamStringResonator::collectCandidates (float fundamental, Candidate* out, int maxOut) const
{
    constexpr float pi    = juce::MathConstants<float>::pi;
    constexpr float sqrt2 = juce::MathConstants<float>::sqrt2;
    const float nyquist   = (float) (spec.sampleRate * 0.49);
    const float b         = juce::jlimit (0.0f, 1.0f, mix);

    // 1D pinned-pinned modes are indexed by a single n; frequency is monotone in
    // n, so we walk upwards and stop once past Nyquist (the base keeps the lowest
    // numModes). Mode shape phi_n(x) = sqrt(2) sin(n*pi*x), identical for string,
    // beam and every blend in between.
    int count = 0;
    for (int n = 1; n <= maxOut; ++n)
    {
        const float fn = fundamental * (float) n * std::sqrt ((1.0f - b) + b * (float) n * (float) n);

        const float phiIn  = sqrt2 * std::sin ((float) n * pi * inPos);
        const float phiOut = sqrt2 * std::sin ((float) n * pi * outPos);
        out[count++] = { fn, phiIn * phiOut };

        if (fn > nyquist)
            break;
    }
    return count;
}

void BeamStringResonator::addGeometryParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix)
{
    using FloatParam = juce::AudioParameterFloat;

    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "mix"), prefix + " Str/Beam",
        juce::NormalisableRange<float> (0.0f, 1.0f, 1e-3f), 0.0f));
    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "inPos"),  prefix + " In Pos",  juce::NormalisableRange<float> (0.02f, 0.98f, 1e-3f), 0.12f));
    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "outPos"), prefix + " Out Pos", juce::NormalisableRange<float> (0.02f, 0.98f, 1e-3f), 0.85f));
}

void BeamStringResonator::assignGeometryParameters (ParamSource& apvts, const juce::String& prefix)
{
    pMix    = apvts.getRawParameterValue (makeId (prefix, "mix"));
    pInPos  = apvts.getRawParameterValue (makeId (prefix, "inPos"));
    pOutPos = apvts.getRawParameterValue (makeId (prefix, "outPos"));
}

bool BeamStringResonator::checkGeometryParameters()
{
    if (pMix == nullptr)
        return false;

    const float newMix    = pMix->load();
    const float newInPos  = pInPos->load();
    const float newOutPos = pOutPos->load();

    const bool changed =
           ! juce::approximatelyEqual (newMix,    mix)
        || ! juce::approximatelyEqual (newInPos,  inPos)
        || ! juce::approximatelyEqual (newOutPos, outPos);

    mix = newMix; inPos = newInPos; outPos = newOutPos;
    return changed;
}

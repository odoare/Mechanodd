/*
  ==============================================================================

    Source.cpp

  ==============================================================================
*/

#include "Source.h"
#include "../Modulation/ParamSource.h"

void Source::prepare (const juce::dsp::ProcessSpec& newSpec)
{
    spec = newSpec;

    adsr.setSampleRate (spec.sampleRate);
    adsr.setParameters (adsrParams);

    lpf.prepare (spec);
    lpf.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    lpf.setCutoffFrequency (cutoff);
    lpf.setResonance (resonance);

    scratch.setSize (1, (int) spec.maximumBlockSize, false, false, true);

    prepareImpl (spec);
}

void Source::noteOn (float vel)
{
    velocity = vel;
    adsr.setParameters (adsrParams);
    adsr.noteOn();
    onNoteOn();
}

void Source::noteOff()
{
    adsr.noteOff();
}

void Source::process (juce::AudioBuffer<float>& outBuffer, int startSample, int numSamples)
{
    if (! adsr.isActive())
        return;

    // 1) raw mono signal into scratch[0..numSamples)
    scratch.clear (0, 0, numSamples);
    juce::AudioBuffer<float> monoView (scratch.getArrayOfWritePointers(), 1, 0, numSamples);
    renderSource (monoView, 0, numSamples);

    // velocity-scaled level and cutoff
    const float velLevel  = juce::jmap (velToLevel,  1.0f, velocity);
    const float effLevel  = level * velLevel;
    const float velCut    = juce::jmap (velToCutoff, 1.0f, velocity);
    lpf.setCutoffFrequency (juce::jlimit (20.0f, (float) (spec.sampleRate * 0.49), cutoff * velCut));
    lpf.setResonance (resonance);

    auto* mono = scratch.getWritePointer (0);

    // 2) per-sample filter + envelope + level
    for (int i = 0; i < numSamples; ++i)
    {
        const float env = adsr.getNextSample();
        float s = lpf.processSample (0, mono[i]);
        mono[i] = s * env * effLevel;
    }

    // 3) add mono content into every output channel
    for (int ch = 0; ch < outBuffer.getNumChannels(); ++ch)
        outBuffer.addFrom (ch, startSample, mono, numSamples);
}

// ---- Common parameters ---------------------------------------------------------------

void Source::addCommonParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                                  const juce::String& prefix)
{
    using FloatParam = juce::AudioParameterFloat;
    auto sec = [] (float top) { return juce::NormalisableRange<float> (0.0f, top, 1e-3f, 0.4f); };

    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "attack"),    prefix + " Attack",    sec (10.0f), 0.01f));
    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "decay"),     prefix + " Decay",     sec (10.0f), 0.10f));
    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "sustain"),   prefix + " Sustain",   juce::NormalisableRange<float> (0.0f, 1.0f, 1e-3f), 1.0f));
    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "release"),   prefix + " Release",   sec (10.0f), 0.20f));

    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "cutoff"),    prefix + " Cutoff",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.25f), 20000.0f));
    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "resonance"), prefix + " Resonance",
        juce::NormalisableRange<float> (0.1f, 10.0f, 1e-3f, 0.3f), 0.70710678f));

    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "level"),     prefix + " Level",
        juce::NormalisableRange<float> (-60.0f, 6.0f, 1e-2f), 0.0f));   // dB

    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "velLevel"),  prefix + " Vel>Level",  juce::NormalisableRange<float> (0.0f, 1.0f, 1e-3f), 0.0f));
    params.push_back (std::make_unique<FloatParam> (makeId (prefix, "velCutoff"), prefix + " Vel>Cutoff", juce::NormalisableRange<float> (0.0f, 1.0f, 1e-3f), 0.0f));
}

void Source::assignCommonParameters (ParamSource& apvts, const juce::String& prefix)
{
    pAttack    = apvts.getRawParameterValue (makeId (prefix, "attack"));
    pDecay     = apvts.getRawParameterValue (makeId (prefix, "decay"));
    pSustain   = apvts.getRawParameterValue (makeId (prefix, "sustain"));
    pRelease   = apvts.getRawParameterValue (makeId (prefix, "release"));
    pCutoff    = apvts.getRawParameterValue (makeId (prefix, "cutoff"));
    pResonance = apvts.getRawParameterValue (makeId (prefix, "resonance"));
    pLevel     = apvts.getRawParameterValue (makeId (prefix, "level"));
    pVelLevel  = apvts.getRawParameterValue (makeId (prefix, "velLevel"));
    pVelCutoff = apvts.getRawParameterValue (makeId (prefix, "velCutoff"));
}

void Source::checkCommonParameters()
{
    if (pAttack == nullptr) return;  // not assigned

    adsrParams.attack  = pAttack->load();
    adsrParams.decay   = pDecay->load();
    adsrParams.sustain = pSustain->load();
    adsrParams.release = pRelease->load();

    cutoff      = pCutoff->load();
    resonance   = pResonance->load();
    level       = juce::Decibels::decibelsToGain (pLevel->load(), -60.0f);
    velToLevel  = pVelLevel->load();
    velToCutoff = pVelCutoff->load();
}

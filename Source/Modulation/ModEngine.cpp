/*
  ==============================================================================

    ModEngine.cpp

  ==============================================================================
*/

#include "ModEngine.h"
#include <algorithm>

void ModEngine::prepare (double sr)
{
    sampleRate = sr;
    reset();
}

void ModEngine::reset()
{
    for (auto& m : mods)
        m.phase = 0.0f;
    prevTouched.clear();
    touchedNow.clear();
}

float ModEngine::evalLfo (int shape, float phase)
{
    switch (shape)
    {
        case triangle: return phase < 0.5f ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
        case square:   return phase < 0.5f ? 1.0f : -1.0f;
        case sawUp:    return 2.0f * phase - 1.0f;
        case sawDown:  return 1.0f - 2.0f * phase;
        case sine:
        default:       return std::sin (juce::MathConstants<float>::twoPi * phase);
    }
}

float ModEngine::syncRateBeats (int index)
{
    static const float beats[] = { 4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 1.0f / 3.0f, 1.0f / 6.0f };
    const int n = (int) (sizeof (beats) / sizeof (beats[0]));
    return beats[juce::jlimit (0, n - 1, index)];
}

void ModEngine::assignParameters (juce::AudioProcessorValueTreeState& apvts)
{
    // Target names come from any modulator's target choice (built in addParameters).
    targetParams.clear();
    targetAtomics.clear();

    juce::StringArray names;
    if (auto* tparam = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (targetId (0))))
        names = tparam->choices;

    for (const auto& name : names)
    {
        if (name == "None")
        {
            targetParams.push_back (nullptr);
            targetAtomics.push_back (nullptr);
        }
        else
        {
            targetParams.push_back (apvts.getParameter (name));
            targetAtomics.push_back (apvts.getRawParameterValue (name));
        }
    }

    offset.assign (targetParams.size(), 0.0f);
    touchedNow.reserve (numModulators);
    prevTouched.reserve (numModulators);

    for (int i = 0; i < numModulators; ++i)
    {
        mods[(size_t) i].target   = apvts.getRawParameterValue (targetId (i));
        mods[(size_t) i].shape    = apvts.getRawParameterValue (shapeId (i));
        mods[(size_t) i].rate     = apvts.getRawParameterValue (rateId (i));
        mods[(size_t) i].sync     = apvts.getRawParameterValue (syncId (i));
        mods[(size_t) i].syncRate = apvts.getRawParameterValue (syncRateId (i));
        mods[(size_t) i].depth    = apvts.getRawParameterValue (depthId (i));
    }
}

void ModEngine::process (int numSamples, double bpm, double ppqPosition, bool isPlaying)
{
    if (targetParams.empty())
        return;

    touchedNow.clear();

    const float blockSeconds = (float) (numSamples / sampleRate);

    for (auto& m : mods)
    {
        if (m.target == nullptr)
            continue;

        const int idx = (int) m.target->load();
        if (idx <= 0 || idx >= (int) targetParams.size() || targetParams[(size_t) idx] == nullptr)
            continue;

        const int   shape  = (int) m.shape->load();
        const bool  synced = m.sync->load() > 0.5f;
        const float depth  = m.depth->load();

        float freq;
        if (synced)
            freq = (float) (bpm / 60.0) / juce::jmax (1.0e-4f, syncRateBeats ((int) m.syncRate->load()));
        else
            freq = juce::jmax (0.0f, m.rate->load());

        if (synced && isPlaying)
            m.phase = (float) std::fmod (ppqPosition / syncRateBeats ((int) m.syncRate->load()), 1.0);
        else
        {
            m.phase += freq * blockSeconds;
            m.phase -= std::floor (m.phase);
        }

        const float val = depth * evalLfo (shape, m.phase);

        auto found = std::find (touchedNow.begin(), touchedNow.end(), idx);
        if (found == touchedNow.end())
        {
            offset[(size_t) idx] = val;
            touchedNow.push_back (idx);
        }
        else
        {
            offset[(size_t) idx] += val;
        }
    }

    // Apply to all targets touched this block or last block (the latter get restored to base).
    auto applyTarget = [this] (int idx, float off)
    {
        auto* param  = targetParams[(size_t) idx];
        auto* atomic = targetAtomics[(size_t) idx];
        if (param == nullptr || atomic == nullptr)
            return;

        const float base01 = param->getValue();
        const float mod01  = juce::jlimit (0.0f, 1.0f, base01 + off);
        atomic->store (param->getNormalisableRange().convertFrom0to1 (mod01));
    };

    for (int idx : prevTouched)
        if (std::find (touchedNow.begin(), touchedNow.end(), idx) == touchedNow.end())
            applyTarget (idx, 0.0f);   // restore base

    for (int idx : touchedNow)
        applyTarget (idx, offset[(size_t) idx]);

    prevTouched = touchedNow;
}

void ModEngine::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                               const juce::StringArray& targetChoices)
{
    for (int i = 0; i < numModulators; ++i)
    {
        params.push_back (std::make_unique<juce::AudioParameterChoice> (targetId (i), "Mod " + juce::String (i) + " Target", targetChoices, 0));
        params.push_back (std::make_unique<juce::AudioParameterChoice> (shapeId (i), "Mod " + juce::String (i) + " Shape", shapeChoices(), 0));
        params.push_back (std::make_unique<juce::AudioParameterFloat>  (rateId (i),  "Mod " + juce::String (i) + " Rate",
            juce::NormalisableRange<float> (0.01f, 40.0f, 1e-3f, 0.3f), 1.0f));
        params.push_back (std::make_unique<juce::AudioParameterBool>   (syncId (i),  "Mod " + juce::String (i) + " Sync", false));
        params.push_back (std::make_unique<juce::AudioParameterChoice> (syncRateId (i), "Mod " + juce::String (i) + " Sync Rate", syncRateChoices(), 2));
        params.push_back (std::make_unique<juce::AudioParameterFloat>  (depthId (i), "Mod " + juce::String (i) + " Depth",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 1e-3f), 0.0f));
    }
}

/*
  ==============================================================================

    ModEngine.cpp

  ==============================================================================
*/

#include "ModEngine.h"
#include "../Resonators/ResonatorSlot.h"
#include <algorithm>

juce::String ModEngine::resGlobalFlagId (const juce::String& id)
{
    if (! id.startsWith ("res"))
        return {};
    const auto digits = id.substring (3).initialSectionContainingOnly ("0123456789");
    if (digits.isEmpty())
        return {};
    return ResonatorSlot::globalParamId (ResonatorSlot::slotPrefix (digits.getIntValue()));
}

void ModEngine::prepare (double sr)
{
    sampleRate = sr;
    for (auto& m : mods)
        m.adsr.setSampleRate (sr);
    reset();
}

void ModEngine::reset()
{
    for (auto& m : mods)
    {
        m.phase = 0.0f;
        m.adsr.reset();
    }
    prevTouched.clear();
    touchedNow.clear();
    prevGate = false;
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
    perVoiceTarget.clear();
    targetResGlobal.clear();

    juce::StringArray names;
    if (auto* tparam = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (targetId (0))))
        names = tparam->choices;

    for (const auto& name : names)
    {
        if (name == "None")
        {
            targetParams.push_back (nullptr);
            targetAtomics.push_back (nullptr);
            perVoiceTarget.push_back (false);
            targetResGlobal.push_back (nullptr);
        }
        else
        {
            targetParams.push_back (apvts.getParameter (name));
            targetAtomics.push_back (apvts.getRawParameterValue (name));
            perVoiceTarget.push_back (isPerVoiceTarget (name));

            const auto flagId = resGlobalFlagId (name);
            targetResGlobal.push_back (flagId.isEmpty() ? nullptr : apvts.getRawParameterValue (flagId));
        }
    }

    offset.assign (targetParams.size(), 0.0f);
    touchedNow.reserve (numModulators);
    prevTouched.reserve (numModulators);

    for (int i = 0; i < numModulators; ++i)
    {
        mods[(size_t) i].target   = apvts.getRawParameterValue (targetId (i));
        mods[(size_t) i].type     = apvts.getRawParameterValue (typeId (i));
        mods[(size_t) i].shape    = apvts.getRawParameterValue (shapeId (i));
        mods[(size_t) i].rate     = apvts.getRawParameterValue (rateId (i));
        mods[(size_t) i].sync     = apvts.getRawParameterValue (syncId (i));
        mods[(size_t) i].syncRate = apvts.getRawParameterValue (syncRateId (i));
        mods[(size_t) i].depth    = apvts.getRawParameterValue (depthId (i));
        mods[(size_t) i].depthParam = apvts.getParameter (depthId (i));
        mods[(size_t) i].attack   = apvts.getRawParameterValue (attackId (i));
        mods[(size_t) i].decay    = apvts.getRawParameterValue (decayId (i));
        mods[(size_t) i].sustain  = apvts.getRawParameterValue (sustainId (i));
        mods[(size_t) i].release  = apvts.getRawParameterValue (releaseId (i));
        mods[(size_t) i].amount   = apvts.getRawParameterValue (amountId (i));
        mods[(size_t) i].polarity = apvts.getRawParameterValue (polarityId (i));
    }
}

void ModEngine::process (int numSamples, double bpm, double ppqPosition, bool isPlaying, bool gate)
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

        const int type = (int) m.type->load();

        float val;
        if (type == typeAdsr)
        {
            // Per-voice targets are modulated by each voice's own envelope (VoiceModEngine);
            // the global engine only handles ADSR on global targets (effects) and on
            // resonators whose slot is currently in global mode.
            bool perVoiceNow = perVoiceTarget[(size_t) idx];
            if (perVoiceNow && targetResGlobal[(size_t) idx] != nullptr
                && targetResGlobal[(size_t) idx]->load() > 0.5f)
                perVoiceNow = false;   // global resonator -> handled here
            if (perVoiceNow)
                continue;

            juce::ADSR::Parameters p;
            p.attack  = juce::jmax (0.0f, m.attack->load());
            p.decay   = juce::jmax (0.0f, m.decay->load());
            p.sustain = juce::jlimit (0.0f, 1.0f, m.sustain->load());
            p.release = juce::jmax (0.0f, m.release->load());
            m.adsr.setParameters (p);

            // Gate edges, shared across all global ADSR modulators: attack on the first
            // note after silence, release when the last held note is lifted.
            if (gate && ! prevGate)
                m.adsr.noteOn();
            else if (prevGate && ! gate)
                m.adsr.noteOff();

            float env = 0.0f;
            for (int s = 0; s < numSamples; ++s)
                env = m.adsr.getNextSample();   // block-rate: use the level at block end

            const float amount   = juce::jlimit (0.0f, 1.0f, m.amount->load());
            const bool  positive = m.polarity->load() > 0.5f;
            const float base01   = targetParams[(size_t) idx]->getValue();

            val = adsrOffset (amount, positive, base01, env);
        }
        else
        {
            const int   shape  = (int) m.shape->load();
            const bool  synced = m.sync->load() > 0.5f;

            // Depth is bipolar (centre = 0 = off). When another modulator targets this
            // LFO's Depth, reinterpret the modulation as a magnitude offset with the sign
            // locked to the knob, so it swings between 0 and the set value rather than
            // dragging Depth to -1. (Same scheme as FeedbackMatrix::modulatedGain.)
            float depth = m.depth->load();
            if (m.depthParam != nullptr)
            {
                const float base01 = m.depthParam->getValue();
                const float mod01  = m.depthParam->getNormalisableRange().convertTo0to1 (depth);
                depth = modulatedDepth (base01, mod01);
            }

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

            val = depth * evalLfo (shape, m.phase);
        }

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
    prevGate    = gate;
}

void ModEngine::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                               const juce::StringArray& targetChoices)
{
    for (int i = 0; i < numModulators; ++i)
    {
        const juce::String pfx = "Mod " + juce::String (i) + " ";

        params.push_back (std::make_unique<juce::AudioParameterChoice> (targetId (i), pfx + "Target", targetChoices, 0));
        params.push_back (std::make_unique<juce::AudioParameterChoice> (typeId (i),   pfx + "Type", typeChoices(), 0));

        // LFO controls.
        params.push_back (std::make_unique<juce::AudioParameterChoice> (shapeId (i), pfx + "Shape", shapeChoices(), 0));
        params.push_back (std::make_unique<juce::AudioParameterFloat>  (rateId (i),  pfx + "Rate",
            juce::NormalisableRange<float> (0.01f, 40.0f, 1e-3f, 0.3f), 1.0f));
        params.push_back (std::make_unique<juce::AudioParameterBool>   (syncId (i),  pfx + "Sync", false));
        params.push_back (std::make_unique<juce::AudioParameterChoice> (syncRateId (i), pfx + "Sync Rate", syncRateChoices(), 2));
        params.push_back (std::make_unique<juce::AudioParameterFloat>  (depthId (i), pfx + "Depth",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 1e-3f), 0.0f));

        // ADSR controls.
        params.push_back (std::make_unique<juce::AudioParameterFloat>  (attackId (i),  pfx + "Attack",
            juce::NormalisableRange<float> (0.0f, 10.0f, 1e-3f, 0.3f), 0.01f));
        params.push_back (std::make_unique<juce::AudioParameterFloat>  (decayId (i),   pfx + "Decay",
            juce::NormalisableRange<float> (0.0f, 10.0f, 1e-3f, 0.3f), 0.1f));
        params.push_back (std::make_unique<juce::AudioParameterFloat>  (sustainId (i), pfx + "Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 1e-3f), 0.8f));
        params.push_back (std::make_unique<juce::AudioParameterFloat>  (releaseId (i), pfx + "Release",
            juce::NormalisableRange<float> (0.0f, 10.0f, 1e-3f, 0.3f), 0.2f));
        params.push_back (std::make_unique<juce::AudioParameterFloat>  (amountId (i),  pfx + "Amount",
            juce::NormalisableRange<float> (0.0f, 1.0f, 1e-3f), 1.0f));
        params.push_back (std::make_unique<juce::AudioParameterChoice> (polarityId (i), pfx + "Polarity", polarityChoices(), 0));
    }
}

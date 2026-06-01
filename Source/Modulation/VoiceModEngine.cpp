/*
  ==============================================================================

    VoiceModEngine.cpp

  ==============================================================================
*/

#include "VoiceModEngine.h"
#include "ParamSource.h"

void VoiceModEngine::prepare (double sr)
{
    sampleRate = sr;
    for (auto& m : mods)
        m.adsr.setSampleRate (sr);
    reset();
}

void VoiceModEngine::reset()
{
    for (auto& m : mods)
        m.adsr.reset();
}

void VoiceModEngine::assign (juce::AudioProcessorValueTreeState& apvts, ParamSource& src)
{
    shadows.clear();

    juce::StringArray names;
    if (auto* tp = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (ModEngine::targetId (0))))
        names = tp->choices;

    targetToShadow.assign ((size_t) names.size(), -1);

    for (int fi = 0; fi < names.size(); ++fi)
    {
        const auto& name = names[fi];
        if (name == "None" || ! ModEngine::isPerVoiceTarget (name))
            continue;

        auto* global = apvts.getRawParameterValue (name);
        auto* param  = apvts.getParameter (name);
        if (global == nullptr || param == nullptr)
            continue;

        Shadow s;
        s.value  = std::make_unique<std::atomic<float>> (global->load());
        s.global = global;
        s.param  = param;

        const auto flagId = ModEngine::resGlobalFlagId (name);
        if (! flagId.isEmpty())
            s.resGlobalFlag = apvts.getRawParameterValue (flagId);

        src.addOverride (name, s.value.get());

        targetToShadow[(size_t) fi] = (int) shadows.size();
        shadows.push_back (std::move (s));
    }

    accum.assign (shadows.size(), 0.0f);

    for (int i = 0; i < ModEngine::numModulators; ++i)
    {
        mods[(size_t) i].target   = apvts.getRawParameterValue (ModEngine::targetId (i));
        mods[(size_t) i].type     = apvts.getRawParameterValue (ModEngine::typeId (i));
        mods[(size_t) i].attack   = apvts.getRawParameterValue (ModEngine::attackId (i));
        mods[(size_t) i].decay    = apvts.getRawParameterValue (ModEngine::decayId (i));
        mods[(size_t) i].sustain  = apvts.getRawParameterValue (ModEngine::sustainId (i));
        mods[(size_t) i].release  = apvts.getRawParameterValue (ModEngine::releaseId (i));
        mods[(size_t) i].amount   = apvts.getRawParameterValue (ModEngine::amountId (i));
        mods[(size_t) i].polarity = apvts.getRawParameterValue (ModEngine::polarityId (i));
    }
}

void VoiceModEngine::noteOn()
{
    for (auto& m : mods)
        m.adsr.noteOn();
}

void VoiceModEngine::noteOff()
{
    for (auto& m : mods)
        m.adsr.noteOff();
}

void VoiceModEngine::process (int numSamples)
{
    if (shadows.empty())
        return;

    // 1) Refresh shadows from the global atomics (base value incl. global LFO).
    for (auto& s : shadows)
        s.value->store (s.global->load());

    // 2) Accumulate this voice's ADSR offsets per target.
    std::fill (accum.begin(), accum.end(), 0.0f);

    for (auto& m : mods)
    {
        if ((int) m.type->load() != ModEngine::typeAdsr)
            continue;

        juce::ADSR::Parameters p;
        p.attack  = juce::jmax (0.0f, m.attack->load());
        p.decay   = juce::jmax (0.0f, m.decay->load());
        p.sustain = juce::jlimit (0.0f, 1.0f, m.sustain->load());
        p.release = juce::jmax (0.0f, m.release->load());
        m.adsr.setParameters (p);

        float env = 0.0f;
        for (int s = 0; s < numSamples; ++s)
            env = m.adsr.getNextSample();   // block-rate: level at block end

        const int flat = (int) m.target->load();
        if (flat < 0 || flat >= (int) targetToShadow.size())
            continue;
        const int sh = targetToShadow[(size_t) flat];
        if (sh < 0)
            continue;   // global target: handled by the global ModEngine

        // A resonator slot currently in global mode is driven globally, so its ADSR is
        // handled by the global engine, not here.
        if (shadows[(size_t) sh].resGlobalFlag != nullptr
            && shadows[(size_t) sh].resGlobalFlag->load() > 0.5f)
            continue;

        const float amount   = juce::jlimit (0.0f, 1.0f, m.amount->load());
        const bool  positive = m.polarity->load() > 0.5f;
        const float base01   = shadows[(size_t) sh].param->getValue();

        accum[(size_t) sh] += ModEngine::adsrOffset (amount, positive, base01, env);
    }

    // 3) Apply the accumulated offsets to the shadows (normalised + clamped).
    for (size_t i = 0; i < shadows.size(); ++i)
    {
        if (accum[i] == 0.0f)
            continue;

        auto& s = shadows[i];
        const auto& range = s.param->getNormalisableRange();
        const float base01 = range.convertTo0to1 (s.value->load());
        const float mod01  = juce::jlimit (0.0f, 1.0f, base01 + accum[i]);
        s.value->store (range.convertFrom0to1 (mod01));
    }
}

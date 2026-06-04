/*
  ==============================================================================

    ResonatorSlot.cpp

  ==============================================================================
*/

#include "ResonatorSlot.h"
#include "ResonatorFactory.h"
#include "../Modulation/ParamSource.h"

ResonatorSlot::ResonatorSlot()
{
    for (const auto& info : ResonatorFactory::types())
        resonators.push_back (info.createResonator());
}

void ResonatorSlot::prepare (const juce::dsp::ProcessSpec& spec)
{
    for (auto& r : resonators)
        r->prepare (spec);

    levelGain.reset (spec.sampleRate, 0.01);   // 10 ms ramp
    levelPrimed = false;
}

void ResonatorSlot::reset()
{
    for (auto& r : resonators)
        r->reset();

    // Snap to the current target so an in-flight ramp can't leak into the attack.
    levelGain.setCurrentAndTargetValue (levelGain.getTargetValue());
}

void ResonatorSlot::assignParameters (ParamSource& apvts, const juce::String& prefix)
{
    typeParam       = apvts.getRawParameterValue (typeParamId (prefix));
    globalParam     = apvts.getRawParameterValue (globalParamId (prefix));
    globalFreqParam = apvts.getRawParameterValue (globalFreqParamId (prefix));
    levelParam      = apvts.getRawParameterValue (levelParamId (prefix));

    const auto& infos = ResonatorFactory::types();
    for (size_t i = 0; i < resonators.size(); ++i)
        resonators[i]->assignParameters (apvts, perTypePrefix (prefix, infos[i].name));
}

void ResonatorSlot::checkParameters()
{
    if (typeParam == nullptr)
        return;

    activeType = juce::jlimit (0, (int) resonators.size() - 1, (int) typeParam->load());
    global     = globalParam->load() > 0.5f;

    const float fundamental = global ? globalFreqParam->load() : voiceFrequency;
    auto* a = active();
    a->setBaseFrequency (fundamental);
    a->checkParameters();

    if (levelParam != nullptr)
    {
        // -60 dB -> 0 (true mute / -inf), matching the parameter readout.
        const float g = juce::Decibels::decibelsToGain (levelParam->load(), levelMinDb);
        if (levelPrimed) levelGain.setTargetValue (g);
        else           { levelGain.setCurrentAndTargetValue (g); levelPrimed = true; }
    }
}

void ResonatorSlot::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix)
{
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        typeParamId (prefix), prefix + " Type", ResonatorFactory::typeChoices(), ResonatorFactory::indexOf ("Block")));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        globalParamId (prefix), prefix + " Global", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        globalFreqParamId (prefix), prefix + " Global Freq",
        juce::NormalisableRange<float> (20.0f, 2000.0f, 0.1f, 0.3f), 110.0f));

    // Per-resonator output level. -60 dB reads (and sounds) as -inf / muted.
    const auto levelAttrs = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([] (float dB, int)
        {
            if (dB <= levelMinDb + 0.05f)
                return juce::String ("-inf dB");
            juce::String s (dB, 1);
            s << " dB";
            return s;
        })
        .withValueFromStringFunction ([] (const juce::String& t)
        {
            if (t.containsIgnoreCase ("inf"))
                return levelMinDb;
            return t.getFloatValue();
        });

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        levelParamId (prefix), prefix + " Level",
        juce::NormalisableRange<float> (levelMinDb, levelMaxDb, 1e-2f), 0.0f, levelAttrs));

    for (const auto& info : ResonatorFactory::types())
    {
        auto probe = info.createResonator();
        probe->addParametersToLayout (params, perTypePrefix (prefix, info.name));
    }
}

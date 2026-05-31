/*
  ==============================================================================

    ResonatorSlot.cpp

  ==============================================================================
*/

#include "ResonatorSlot.h"
#include "ResonatorFactory.h"

ResonatorSlot::ResonatorSlot()
{
    for (const auto& info : ResonatorFactory::types())
        resonators.push_back (info.createResonator());
}

void ResonatorSlot::prepare (const juce::dsp::ProcessSpec& spec)
{
    for (auto& r : resonators)
        r->prepare (spec);
}

void ResonatorSlot::reset()
{
    for (auto& r : resonators)
        r->reset();
}

void ResonatorSlot::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    typeParam       = apvts.getRawParameterValue (typeParamId (prefix));
    globalParam     = apvts.getRawParameterValue (globalParamId (prefix));
    globalFreqParam = apvts.getRawParameterValue (globalFreqParamId (prefix));

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

    for (const auto& info : ResonatorFactory::types())
    {
        auto probe = info.createResonator();
        probe->addParametersToLayout (params, perTypePrefix (prefix, info.name));
    }
}

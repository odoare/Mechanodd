/*
  ==============================================================================

    EffectSlot.cpp

  ==============================================================================
*/

#include "EffectSlot.h"
#include "EffectFactory.h"

EffectSlot::EffectSlot()
{
    for (const auto& info : EffectFactory::types())
        effects.push_back (info.create());
}

void EffectSlot::prepare (double sampleRate, int numChannels, int blockSize)
{
    for (auto& e : effects)
        e->prepare (sampleRate, numChannels, blockSize);
}

void EffectSlot::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& slotPrefix)
{
    typeParam = apvts.getRawParameterValue (typeParamId (slotPrefix));

    const auto& infos = EffectFactory::types();
    for (size_t i = 0; i < effects.size(); ++i)
        effects[i]->assignParameters (apvts, perTypePrefix (slotPrefix, infos[i].name));
}

void EffectSlot::checkParameters()
{
    if (typeParam == nullptr)
        return;

    activeType = (int) typeParam->load();
    if (activeType > 0 && activeType <= (int) effects.size())
        effects[(size_t) (activeType - 1)]->checkParameters();
}

void EffectSlot::process (juce::AudioBuffer<float>& buffer)
{
    if (activeType > 0 && activeType <= (int) effects.size())
        effects[(size_t) (activeType - 1)]->process (buffer);
}

void EffectSlot::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& slotPrefix)
{
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        typeParamId (slotPrefix), slotPrefix + " Type", EffectFactory::typeChoices(), 0));

    for (const auto& info : EffectFactory::types())
    {
        auto probe = info.create();
        probe->addParametersToLayout (params, perTypePrefix (slotPrefix, info.name));
    }
}

/*
  ==============================================================================

    SourceSlot.cpp

  ==============================================================================
*/

#include "SourceSlot.h"
#include "SourceFactory.h"

SourceSlot::SourceSlot()
{
    for (const auto& info : SourceFactory::types())
        sources.push_back (info.createSource());
}

void SourceSlot::prepare (const juce::dsp::ProcessSpec& spec)
{
    for (auto& s : sources)
        s->prepare (spec);
}

void SourceSlot::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& slotPrefix)
{
    typeParam = apvts.getRawParameterValue (typeParamId (slotPrefix));

    const auto& infos = SourceFactory::types();
    for (size_t i = 0; i < sources.size(); ++i)
        sources[i]->assignParameters (apvts, perTypePrefix (slotPrefix, infos[i].name));
}

void SourceSlot::checkParameters()
{
    if (typeParam == nullptr)
        return;

    activeType = (int) typeParam->load();
    if (activeType > 0 && activeType <= (int) sources.size())
        sources[(size_t) (activeType - 1)]->checkParameters();
}

void SourceSlot::process (juce::AudioBuffer<float>& outBuffer, int startSample, int numSamples)
{
    if (activeType > 0 && activeType <= (int) sources.size())
        sources[(size_t) (activeType - 1)]->process (outBuffer, startSample, numSamples);
}

void SourceSlot::noteOn (float velocity)
{
    // Trigger every type so switching type mid-note keeps envelopes coherent.
    for (auto& s : sources)
        s->noteOn (velocity);
}

void SourceSlot::noteOff()
{
    for (auto& s : sources)
        s->noteOff();
}

void SourceSlot::setNoteFrequency (float hz)
{
    for (auto& s : sources)
        s->setNoteFrequency (hz);
}

void SourceSlot::setInputPointers (const float* l, const float* r)
{
    for (auto& s : sources)
        s->setInputPointers (l, r);
}

bool SourceSlot::isActive() const
{
    return activeType > 0
        && activeType <= (int) sources.size()
        && sources[(size_t) (activeType - 1)]->isActive();
}

void SourceSlot::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& slotPrefix)
{
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        typeParamId (slotPrefix), slotPrefix + " Type", SourceFactory::typeChoices(), 0));

    for (const auto& info : SourceFactory::types())
    {
        auto probe = info.createSource();
        probe->addParametersToLayout (params, perTypePrefix (slotPrefix, info.name));
    }
}

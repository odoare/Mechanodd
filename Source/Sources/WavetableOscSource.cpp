/*
  ==============================================================================

    WavetableOscSource.cpp

  ==============================================================================
*/

#include "WavetableOscSource.h"
#include "WavetableLibrary.h"
#include "../Modulation/ParamSource.h"

void WavetableOscSource::onNoteOn()
{
    phase = 0.0f;
    oneShotDone = false;
}

void WavetableOscSource::addParametersToLayout (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                                                const juce::String& prefix)
{
    addCommonParameters (params, prefix);

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        makeId (prefix, "wave"), prefix + " Wave", WavetableLibrary::names(), 0));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        makeId (prefix, "mode"), prefix + " Mode", juce::StringArray { "One-shot", "Loop" }, 1));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        makeId (prefix, "tune"), prefix + " Tune", juce::NormalisableRange<float> (-24.0f, 24.0f, 1.0f), 0.0f));
}

void WavetableOscSource::assignParameters (ParamSource& apvts, const juce::String& prefix)
{
    assignCommonParameters (apvts, prefix);
    pWave = apvts.getRawParameterValue (makeId (prefix, "wave"));
    pLoop = apvts.getRawParameterValue (makeId (prefix, "mode"));
    pTune = apvts.getRawParameterValue (makeId (prefix, "tune"));
}

void WavetableOscSource::checkParameters()
{
    checkCommonParameters();
    if (pWave != nullptr)
    {
        waveIndex = (int) pWave->load();
        loop      = pLoop->load() > 0.5f;
        tune      = pTune->load();
    }
}

void WavetableOscSource::renderSource (juce::AudioBuffer<float>& outBuffer, int startSample, int numSamples)
{
    auto* out = outBuffer.getWritePointer (0);

    const auto& library = WavetableLibrary::tables();
    if (library.empty())
        return;

    const auto& table = library[(size_t) juce::jlimit (0, (int) library.size() - 1, waveIndex)];

    if (oneShotDone)
    {
        juce::FloatVectorOperations::clear (out + startSample, numSamples);
        return;
    }

    const float inc = noteFrequency * std::pow (2.0f, tune / 12.0f) / (float) spec.sampleRate;

    for (int i = 0; i < numSamples; ++i)
    {
        if (oneShotDone)
        {
            out[startSample + i] = 0.0f;
            continue;
        }

        out[startSample + i] = WavetableLibrary::read (table, phase);

        phase += inc;
        if (phase >= 1.0f)
        {
            if (loop)
                phase -= std::floor (phase);
            else
                oneShotDone = true;
        }
    }
}

/*
  ==============================================================================

    ParamSource.h

    A thin indirection over the APVTS for the DSP modules' parameter reads.

    Every module resolves its parameters once in assignParameters() by caching
    the std::atomic<float>* returned by getRawParameterValue(). Normally that
    pointer is the APVTS's own shared atomic, which all voices read - which is
    why global modulation (ModEngine) works by overwriting it.

    ParamSource lets a voice substitute a *per-voice* shadow atomic for chosen
    parameter ids, so that voice's modules read a per-voice modulated value
    instead of the shared one. Overrides are consulted only at assign time
    (setup); the audio path never touches this class.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <unordered_map>
#include <string>

class ParamSource
{
public:
    explicit ParamSource (juce::AudioProcessorValueTreeState& a) : apvts (a) {}

    // Drop-in for AudioProcessorValueTreeState::getRawParameterValue: returns the
    // per-voice override for this id if one was registered, else the shared atomic.
    std::atomic<float>* getRawParameterValue (juce::StringRef id) const
    {
        if (! overrides.empty())
        {
            const auto it = overrides.find (juce::String (id).toStdString());
            if (it != overrides.end())
                return it->second;
        }
        return apvts.getRawParameterValue (id);
    }

    juce::AudioProcessorValueTreeState& getApvts() const { return apvts; }

    // Setup-time only: route reads of `id` to `shadow` instead of the APVTS atomic.
    void addOverride (const juce::String& id, std::atomic<float>* shadow)
    {
        overrides[id.toStdString()] = shadow;
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::unordered_map<std::string, std::atomic<float>*> overrides;
};

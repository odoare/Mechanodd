/*
  ==============================================================================

    PolySynth.h

    juce::Synthesiser that caps how many of its allocated voices may sound at
    once, settable at runtime. All voices stay allocated (no audio-thread
    allocation); only the first `maxActive` are eligible for new notes and for
    stealing, so lowering the count simply lets the surplus voices ring out and
    then fall idle.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class PolySynth : public juce::Synthesiser
{
public:
    // RT-safe: stores an atomic, clamped to [1, number of allocated voices].
    void setMaxActiveVoices (int n) noexcept
    {
        maxActive.store (juce::jlimit (1, juce::jmax (1, getNumVoices()), n),
                         std::memory_order_relaxed);
    }

protected:
    juce::SynthesiserVoice* findFreeVoice (juce::SynthesiserSound* sound, int midiChannel,
                                           int midiNoteNumber, bool stealIfNoneAvailable) const override
    {
        const int n = activeLimit();
        for (int i = 0; i < n; ++i)
            if (auto* v = getVoice (i); ! v->isVoiceActive() && v->canPlaySound (sound))
                return v;

        return stealIfNoneAvailable ? findVoiceToSteal (sound, midiChannel, midiNoteNumber) : nullptr;
    }

    juce::SynthesiserVoice* findVoiceToSteal (juce::SynthesiserSound* sound, int, int) const override
    {
        // Steal the oldest-started eligible voice within the active set.
        const int n = activeLimit();
        juce::SynthesiserVoice* oldest = nullptr;
        for (int i = 0; i < n; ++i)
        {
            auto* v = getVoice (i);
            if (! v->canPlaySound (sound))
                continue;
            if (oldest == nullptr || v->wasStartedBefore (*oldest))
                oldest = v;
        }
        return oldest;
    }

private:
    int activeLimit() const noexcept
    {
        return juce::jlimit (1, juce::jmax (1, getNumVoices()),
                             maxActive.load (std::memory_order_relaxed));
    }

    std::atomic<int> maxActive { 8 };
};

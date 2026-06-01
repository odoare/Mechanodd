/*
  ==============================================================================

    WavetableOscSource.h

    Single-cycle wavetable oscillator. Plays a table from the WavetableLibrary at
    the note frequency (plus a semitone tune offset), either looped (sustained
    tone) or one-shot (a single cycle, i.e. a short excitation burst).

  ==============================================================================
*/

#pragma once

#include "Source.h"

class WavetableOscSource : public Source
{
public:
    static constexpr const char* typeName = "Wavetable";

    void setNoteFrequency (float hz) override { noteFrequency = hz; }

    void addParametersToLayout (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                                const juce::String& prefix) override;
    void assignParameters (ParamSource& apvts, const juce::String& prefix) override;
    void checkParameters() override;

protected:
    void onNoteOn() override;
    void renderSource (juce::AudioBuffer<float>& outBuffer, int startSample, int numSamples) override;

private:
    float noteFrequency { 110.0f };
    float phase { 0.0f };
    bool  oneShotDone { false };

    int   waveIndex { 0 };
    bool  loop { true };
    float tune { 0.0f };   // semitones

    std::atomic<float>* pWave { nullptr };
    std::atomic<float>* pLoop { nullptr };
    std::atomic<float>* pTune { nullptr };
};

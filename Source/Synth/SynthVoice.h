/*
  ==============================================================================

    SynthVoice.h

    A voice driving N source slots into a feedback matrix of M resonator slots.
    Sources are pre-rendered per block (they take no feedback); the matrix then
    runs the resonator network sample-by-sample and mixes the result (level/pan)
    into the output. The voice stays alive while resonators ring, not just while
    the exciter envelope is active.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SynthSound.h"
#include "SourceSlot.h"
#include "ResonatorSlot.h"
#include "FeedbackMatrix.h"
#include "../Modulation/VoiceModEngine.h"

class SynthVoice : public juce::SynthesiserVoice
{
public:
    static constexpr int numSourceSlots    = 4;
    static constexpr int numResonatorSlots = ResonatorSlot::numSlots;

    static juce::String sourceSlotPrefix (int slot) { return "src" + juce::String (slot); }

    SynthVoice();

    bool canPlaySound (juce::SynthesiserSound* sound) override;
    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int newPitchWheelValue) override;
    void controllerMoved (int controllerNumber, int newControllerValue) override;
    using juce::SynthesiserVoice::renderNextBlock;   // keep the double-precision overload visible
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void assignParameters (juce::AudioProcessorValueTreeState& apvts);
    void checkParameters();

    // Advance this voice's per-voice modulators and refresh its parameter shadows.
    // Must run before checkParameters() so the modules cache the modulated values.
    void updateModulation (int numSamples);

    // Shared block buffers owned by the processor:
    //  columnSum  - summed-over-voices column signals (sources + PV resonators) for the global rows.
    //  globalPrev - previous block's global resonator outputs, broadcast to PV rows.
    void setSharedBuffers (juce::AudioBuffer<float>* columnSum, const juce::AudioBuffer<float>* globalPrev,
                           juce::AudioBuffer<float>* sendBus)
    {
        columnSumBuf = columnSum;
        globalPrevBuf = globalPrev;
        sendBusBuf = sendBus;
    }

    // Host audio input for this block (stereo), read by the Plugin Input sources.
    void setInputBuffer (const juce::AudioBuffer<float>* input) { inputBuf = input; }

private:
    std::array<SourceSlot, numSourceSlots>       sourceSlots;
    std::array<ResonatorSlot, numResonatorSlots> resonatorSlots;
    FeedbackMatrix matrix;
    VoiceModEngine voiceMod;

    juce::AudioBuffer<float> sourceScratch;  // numSourceSlots channels
    juce::AudioBuffer<float> voiceOut;       // stereo

    juce::AudioBuffer<float>*       columnSumBuf  { nullptr };
    const juce::AudioBuffer<float>* globalPrevBuf { nullptr };
    juce::AudioBuffer<float>*       sendBusBuf    { nullptr };
    const juce::AudioBuffer<float>* inputBuf      { nullptr };

    double sampleRate { 44100.0 };
    bool   sourcesPlaying { false };
    int    silentSamples  { 0 };
    bool   isPrepared { false };
};

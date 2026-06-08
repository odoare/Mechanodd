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

    // Actually start (reset + retune + retrigger) the new note. Called immediately
    // for a free voice, or after the steal-fade completes for a stolen one.
    void beginNote (int midiNoteNumber, float velocity);
    void assignParameters (juce::AudioProcessorValueTreeState& apvts);
    void checkParameters();

    // Advance this voice's per-voice modulators and refresh its parameter shadows.
    // Must run before checkParameters() so the modules cache the modulated values.
    void updateModulation (int numSamples);

    // Glide the played frequency toward the target note over the portamento time,
    // re-tuning the slots. Call once per block before checkParameters().
    void updatePortamento (int numSamples);

    // Shared block buffers owned by the processor:
    //  columnSum       - summed-over-voices column signals (sources + PV resonators) for the global rows.
    //  globalPrev      - previous block's global resonator outputs, broadcast to PV rows.
    //  busFeedback     - previous block's mono send-bus output for the bus feedback matrix column.
    void setSharedBuffers (juce::AudioBuffer<float>*       columnSum,
                           const juce::AudioBuffer<float>* globalPrev,
                           juce::AudioBuffer<float>*       sendBus,
                           const juce::AudioBuffer<float>* busFeedback)
    {
        columnSumBuf   = columnSum;
        globalPrevBuf  = globalPrev;
        sendBusBuf     = sendBus;
        busFeedbackBuf = busFeedback;
    }

    // Host audio input for this block (stereo), read by the Plugin Input sources.
    void setInputBuffer (const juce::AudioBuffer<float>* input) { inputBuf = input; }

private:
    // Render `numSamples` of this voice into outputBuffer at startSample. When
    // fading, the (old, still-ringing) content is multiplied by the descending
    // steal-fade ramp; otherwise the note-start bridge offset is added. Returns
    // the peak magnitude of the raw voice content over the segment.
    float renderSegment (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples, bool fading);

    std::array<SourceSlot, numSourceSlots>       sourceSlots;
    std::array<ResonatorSlot, numResonatorSlots> resonatorSlots;
    FeedbackMatrix matrix;
    VoiceModEngine voiceMod;

    juce::AudioBuffer<float> sourceScratch;  // numSourceSlots channels
    juce::AudioBuffer<float> voiceOut;       // stereo

    juce::AudioBuffer<float>*       columnSumBuf   { nullptr };
    const juce::AudioBuffer<float>* globalPrevBuf  { nullptr };
    juce::AudioBuffer<float>*       sendBusBuf     { nullptr };
    const juce::AudioBuffer<float>* busFeedbackBuf { nullptr };
    const juce::AudioBuffer<float>* inputBuf      { nullptr };

    double sampleRate { 44100.0 };
    bool   sourcesPlaying { false };
    int    silentSamples  { 0 };
    bool   isPrepared { false };

    // Note-start declick: starting a note resets/retunes this voice's resonators,
    // which (when the voice was still ringing - i.e. stolen) cuts the output to ~0
    // in one sample. We bridge that step by carrying the last output value into the
    // new note and decaying it out, so the output stays continuous. For a free
    // (silent) voice the carried value is ~0, so it does nothing.
    float lastOutL { 0.0f }, lastOutR { 0.0f };   // last emitted output sample
    float declickL { 0.0f }, declickR { 0.0f };   // decaying bridge offset
    float declickCoeff { 0.0f };                  // per-sample decay

    // Voice stealing: rather than zeroing a still-ringing voice onto the new note
    // (a click, worst when the pitch jump is large), fade the old content out over
    // a few ms, then begin the new note. The new note is held pending until then.
    bool  audible           { false };   // was this voice producing sound last block?
    bool  pendingStart      { false };
    int   pendingNote       { 60 };
    float pendingVelocity   { 1.0f };
    int   stealFadeLen      { 0 };       // total fade length (samples)
    int   stealFadeRemaining { 0 };
    float stealGain         { 0.0f };    // ramps 1 -> 0 over the fade

    // Portamento: glideFreq slides toward targetFreq (the last struck note) at a
    // rate set by the shared "portamento" parameter; pushed to the slots each block.
    std::atomic<float>* pPortamento { nullptr };
    float glideFreq  { 0.0f };
    float targetFreq { 0.0f };
    bool  hasPlayed  { false };

    void pushFrequency (float hz);   // retune all source + resonator slots
};

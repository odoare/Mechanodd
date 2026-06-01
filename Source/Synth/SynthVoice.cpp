/*
  ==============================================================================

    SynthVoice.cpp

  ==============================================================================
*/

#include "SynthVoice.h"
#include "../Modulation/ParamSource.h"

SynthVoice::SynthVoice() = default;

bool SynthVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<SynthSound*> (sound) != nullptr;
}

void SynthVoice::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    for (auto& slot : sourceSlots)
        slot.prepare (spec);
    for (auto& slot : resonatorSlots)
        slot.prepare (spec);
    matrix.prepare (spec);

    sourceScratch.setSize (numSourceSlots, (int) spec.maximumBlockSize, false, false, true);
    voiceOut.setSize (2, (int) spec.maximumBlockSize, false, false, true);

    voiceMod.prepare (spec.sampleRate);

    isPrepared = true;
}

void SynthVoice::assignParameters (juce::AudioProcessorValueTreeState& apvts)
{
    // Build a per-voice parameter source: the voice's modulators register shadow
    // atomics for per-voice targets, then the modules below resolve through it so
    // they read the voice's modulated values instead of the shared APVTS atomics.
    ParamSource src (apvts);
    voiceMod.assign (apvts, src);

    for (int i = 0; i < numSourceSlots; ++i)
        sourceSlots[(size_t) i].assignParameters (src, sourceSlotPrefix (i));
    for (int i = 0; i < numResonatorSlots; ++i)
        resonatorSlots[(size_t) i].assignParameters (src, ResonatorSlot::slotPrefix (i));
    matrix.assignParameters (src);
}

void SynthVoice::updateModulation (int numSamples)
{
    voiceMod.process (numSamples);
}

void SynthVoice::checkParameters()
{
    for (auto& slot : sourceSlots)
        slot.checkParameters();
    for (auto& slot : resonatorSlots)
        slot.checkParameters();
    matrix.checkParameters();
}

void SynthVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int)
{
    const float freq = (float) juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);

    for (auto& slot : sourceSlots)
    {
        slot.setNoteFrequency (freq);
        slot.noteOn (velocity);
    }

    for (auto& slot : resonatorSlots)
    {
        slot.setVoiceFrequency (freq);
        slot.reset();
        slot.noteOn();
    }
    matrix.reset();
    voiceMod.noteOn();

    sourcesPlaying = true;
    silentSamples  = 0;
}

void SynthVoice::stopNote (float, bool allowTailOff)
{
    for (auto& slot : sourceSlots)
        slot.noteOff();
    for (auto& slot : resonatorSlots)
        slot.noteOff();
    voiceMod.noteOff();

    if (! allowTailOff)
        clearCurrentNote();
}

void SynthVoice::pitchWheelMoved (int) {}
void SynthVoice::controllerMoved (int, int) {}

void SynthVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (! isPrepared)
        return;

    // Offset host-input pointers to this sub-block; hand them to the slots so
    // the Plugin Input sources read the right samples.
    const float* inL = nullptr;
    const float* inR = nullptr;
    if (inputBuf != nullptr && inputBuf->getNumChannels() > 0)
    {
        inL = inputBuf->getReadPointer (0) + startSample;
        inR = (inputBuf->getNumChannels() > 1) ? inputBuf->getReadPointer (1) + startSample : inL;
    }

    // 1) Pre-render each source slot into its own mono channel of sourceScratch.
    sourceScratch.clear();
    for (int s = 0; s < numSourceSlots; ++s)
    {
        sourceSlots[(size_t) s].setInputPointers (inL, inR);

        float* ptr = sourceScratch.getWritePointer (s);
        juce::AudioBuffer<float> mono (&ptr, 1, numSamples);
        sourceSlots[(size_t) s].process (mono, 0, numSamples);
    }

    // 2) Run the PV resonator network + mix into the per-voice stereo buffer.
    voiceOut.clear();
    const float* srcPtrs[numSourceSlots];
    for (int s = 0; s < numSourceSlots; ++s)
        srcPtrs[s] = sourceScratch.getReadPointer (s);

    const float* globalPrevPtrs[numResonatorSlots] = {};
    if (globalPrevBuf != nullptr)
        for (int k = 0; k < numResonatorSlots; ++k)
            globalPrevPtrs[k] = globalPrevBuf->getReadPointer (k) + startSample;

    float* colSumPtrs[FeedbackMatrix::numColumns] = {};
    if (columnSumBuf != nullptr)
        for (int c = 0; c < FeedbackMatrix::numColumns; ++c)
            colSumPtrs[c] = columnSumBuf->getWritePointer (c) + startSample;

    float* sendL = nullptr;
    float* sendR = nullptr;
    if (sendBusBuf != nullptr && sendBusBuf->getNumChannels() > 1)
    {
        sendL = sendBusBuf->getWritePointer (0) + startSample;
        sendR = sendBusBuf->getWritePointer (1) + startSample;
    }

    float* vL = voiceOut.getWritePointer (0);
    float* vR = voiceOut.getWritePointer (1);
    matrix.processVoice (srcPtrs, resonatorSlots, globalPrevPtrs, vL, vR, sendL, sendR, colSumPtrs, numSamples);

    // 3) Add to the shared output.
    const int outChannels = outputBuffer.getNumChannels();
    outputBuffer.addFrom (0, startSample, vL, numSamples);
    if (outChannels > 1)
        outputBuffer.addFrom (1, startSample, vR, numSamples);

    // 4) Tail handling: keep the voice alive while the resonators still ring.
    bool sourcesActive = false;
    for (auto& slot : sourceSlots)
        sourcesActive = sourcesActive || slot.isActive();
    sourcesPlaying = sourcesActive;

    const float peak = juce::jmax (voiceOut.getMagnitude (0, 0, numSamples),
                                   voiceOut.getMagnitude (1, 0, numSamples));

    if (sourcesActive || peak > 1.0e-4f)
        silentSamples = 0;
    else
        silentSamples += numSamples;

    if (! sourcesActive && silentSamples > (int) (0.2 * sampleRate))
        clearCurrentNote();
}

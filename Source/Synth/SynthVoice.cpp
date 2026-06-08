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

    // ~2.5 ms exponential decay for the note-start output bridge (see header).
    constexpr double declickTau = 0.0025;
    declickCoeff = (float) std::exp (-1.0 / (declickTau * sampleRate));

    // ~5 ms linear fade-out when a still-ringing voice is stolen (see header).
    constexpr double stealFadeSeconds = 0.005;
    stealFadeLen = juce::jmax (1, (int) (stealFadeSeconds * sampleRate));

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

    // Portamento is a shared (non-per-voice) parameter, read straight from the APVTS.
    pPortamento = apvts.getRawParameterValue ("portamento");
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

void SynthVoice::pushFrequency (float hz)
{
    for (auto& slot : sourceSlots)
        slot.setNoteFrequency (hz);
    for (auto& slot : resonatorSlots)
        slot.setVoiceFrequency (hz);
}

void SynthVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int)
{
    // If this voice is still ringing (it was stolen), let the old content release
    // over a short fade first, then begin the new note - otherwise resetting it now
    // would cut the ring to ~0 in one sample (a click, worst on a large pitch jump).
    // The steal's preceding stopNote() has already started the old gate releasing.
    if (audible)
    {
        pendingStart      = true;
        pendingNote       = midiNoteNumber;
        pendingVelocity   = velocity;
        stealFadeRemaining = stealFadeLen;
        stealGain         = 1.0f;
        return;
    }

    beginNote (midiNoteNumber, velocity);
}

void SynthVoice::beginNote (int midiNoteNumber, float velocity)
{
    targetFreq = (float) juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);

    // Glide from the voice's previous note when portamento is engaged; otherwise
    // (or on the voice's first note) snap to the new pitch.
    const float portaMs = pPortamento != nullptr ? pPortamento->load() : 0.0f;
    if (! hasPlayed || portaMs < 1.0f)
        glideFreq = targetFreq;
    hasPlayed = true;

    pushFrequency (glideFreq);

    // Bridge any residual output step from the resets below: continue from the last
    // emitted sample and decay it out. After a steal-fade this is ~0 already.
    declickL = lastOutL;
    declickR = lastOutR;

    for (auto& slot : sourceSlots)
        slot.noteOn (velocity);

    for (auto& slot : resonatorSlots)
    {
        slot.reset();
        slot.noteOn();
    }
    matrix.reset();
    voiceMod.noteOn();

    // Retune the resonators to the new note now. checkParameters() for the block
    // already ran (before renderNextBlock, where this note starts), so without
    // this the resonators would render their first block at the previous note's
    // tuning and only retune next block - a deferred geometry jump that clicks
    // once they have rung up, and grows with the size of the pitch change.
    for (auto& slot : resonatorSlots)
        slot.checkParameters();

    sourcesPlaying = true;
    silentSamples  = 0;
}

void SynthVoice::updatePortamento (int numSamples)
{
    if (! isVoiceActive() || juce::approximatelyEqual (glideFreq, targetFreq))
        return;

    const float portaMs = pPortamento != nullptr ? pPortamento->load() : 0.0f;
    if (portaMs < 1.0f)
    {
        glideFreq = targetFreq;
    }
    else
    {
        // One-pole approach in log-frequency (perceptually even glide).
        const double tau = (double) portaMs * 0.001;
        const double dt  = (double) numSamples / sampleRate;
        const float  k   = (float) (1.0 - std::exp (-dt / tau));

        const float lg = std::log (glideFreq);
        const float lt = std::log (targetFreq);
        glideFreq = std::exp (lg + k * (lt - lg));

        if (std::abs (lt - std::log (glideFreq)) < 1.0e-4f)
            glideFreq = targetFreq;
    }

    pushFrequency (glideFreq);
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

float SynthVoice::renderSegment (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples, bool fading)
{
    if (numSamples <= 0)
        return 0.0f;

    // Offset host-input pointers to this segment; hand them to the slots so the
    // Plugin Input sources read the right samples.
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

    const float* busFeedbackPtr = nullptr;
    if (busFeedbackBuf != nullptr)
        busFeedbackPtr = busFeedbackBuf->getReadPointer (0) + startSample;

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
    matrix.processVoice (srcPtrs, resonatorSlots, globalPrevPtrs, busFeedbackPtr, vL, vR, sendL, sendR, colSumPtrs, numSamples);

    // 3) Mix into the shared output. When fading, scale the old content by the
    //    descending steal ramp; otherwise add the decaying note-start bridge so
    //    the per-voice output stays continuous across resets. Remember the last
    //    emitted value to bridge from on the next note start.
    const int outChannels = outputBuffer.getNumChannels();
    float* oL = outputBuffer.getWritePointer (0) + startSample;
    float* oR = outChannels > 1 ? outputBuffer.getWritePointer (1) + startSample : nullptr;

    const float gainStep = (stealFadeLen > 0) ? 1.0f / (float) stealFadeLen : 1.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float l = vL[i];
        float r = vR[i];

        if (fading)
        {
            l *= stealGain;
            r *= stealGain;
            stealGain = juce::jmax (0.0f, stealGain - gainStep);
        }
        else
        {
            l += declickL;
            r += declickR;
            declickL *= declickCoeff;
            declickR *= declickCoeff;
        }

        oL[i] += l;
        if (oR != nullptr)
            oR[i] += r;

        lastOutL = l;
        lastOutR = r;
    }

    return juce::jmax (voiceOut.getMagnitude (0, 0, numSamples),
                       voiceOut.getMagnitude (1, 0, numSamples));
}

void SynthVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (! isPrepared)
        return;

    int pos       = startSample;
    int remaining = numSamples;
    float peak    = 0.0f;

    // Phase 1: fade out a stolen voice's still-ringing content, then begin the
    // pending note once the fade completes (possibly mid-block).
    if (pendingStart)
    {
        const int n = juce::jmin (remaining, stealFadeRemaining);
        peak = juce::jmax (peak, renderSegment (outputBuffer, pos, n, true));
        pos += n;
        remaining -= n;
        stealFadeRemaining -= n;

        if (stealFadeRemaining <= 0)
        {
            beginNote (pendingNote, pendingVelocity);
            pendingStart = false;
        }
    }

    // Phase 2: render the live note for the rest of the block.
    if (remaining > 0)
        peak = juce::jmax (peak, renderSegment (outputBuffer, pos, remaining, false));

    // Tail handling: keep the voice alive while sources / resonators still ring.
    bool sourcesActive = false;
    for (auto& slot : sourceSlots)
        sourcesActive = sourcesActive || slot.isActive();
    sourcesPlaying = sourcesActive;

    audible = sourcesActive || peak > 1.0e-4f;

    if (audible)
        silentSamples = 0;
    else
        silentSamples += numSamples;

    // Never free the voice while a note is still pending its steal-fade.
    if (! pendingStart && ! sourcesActive && silentSamples > (int) (0.2 * sampleRate))
        clearCurrentNote();
}

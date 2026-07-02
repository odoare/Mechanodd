/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ResonatorSlot.h"
#include "Modulation/ParamSource.h"
#include <BinaryData.h>

//==============================================================================
MechanOddAudioProcessor::MechanOddAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                       // A stereo audio input bus is exposed even though this is a synth, so the
                       // "Input L/R" sources can feed the host's audio into the engine. Hosts with
                       // free channel routing (e.g. Reaper) can route any channels into it.
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts (*this, nullptr, "Parameters", createParameterLayout()),
#else
       apvts (*this, nullptr, "Parameters", createParameterLayout()),
#endif
       presetManager (apvts,
                      fxme::PresetManager::getDefaultUserPresetDirectory ("MechanOdd"),
                      BinaryData::namedResourceList,
                      BinaryData::namedResourceListSize,
                      BinaryData::getNamedResource)
{
    synth.addSound (new SynthSound());
    for (int i = 0; i < numVoices; ++i)
        synth.addVoice (new SynthVoice());
}

juce::AudioProcessorValueTreeState::ParameterLayout MechanOddAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    for (int i = 0; i < SynthVoice::numSourceSlots; ++i)
        SourceSlot::addParameters (params, SynthVoice::sourceSlotPrefix (i));

    for (int i = 0; i < ResonatorSlot::numSlots; ++i)
        ResonatorSlot::addParameters (params, ResonatorSlot::slotPrefix (i));

    FeedbackMatrix::addParameters (params);

    EffectChain::addParameters (params, "bus");
    EffectChain::addParameters (params, "master");

    // Modulation targets = every float parameter created so far, plus the rate
    // and depth of each LFO (added below by ModEngine::addParameters — their IDs
    // are deterministic so we can list them now; the APVTS lookup happens later in
    // assignParameters(), after all parameters are registered).
    juce::StringArray modTargets;
    modTargets.add ("None");
    for (auto& p : params)
        if (auto* fp = dynamic_cast<juce::AudioParameterFloat*> (p.get()))
            modTargets.add (fp->paramID);
    for (int i = 0; i < ModEngine::numModulators; ++i)
    {
        modTargets.add (ModEngine::rateId  (i));
        modTargets.add (ModEngine::depthId (i));
    }

    ModEngine::addParameters (params, modTargets);

    // Global output controls. Added after the modulation-target snapshot so they
    // are not themselves modulation destinations.
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        outputVolumeId, "Output Volume",
        juce::NormalisableRange<float> (-60.0f, 6.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")
            .withStringFromValueFunction ([] (float v, int) { return v <= -59.9f ? juce::String ("-inf") : juce::String (v, 1); })));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        numVoicesId, "Voices", 1, MechanOddAudioProcessor::numVoices, MechanOddAudioProcessor::numVoices));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        portamentoId, "Portamento",
        juce::NormalisableRange<float> (0.0f, 2000.0f, 1.0f, 0.4f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")
            .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v)); })));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        busPostMasterId, "Bus Post Master", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        busOutVolId, "Send Output Volume",
        juce::NormalisableRange<float> (-60.0f, 12.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")
            .withStringFromValueFunction ([] (float v, int) { return v <= -59.9f ? juce::String ("-inf") : juce::String (v, 1); })));

    return { params.begin(), params.end() };
}

MechanOddAudioProcessor::~MechanOddAudioProcessor()
{
}

//==============================================================================
const juce::String MechanOddAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MechanOddAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MechanOddAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MechanOddAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MechanOddAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

// Program layout: index 0 is always "Default" (current state, no load).
// Factory presets occupy indices 1..N.  This guarantees getNumPrograms() >= 2
// whenever at least one preset XML exists, which is required by the JUCE VST3
// wrapper to register the program-selection parameter with the host.
// Preset handling itself (factory + user banks) lives in fxme::PresetManager.

int MechanOddAudioProcessor::getNumPrograms()
{
    return 1 + (int) presetManager.getFactoryPresets().size();   // 0 = Default, 1..N = factory presets
}

int MechanOddAudioProcessor::getCurrentProgram()
{
    // -1 (not a factory preset) maps to program 0, "Default".
    return presetManager.getCurrentFactoryIndex() + 1;
}

void MechanOddAudioProcessor::setCurrentProgram (int index)
{
    if (index == 0)
        return;   // "Default" — no state change

    presetManager.loadFactoryPreset (index - 1);
}

const juce::String MechanOddAudioProcessor::getProgramName (int index)
{
    if (index == 0)
        return "Default";

    const auto& factory = presetManager.getFactoryPresets();
    if (index >= 1 && index <= (int) factory.size())
        return factory[(size_t) (index - 1)].name;
    return {};
}

void MechanOddAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);  // factory presets are read-only
}

//==============================================================================
void MechanOddAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = 1;

    // Global modules read the shared APVTS atomics directly (no per-voice shadows).
    ParamSource globalSrc (apvts);

    for (auto& slot : globalResonators)
        slot.prepare (spec);
    globalMatrix.prepare (spec);
    globalMatrix.assignParameters (globalSrc);
    for (int i = 0; i < ResonatorSlot::numSlots; ++i)
        globalResonators[(size_t) i].assignParameters (globalSrc, ResonatorSlot::slotPrefix (i));

    inputCapture.setSize (2, samplesPerBlock, false, false, true);
    inputCapture.clear();

    columnSum.setSize (FeedbackMatrix::numColumns, samplesPerBlock, false, false, true);
    globalOutA.setSize (FeedbackMatrix::numResonators, samplesPerBlock, false, false, true);
    globalOutB.setSize (FeedbackMatrix::numResonators, samplesPerBlock, false, false, true);
    sendBus.setSize (2, samplesPerBlock, false, false, true);
    prevSendBusOut.setSize (1, samplesPerBlock, false, false, true);
    columnSum.clear();
    globalOutA.clear();
    globalOutB.clear();
    sendBus.clear();
    prevSendBusOut.clear();
    useAasPrev = true;

    for (auto& l : columnLevel)
        l.store (0.0f, std::memory_order_relaxed);
    columnEnv.fill (0.0f);

    pOutputVolume  = apvts.getRawParameterValue (outputVolumeId);
    pNumVoices     = apvts.getRawParameterValue (numVoicesId);
    pBusPostMaster = apvts.getRawParameterValue (busPostMasterId);
    pBusOutVol     = apvts.getRawParameterValue (busOutVolId);
    busOutputGain.reset (sampleRate, 0.02);
    busOutputGain.setCurrentAndTargetValue (
        juce::Decibels::decibelsToGain (pBusOutVol != nullptr ? pBusOutVol->load() : 0.0f, -60.0f));
    outputGain.reset (sampleRate, 0.02);
    outputGain.setCurrentAndTargetValue (
        juce::Decibels::decibelsToGain (pOutputVolume != nullptr ? pOutputVolume->load() : 0.0f, -60.0f));
    outEnv.fill (0.0f);
    outHoldEnv.fill (0.0f);
    for (auto& l : outLevel) l.store (-100.0f, std::memory_order_relaxed);
    for (auto& l : outHold)  l.store (-100.0f, std::memory_order_relaxed);

    busChain.prepare (sampleRate, 2, samplesPerBlock);
    masterChain.prepare (sampleRate, 2, samplesPerBlock);
    busChain.assignParameters (apvts, "bus");
    masterChain.assignParameters (apvts, "master");

    modEngine.prepare (sampleRate);
    modEngine.assignParameters (apvts);

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
        {
            voice->prepare (spec);
            voice->assignParameters (apvts);
        }
}

void MechanOddAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MechanOddAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // Main output must be mono or stereo.
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    // Audio input may be disabled, mono or stereo (read by the Input L/R sources).
    const auto in = layouts.getMainInputChannelSet();
    if (in != juce::AudioChannelSet::disabled()
     && in != juce::AudioChannelSet::mono()
     && in != juce::AudioChannelSet::stereo())
        return false;

    return true;
  #endif
}
#endif

void MechanOddAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();

    if (pNumVoices != nullptr)
        synth.setMaxActiveVoices ((int) pNumVoices->load());

    // Capture the host's audio input before clearing the buffer, so the
    // "Input L/R" sources can read it. Missing channels stay silent.
    inputCapture.clear();
    const int numIn = juce::jmin (2, getTotalNumInputChannels(), buffer.getNumChannels());
    for (int ch = 0; ch < numIn; ++ch)
        inputCapture.copyFrom (ch, 0, buffer, ch, 0, numSamples);

    buffer.clear();

    // Apply modulation first so every module reads modulated parameter values this block.
    {
        double bpm = 120.0, ppq = 0.0;
        bool playing = false;
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
            {
                bpm     = pos->getBpm().orFallback (120.0);
                ppq     = pos->getPpqPosition().orFallback (0.0);
                playing = pos->getIsPlaying();
            }

        // Track held notes (read-only; doesn't consume the buffer) to derive the global
        // ADSR gate: attack on the first note after silence, release on the last note off.
        for (const auto meta : midiMessages)
        {
            const auto msg = meta.getMessage();
            if (msg.isNoteOn())
                ++heldNoteCount;
            else if (msg.isNoteOff())
                heldNoteCount = juce::jmax (0, heldNoteCount - 1);
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                heldNoteCount = 0;
        }

        modEngine.process (numSamples, bpm, ppq, playing, heldNoteCount > 0);
    }

    // Pick ping-pong roles: voices read last block's global output (prev); global writes this block (cur).
    auto& prevBuf = useAasPrev ? globalOutA : globalOutB;
    auto& curBuf  = useAasPrev ? globalOutB : globalOutA;

    columnSum.clear();
    sendBus.clear();

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
        {
            // Shared buffers/input must be set for every voice (a note may start on an
            // idle one mid-block). prepareBlock() then does the per-voice modulation +
            // parameter work for active voices only, so idle voices cost almost nothing.
            voice->setSharedBuffers (&columnSum, &prevBuf, &sendBus, &prevSendBusOut);
            voice->setInputBuffer (&inputCapture);
            voice->prepareBlock (numSamples);
        }

    for (auto& slot : globalResonators)
        slot.checkParameters();
    globalMatrix.checkParameters();
    busChain.checkParameters();
    masterChain.checkParameters();

    // Voices render: per-voice output + accumulate columnSum, reading prevBuf for global columns.
    synth.renderNextBlock (buffer, midiMessages, 0, numSamples);

    // Global resonator rows: fed by columnSum, mixed into the main output, written to curBuf.
    curBuf.clear();
    {
        const float* colPtrs[FeedbackMatrix::numColumns];
        for (int c = 0; c < FeedbackMatrix::numColumns; ++c)
            colPtrs[c] = columnSum.getReadPointer (c);

        float* gOutPtrs[FeedbackMatrix::numResonators];
        for (int k = 0; k < FeedbackMatrix::numResonators; ++k)
            gOutPtrs[k] = curBuf.getWritePointer (k);

        float* outL = buffer.getWritePointer (0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : outL;

        const bool stereoBus = sendBus.getNumChannels() > 1;
        float* sendL = sendBus.getWritePointer (0);
        float* sendR = stereoBus ? sendBus.getWritePointer (1) : sendL;

        globalMatrix.processGlobal (colPtrs, globalResonators, prevSendBusOut.getReadPointer (0),
                                    outL, outR, sendL, sendR, gOutPtrs, numSamples);

        // Per-column "entering signal" level for the matrix meters. Sources and
        // per-voice resonators are summed in columnSum; global resonators are in
        // this block's global output (curBuf).
        //
        // Peak-hold ballistics: jump to the block peak instantly, then release
        // slowly, so brief transients (e.g. very short-attack sources) stay
        // visible between the 30 Hz GUI frames instead of flashing for one block.
        constexpr double meterReleaseSeconds = 0.6;
        const double sr = getSampleRate();
        const float relCoeff = sr > 0.0
            ? (float) (1.0 - std::exp (-((double) numSamples / sr) / meterReleaseSeconds))
            : 1.0f;

        for (int c = 0; c < FeedbackMatrix::numColumns; ++c)
        {
            float mag;
            if (c < FeedbackMatrix::numSources)
                mag = columnSum.getMagnitude (c, 0, numSamples);
            else if (c == FeedbackMatrix::busFeedbackCol)
                mag = prevSendBusOut.getMagnitude (0, 0, numSamples);   // previous block's bus output
            else
            {
                const int k = c - FeedbackMatrix::numSources;
                mag = globalMatrix.isGlobal (k) ? curBuf.getMagnitude (k, 0, numSamples)
                                                : columnSum.getMagnitude (FeedbackMatrix::numSources + k, 0, numSamples);
            }

            float& env = columnEnv[(size_t) c];
            env = (mag > env) ? mag : env + relCoeff * (mag - env);
            columnLevel[(size_t) c].store (env, std::memory_order_relaxed);
        }
    }

    useAasPrev = ! useAasPrev;

    // Send bus: process its effect chain, then mix back into the main output.
    // When "post master" is enabled the bus chain runs after the master chain so
    // it is not coloured by the master processing; otherwise (default) the bus
    // output is folded into the mix before the master chain runs.
    const bool busPostMaster = pBusPostMaster != nullptr && pBusPostMaster->load() > 0.5f;

    // Apply a per-sample smoothed output level to the send bus (runs in both branches).
    auto applyBusOutputGain = [&]()
    {
        busOutputGain.setTargetValue (
            juce::Decibels::decibelsToGain (pBusOutVol != nullptr ? pBusOutVol->load() : 0.0f, -60.0f));
        const int nch = sendBus.getNumChannels();
        for (int i = 0; i < numSamples; ++i)
        {
            const float g = busOutputGain.getNextValue();
            for (int ch = 0; ch < nch; ++ch)
                sendBus.getWritePointer (ch)[i] *= g;
        }
    };

    // Store a mono (L+R)/2 mix of the levelled send bus for next block's
    // resonator feedback column.
    auto captureSendMono = [&]()
    {
        auto* dst        = prevSendBusOut.getWritePointer (0);
        const auto* busL = sendBus.getReadPointer (0);
        const auto* busR = sendBus.getNumChannels() > 1 ? sendBus.getReadPointer (1) : busL;
        for (int i = 0; i < numSamples; ++i)
        {
            // This mono mix re-enters the matrix as a feedback column next block, so a
            // non-finite sample from the bus FX would poison the loop permanently — drop it.
            const float m = 0.5f * (busL[i] + busR[i]);
            dst[i] = std::isfinite (m) ? m : 0.0f;
        }
    };

    if (busPostMaster)
    {
        masterChain.process (buffer);
        busChain.process (sendBus);
        captureSendMono();
        applyBusOutputGain();
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addFrom (ch, 0, sendBus, juce::jmin (ch, sendBus.getNumChannels() - 1), 0, numSamples);
    }
    else
    {
        busChain.process (sendBus);
        captureSendMono();
        applyBusOutputGain();
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addFrom (ch, 0, sendBus, juce::jmin (ch, sendBus.getNumChannels() - 1), 0, numSamples);
        masterChain.process (buffer);
    }

    // Output volume (smoothed) applied to the full mix.
    outputGain.setTargetValue (
        juce::Decibels::decibelsToGain (pOutputVolume != nullptr ? pOutputVolume->load() : 0.0f, -60.0f));
    const int outChannels = buffer.getNumChannels();
    for (int i = 0; i < numSamples; ++i)
    {
        const float g = outputGain.getNextValue();
        for (int ch = 0; ch < outChannels; ++ch)
            buffer.getWritePointer (ch)[i] *= g;
    }

    // Post-gain stereo level for the output meter: fast peak (moving bar) and a
    // slow peak-hold (trailing marker), stored in dB. Same peak-hold idea as the
    // matrix meters: jump to the block peak, then release.
    {
        const double sr = getSampleRate();
        const auto relCoeff = [sr, numSamples] (double seconds)
        {
            return sr > 0.0 ? (float) (1.0 - std::exp (-((double) numSamples / sr) / seconds)) : 1.0f;
        };
        const float relFast = relCoeff (0.15);
        const float relHold = relCoeff (1.2);

        for (int ch = 0; ch < 2; ++ch)
        {
            const float mag = ch < outChannels ? buffer.getMagnitude (ch, 0, numSamples) : 0.0f;

            float& e = outEnv[(size_t) ch];
            e = (mag > e) ? mag : e + relFast * (mag - e);
            float& h = outHoldEnv[(size_t) ch];
            h = (mag > h) ? mag : h + relHold * (mag - h);

            outLevel[(size_t) ch].store (juce::Decibels::gainToDecibels (e, -100.0f), std::memory_order_relaxed);
            outHold [(size_t) ch].store (juce::Decibels::gainToDecibels (h, -100.0f), std::memory_order_relaxed);
        }
    }
}

//==============================================================================
bool MechanOddAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* MechanOddAudioProcessor::createEditor()
{
    return new MechanOddAudioProcessorEditor (*this);
}

//==============================================================================
void MechanOddAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void MechanOddAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MechanOddAudioProcessor();
}

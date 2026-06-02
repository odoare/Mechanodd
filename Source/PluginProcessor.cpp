/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ResonatorSlot.h"
#include "Modulation/ParamSource.h"

//==============================================================================
MechanoscAudioProcessor::MechanoscAudioProcessor()
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
       apvts (*this, nullptr, "Parameters", createParameterLayout())
#else
       apvts (*this, nullptr, "Parameters", createParameterLayout())
#endif
{
    synth.addSound (new SynthSound());
    for (int i = 0; i < numVoices; ++i)
        synth.addVoice (new SynthVoice());
}

juce::AudioProcessorValueTreeState::ParameterLayout MechanoscAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    for (int i = 0; i < SynthVoice::numSourceSlots; ++i)
        SourceSlot::addParameters (params, SynthVoice::sourceSlotPrefix (i));

    for (int i = 0; i < ResonatorSlot::numSlots; ++i)
        ResonatorSlot::addParameters (params, ResonatorSlot::slotPrefix (i));

    FeedbackMatrix::addParameters (params);

    EffectChain::addParameters (params, "bus");
    EffectChain::addParameters (params, "master");

    // Modulation targets = every float parameter created so far.
    juce::StringArray modTargets;
    modTargets.add ("None");
    for (auto& p : params)
        if (auto* fp = dynamic_cast<juce::AudioParameterFloat*> (p.get()))
            modTargets.add (fp->paramID);

    ModEngine::addParameters (params, modTargets);

    return { params.begin(), params.end() };
}

MechanoscAudioProcessor::~MechanoscAudioProcessor()
{
}

//==============================================================================
const juce::String MechanoscAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MechanoscAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MechanoscAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MechanoscAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MechanoscAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MechanoscAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int MechanoscAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MechanoscAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String MechanoscAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void MechanoscAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void MechanoscAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
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
    columnSum.clear();
    globalOutA.clear();
    globalOutB.clear();
    sendBus.clear();
    useAasPrev = true;

    for (auto& l : columnLevel)
        l.store (0.0f, std::memory_order_relaxed);
    columnEnv.fill (0.0f);

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

void MechanoscAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MechanoscAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void MechanoscAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();

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
            voice->updateModulation (numSamples);   // fill per-voice shadows before modules cache them
            voice->checkParameters();
            voice->setSharedBuffers (&columnSum, &prevBuf, &sendBus);
            voice->setInputBuffer (&inputCapture);
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

        globalMatrix.processGlobal (colPtrs, globalResonators, outL, outR, sendL, sendR, gOutPtrs, numSamples);

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
    busChain.process (sendBus);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        buffer.addFrom (ch, 0, sendBus, juce::jmin (ch, sendBus.getNumChannels() - 1), 0, numSamples);

    // Master effect chain on the full mix.
    masterChain.process (buffer);
}

//==============================================================================
bool MechanoscAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* MechanoscAudioProcessor::createEditor()
{
    return new MechanoscAudioProcessorEditor (*this);
}

//==============================================================================
void MechanoscAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void MechanoscAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MechanoscAudioProcessor();
}

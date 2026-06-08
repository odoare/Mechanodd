/*
  ==============================================================================

    PluginProcessor.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SynthVoice.h"
#include "SynthSound.h"
#include "PolySynth.h"
#include "ResonatorSlot.h"
#include "FeedbackMatrix.h"
#include "EffectChain.h"
#include "ModEngine.h"

class MechanOddAudioProcessor  : public juce::AudioProcessor
{
public:
    static constexpr int numVoices = 8;

    MechanOddAudioProcessor();
    ~MechanOddAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Per-column "entering signal" level (linear peak), refreshed every block for
    // the matrix meters. Columns are the matrix feedback sources: source slots
    // first, then resonator slots. The level is summed over all voices (taken from
    // the voice-summed buffers), so it is a whole-synth aggregate, not per-voice.
    // The GUI scales it by each cell's knob gain to show the post-gain contribution.
    // Audio thread writes, GUI reads.
    float getColumnLevelLinear (int col) const
    {
        return columnLevel[(size_t) juce::jlimit (0, FeedbackMatrix::numColumns - 1, col)]
                   .load (std::memory_order_relaxed);
    }

    // Post-master output level for the bottom-bar stereo meter, in dB. Two
    // ballistics per channel: a fast peak (the moving bar) and a slow peak-hold
    // (the trailing marker). Audio thread writes, GUI reads.
    float getOutputLevelDb (int ch) const { return outLevel[(size_t) juce::jlimit (0, 1, ch)].load (std::memory_order_relaxed); }
    float getOutputHoldDb  (int ch) const { return outHold [(size_t) juce::jlimit (0, 1, ch)].load (std::memory_order_relaxed); }

    static constexpr const char* outputVolumeId  = "outputVolume";
    static constexpr const char* numVoicesId     = "numVoices";
    static constexpr const char* portamentoId    = "portamento";
    static constexpr const char* busPostMasterId = "bus_postMaster";

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    struct FactoryPreset
    {
        juce::String name;         // display name derived from filename
        juce::String resourceName; // BinaryData symbol name (e.g. "Cool_Preset_xml")
    };
    std::vector<FactoryPreset> factoryPresets;
    int currentProgram { 0 };
    void buildFactoryPresets();

    PolySynth synth;

    // Global (not per-voice) resonators and their matrix role.
    std::array<ResonatorSlot, ResonatorSlot::numSlots> globalResonators;
    FeedbackMatrix globalMatrix;

    juce::AudioBuffer<float> inputCapture;  // host audio input, stereo (for Input L/R sources)
    juce::AudioBuffer<float> columnSum;     // FeedbackMatrix::numColumns channels
    juce::AudioBuffer<float> globalOutA, globalOutB;  // ping-pong: numResonators channels
    bool useAasPrev { true };

    std::array<std::atomic<float>, FeedbackMatrix::numColumns> columnLevel {};
    std::array<float, FeedbackMatrix::numColumns> columnEnv {};   // meter peak-hold state (audio thread)

    // Output section.
    std::atomic<float>* pOutputVolume  { nullptr };
    std::atomic<float>* pNumVoices     { nullptr };
    std::atomic<float>* pBusPostMaster { nullptr };
    juce::LinearSmoothedValue<float> outputGain { 1.0f };
    std::array<std::atomic<float>, 2> outLevel {};   // dB, fast peak (GUI reads)
    std::array<std::atomic<float>, 2> outHold  {};   // dB, slow peak-hold
    std::array<float, 2> outEnv {}, outHoldEnv {};   // linear envelope state (audio thread)

    // Effect chains: send bus and master.
    EffectChain busChain, masterChain;
    juce::AudioBuffer<float> sendBus;           // stereo
    juce::AudioBuffer<float> prevSendBusOut;    // mono, previous block's bus-chain output for matrix column

    ModEngine modEngine;
    int       heldNoteCount { 0 };   // held MIDI notes, drives the global ADSR gate

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MechanOddAudioProcessor)
};

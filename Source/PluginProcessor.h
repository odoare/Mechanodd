/*
  ==============================================================================

    PluginProcessor.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SynthVoice.h"
#include "SynthSound.h"
#include "ResonatorSlot.h"
#include "FeedbackMatrix.h"
#include "EffectChain.h"
#include "ModEngine.h"

class MechanoscAudioProcessor  : public juce::AudioProcessor
{
public:
    static constexpr int numVoices = 8;

    MechanoscAudioProcessor();
    ~MechanoscAudioProcessor() override;

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

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::Synthesiser synth;

    // Global (not per-voice) resonators and their matrix role.
    std::array<ResonatorSlot, ResonatorSlot::numSlots> globalResonators;
    FeedbackMatrix globalMatrix;

    juce::AudioBuffer<float> inputCapture;  // host audio input, stereo (for Input L/R sources)
    juce::AudioBuffer<float> columnSum;     // FeedbackMatrix::numColumns channels
    juce::AudioBuffer<float> globalOutA, globalOutB;  // ping-pong: numResonators channels
    bool useAasPrev { true };

    // Effect chains: send bus and master.
    EffectChain busChain, masterChain;
    juce::AudioBuffer<float> sendBus;       // stereo

    ModEngine modEngine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MechanoscAudioProcessor)
};

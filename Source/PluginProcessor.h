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
#include <map>

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

    // Factory (BinaryData) + user (XML files) preset banks; the editor's
    // top-bar preset bar (and its browser) and the host program list both drive this.
    fxme::PresetManager& getPresetManager() noexcept { return presetManager; }

    // The module presets of one effect in one slot (local presets, shared
    // with FxmeFX's own effect plugins: FX-Mechanics/Modules/<Effect>/Presets),
    // by the slot's per-type prefix (EffectSlot::perTypePrefix, e.g.
    // "master_fx0_Comp"). Null for an unknown prefix.
    // The effect chains: the send bus and the master. The effects tab binds each
    // effect's panel to the instance here (message thread).
    EffectChain& getBusChain() noexcept    { return busChain; }
    EffectChain& getMasterChain() noexcept { return masterChain; }

    fxme::ModulePresetTarget* getEffectPresets (const juce::String& perTypePrefix) const
    {
        const auto it = effectPresetTargets.find (perTypePrefix);
        return it != effectPresetTargets.end() ? it->second.get() : nullptr;
    }

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
    static constexpr const char* busOutVolId     = "bus_outVol";

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    fxme::PresetManager presetManager;   // must be declared after apvts

    // Effect module presets: one library per effect type (aligned with
    // EffectFactory::types()), shared by every slot, so a preset saved in one
    // slot is listed in all of them at once; one target per effect per slot
    // (every slot holds an instance of every effect). Made in the ctor body,
    // once the APVTS exists; the targets are declared after the libraries
    // (and both after the APVTS) so they go first.
    std::vector<std::unique_ptr<fxme::ModulePresetLibrary>> effectPresetLibraries;
    std::map<juce::String, std::unique_ptr<fxme::ModulePresetTarget>> effectPresetTargets;
    void createEffectPresets();

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
    std::atomic<float>* pBusOutVol     { nullptr };
    juce::LinearSmoothedValue<float> busOutputGain { 1.0f };
    juce::LinearSmoothedValue<float> outputGain { 1.0f };
    std::array<std::atomic<float>, 2> outLevel {};   // dB, fast peak (GUI reads)
    std::array<std::atomic<float>, 2> outHold  {};   // dB, slow peak-hold
    std::array<float, 2> outEnv {}, outHoldEnv {};   // linear envelope state (audio thread)

    // Effect chains: send bus and master.
    EffectChain busChain, masterChain;

    // Loads the IR of a Cab or Reverb slot once the slot is set to it (see
    // EffectSlot::loadActiveEffect), off the audio thread: a plain thread
    // polling the slot types, so it works with no editor open (automation,
    // a preset or a session load can change a slot). Declared after the
    // chains; stopped first thing in the destructor.
    class EffectLoader : public juce::Thread
    {
    public:
        explicit EffectLoader (MechanOddAudioProcessor& p)
            : juce::Thread ("MechanOdd effect loader"), owner (p) {}

        void run() override
        {
            while (! threadShouldExit())
            {
                owner.busChain.loadActiveEffects();
                owner.masterChain.loadActiveEffects();
                wait (20);
            }
        }

    private:
        MechanOddAudioProcessor& owner;
    };
    EffectLoader effectLoader { *this };
    juce::AudioBuffer<float> sendBus;           // stereo
    juce::AudioBuffer<float> prevSendBusOut;    // mono, previous block's bus-chain output for matrix column

    ModEngine modEngine;
    int       heldNoteCount { 0 };   // held MIDI notes, drives the global ADSR gate

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MechanOddAudioProcessor)
};

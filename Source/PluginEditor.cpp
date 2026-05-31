/*
  ==============================================================================

    PluginEditor.cpp

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ResonatorSlot.h"

MechanoscAudioProcessorEditor::MechanoscAudioProcessorEditor (MechanoscAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    sourcesTab = std::make_unique<juce::Component>();
    for (int i = 0; i < SynthVoice::numSourceSlots; ++i)
    {
        auto slot = std::make_unique<SourceSlotComponent> (audioProcessor.apvts, SynthVoice::sourceSlotPrefix (i));
        sourcesTab->addAndMakeVisible (*slot);
        sourceSlots.push_back (std::move (slot));
    }

    resonatorsTab = std::make_unique<juce::Component>();
    for (int i = 0; i < ResonatorSlot::numSlots; ++i)
    {
        auto slot = std::make_unique<ResonatorSlotComponent> (audioProcessor.apvts, ResonatorSlot::slotPrefix (i));
        resonatorsTab->addAndMakeVisible (*slot);
        resonatorSlots.push_back (std::move (slot));
    }

    matrixComponent = std::make_unique<FeedbackMatrixComponent> (audioProcessor.apvts);

    effectsTab = std::make_unique<juce::Component>();
    busChainComponent    = std::make_unique<EffectChainComponent> (audioProcessor.apvts, "bus",    "Send Bus");
    masterChainComponent = std::make_unique<EffectChainComponent> (audioProcessor.apvts, "master", "Master");
    effectsTab->addAndMakeVisible (*busChainComponent);
    effectsTab->addAndMakeVisible (*masterChainComponent);

    tabs.addTab ("Sources",    juce::Colours::darkgrey, sourcesTab.get(), false);
    tabs.addTab ("Resonators", juce::Colours::darkgrey, resonatorsTab.get(), false);
    tabs.addTab ("Matrix",     juce::Colours::darkgrey, matrixComponent.get(), false);
    modulationComponent = std::make_unique<ModulationComponent> (audioProcessor.apvts);

    tabs.addTab ("Effects",    juce::Colours::darkgrey, effectsTab.get(), false);
    tabs.addTab ("LFO",        juce::Colours::darkgrey, modulationComponent.get(), false);
    addAndMakeVisible (tabs);

    setResizable (true, true);
    setSize (900, 480);
}

MechanoscAudioProcessorEditor::~MechanoscAudioProcessorEditor() = default;

void MechanoscAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MechanoscAudioProcessorEditor::resized()
{
    tabs.setBounds (getLocalBounds());

    if (sourcesTab != nullptr)
    {
        auto area = sourcesTab->getLocalBounds().reduced (8);
        const int rowH = 130;
        for (auto& slot : sourceSlots)
            slot->setBounds (area.removeFromTop (rowH).reduced (0, 3));
    }

    if (resonatorsTab != nullptr)
    {
        auto area = resonatorsTab->getLocalBounds().reduced (8);
        const int rowH = juce::jmax (110, area.getHeight() / juce::jmax (1, (int) resonatorSlots.size()));
        for (auto& slot : resonatorSlots)
            slot->setBounds (area.removeFromTop (rowH).reduced (0, 3));
    }

    if (effectsTab != nullptr)
    {
        auto area = effectsTab->getLocalBounds().reduced (8);
        const int half = area.getHeight() / 2;
        busChainComponent->setBounds (area.removeFromTop (half).reduced (0, 3));
        masterChainComponent->setBounds (area.reduced (0, 3));
    }
}

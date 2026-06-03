/*
  ==============================================================================

    PluginEditor.cpp

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ResonatorSlot.h"
#include "Theme.h"

MechanoscAudioProcessorEditor::MechanoscAudioProcessorEditor (MechanoscAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    sourcesTab = std::make_unique<juce::Component>();
    for (int i = 0; i < SynthVoice::numSourceSlots; ++i)
    {
        auto slot = std::make_unique<SourceSlotComponent> (audioProcessor.apvts, SynthVoice::sourceSlotPrefix (i));
        MechanoscTheme::applyAccent (*slot, MechanoscTheme::sourceColour (i));
        sourcesTab->addAndMakeVisible (*slot);
        sourceSlots.push_back (std::move (slot));
    }

    resonatorsTab = std::make_unique<juce::Component>();
    for (int i = 0; i < ResonatorSlot::numSlots; ++i)
    {
        auto slot = std::make_unique<ResonatorSlotComponent> (audioProcessor.apvts, ResonatorSlot::slotPrefix (i));
        MechanoscTheme::applyAccent (*slot, MechanoscTheme::resonatorColour (i));
        resonatorsTab->addAndMakeVisible (*slot);
        resonatorSlots.push_back (std::move (slot));
    }

    matrixComponent = std::make_unique<FeedbackMatrixComponent> (audioProcessor.apvts);
    matrixComponent->setColumnLevelProvider ([&p = audioProcessor] (int c) { return p.getColumnLevelLinear (c); });

    effectsTab = std::make_unique<juce::Component>();
    busChainComponent    = std::make_unique<EffectChainComponent> (audioProcessor.apvts, "bus",    "Send Bus");
    masterChainComponent = std::make_unique<EffectChainComponent> (audioProcessor.apvts, "master", "Master");
    effectsTab->addAndMakeVisible (*busChainComponent);
    effectsTab->addAndMakeVisible (*masterChainComponent);

    modulationComponent = std::make_unique<ModulationComponent> (audioProcessor.apvts);
    // The modulation page stays homogeneous; the matrix colours itself per row.
    MechanoscTheme::applyAccent (*modulationComponent, MechanoscTheme::modulation);

    tabs.addTab ("Sources",    MechanoscTheme::tabButton, sourcesTab.get(),          false);
    tabs.addTab ("Resonators", MechanoscTheme::tabButton, resonatorsTab.get(),       false);
    tabs.addTab ("Matrix",     MechanoscTheme::tabButton, matrixComponent.get(),     false);
    tabs.addTab ("Effects",    MechanoscTheme::tabButton, effectsTab.get(),          false);
    tabs.addTab ("Modulation", MechanoscTheme::tabButton, modulationComponent.get(), false);
    addAndMakeVisible (tabs);

    bottomBar = std::make_unique<BottomBarComponent> (audioProcessor);
    addAndMakeVisible (*bottomBar);

    setResizable (true, true);
    setSize (1280, 600);
}

MechanoscAudioProcessorEditor::~MechanoscAudioProcessorEditor() = default;

void MechanoscAudioProcessorEditor::paint (juce::Graphics& g)
{
    MechanoscTheme::paintBackground (g, getLocalBounds().toFloat());
}

void MechanoscAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    if (bottomBar != nullptr)
        bottomBar->setBounds (area.removeFromBottom (84));

    tabs.setBounds (area);

    if (sourcesTab != nullptr)
    {
        auto area = sourcesTab->getLocalBounds().reduced (8);
        const int rowH = juce::jmax (110, area.getHeight() / juce::jmax (1, (int) sourceSlots.size()));
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

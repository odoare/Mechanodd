/*
  ==============================================================================

    PluginEditor.cpp

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ResonatorSlot.h"
#include "Theme.h"

MechanOddAudioProcessorEditor::MechanOddAudioProcessorEditor (MechanOddAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    sourcesTab = std::make_unique<juce::Component>();
    for (int i = 0; i < SynthVoice::numSourceSlots; ++i)
    {
        auto slot = std::make_unique<SourceSlotComponent> (audioProcessor.apvts, SynthVoice::sourceSlotPrefix (i));
        MechanOddTheme::applyAccent (*slot, MechanOddTheme::sourceColour (i));
        sourcesTab->addAndMakeVisible (*slot);
        sourceSlots.push_back (std::move (slot));
    }

    resonatorsTab = std::make_unique<juce::Component>();
    for (int i = 0; i < ResonatorSlot::numSlots; ++i)
    {
        auto slot = std::make_unique<ResonatorSlotComponent> (audioProcessor.apvts, ResonatorSlot::slotPrefix (i));
        MechanOddTheme::applyAccent (*slot, MechanOddTheme::resonatorColour (i));
        slot->setMeterColour (MechanOddTheme::resonatorColour (i));
        // This resonator's output is column (numSources + i) of the feedback matrix.
        slot->setLevelProvider ([&p = audioProcessor, col = FeedbackMatrix::numSources + i]
                                { return p.getColumnLevelLinear (col); });
        resonatorsTab->addAndMakeVisible (*slot);
        resonatorSlots.push_back (std::move (slot));
    }

    matrixComponent = std::make_unique<FeedbackMatrixComponent> (audioProcessor.apvts);
    matrixComponent->setColumnLevelProvider ([&p = audioProcessor] (int c) { return p.getColumnLevelLinear (c); });

    effectsTabComponent = std::make_unique<EffectsTabComponent> (audioProcessor.apvts);

    modulationComponent = std::make_unique<ModulationComponent> (audioProcessor.apvts);
    // The modulation page stays homogeneous; the matrix colours itself per row.
    MechanOddTheme::applyAccent (*modulationComponent, MechanOddTheme::modulation);

    tabs.addTab ("Sources",    MechanOddTheme::tabButton, sourcesTab.get(),          false);
    tabs.addTab ("Resonators", MechanOddTheme::tabButton, resonatorsTab.get(),       false);
    tabs.addTab ("Matrix",     MechanOddTheme::tabButton, matrixComponent.get(),     false);
    tabs.addTab ("Effects",    MechanOddTheme::tabButton, effectsTabComponent.get(), false);
    tabs.addTab ("Modulation", MechanOddTheme::tabButton, modulationComponent.get(), false);
    addAndMakeVisible (tabs);

    bottomBar = std::make_unique<BottomBarComponent> (audioProcessor);
    addAndMakeVisible (*bottomBar);

    setResizable (true, true);
    setSize (1280, 700);
}

MechanOddAudioProcessorEditor::~MechanOddAudioProcessorEditor() = default;

void MechanOddAudioProcessorEditor::paint (juce::Graphics& g)
{
    MechanOddTheme::paintBackground (g, getLocalBounds().toFloat());
}

void MechanOddAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    if (bottomBar != nullptr)
        bottomBar->setBounds (area.removeFromBottom (100));

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

    // EffectsTabComponent manages its own layout internally.
}

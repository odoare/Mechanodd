/*
  ==============================================================================

    PluginEditor.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SourceSlotComponent.h"
#include "ResonatorSlotComponent.h"
#include "FeedbackMatrixComponent.h"
#include "EffectsTabComponent.h"
#include "ModulationComponent.h"
#include "BottomBarComponent.h"
#include "Theme.h"

// TabbedComponent that paints the shared StrinGO gradient behind every tab
// instead of the default per-tab background fill.
class GradientTabbedComponent : public juce::TabbedComponent
{
public:
    GradientTabbedComponent() : juce::TabbedComponent (juce::TabbedButtonBar::TabsAtTop) {}
    void paint (juce::Graphics& g) override { MechanOddTheme::paintBackground (g, getLocalBounds().toFloat()); }
};

class MechanOddAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    MechanOddAudioProcessorEditor (MechanOddAudioProcessor&);
    ~MechanOddAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    MechanOddAudioProcessor& audioProcessor;

    GradientTabbedComponent tabs;

    std::unique_ptr<juce::Component> sourcesTab;
    std::vector<std::unique_ptr<SourceSlotComponent>> sourceSlots;

    std::unique_ptr<juce::Component> resonatorsTab;
    std::vector<std::unique_ptr<ResonatorSlotComponent>> resonatorSlots;

    std::unique_ptr<FeedbackMatrixComponent> matrixComponent;

    std::unique_ptr<EffectsTabComponent> effectsTabComponent;

    std::unique_ptr<ModulationComponent> modulationComponent;

    std::unique_ptr<BottomBarComponent> bottomBar;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MechanOddAudioProcessorEditor)
};

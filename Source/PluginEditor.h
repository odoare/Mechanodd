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
#include "GlobalPanel.h"
#include "OutputMeter.h"
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
    /** Opens the voice count and portamento in a callout under the gear. */
    void showGlobalPanel();

    /** The FX-Mechanics logo, from the binary data. */
    static juce::Image logoImage();

    MechanOddAudioProcessor& audioProcessor;

    // For the top bar's own controls (the output fader); the tabs keep their
    // look-and-feels.
    fxme::FxmeLookAndFeel laf;

    // The top bar: logo, name, then the output level (a fader and the
    // stereo meter), the global presets ("..." opens the browser) and the
    // gear for the global settings, then the version.
    fxme::TopBar topBar { MechanOddTheme::displayName, MechanOddTheme::tagline,
                          JucePlugin_VersionString, logoImage() };
    fxme::FxmeSlider outputSlider;
    OutputMeter outputMeter { audioProcessor };
    fxme::PresetBarComponent presetBar { audioProcessor.getPresetManager() };
    GearButton globalButton { MechanOddTheme::modulation };

    GradientTabbedComponent tabs;

    std::unique_ptr<juce::Component> sourcesTab;
    std::vector<std::unique_ptr<SourceSlotComponent>> sourceSlots;

    std::unique_ptr<juce::Component> resonatorsTab;
    std::vector<std::unique_ptr<ResonatorSlotComponent>> resonatorSlots;

    std::unique_ptr<FeedbackMatrixComponent> matrixComponent;

    std::unique_ptr<EffectsTabComponent> effectsTabComponent;

    std::unique_ptr<ModulationComponent> modulationComponent;

    // Typing in the preset browsers' name fields (and in the knobs' value
    // entry) in a hosted window. Exactly one, declared after the children.
    fxme::TextEntryFocusFixer textEntryFixer { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MechanOddAudioProcessorEditor)
};

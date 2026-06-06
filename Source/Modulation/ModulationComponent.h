/*
  ==============================================================================

    ModulationComponent.h

    The modulation tab: a row per modulator. Each row can be an LFO or an ADSR
    envelope (chosen by a type combo, which reveals the matching controls), and
    its target is picked with a two-level module -> parameter pair of combos that
    map onto the single underlying flat target choice parameter.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "ModEngine.h"

class ModulationComponent : public juce::Component
{
public:
    explicit ModulationComponent (juce::AudioProcessorValueTreeState& apvts);
    ~ModulationComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    // Two-level mapping of the flat target choice list (index 0 == "None").
    struct ParamEntry
    {
        juce::String label;      // human-readable param name
        int          flatIndex;  // index in the flat target choice list
        juce::String paramId;    // APVTS parameter ID (used for effect-type filtering)
    };
    struct ModuleGroup
    {
        juce::String            label;    // e.g. "Source 0", "Matrix", "Bus FX 0"
        std::vector<ParamEntry> params;
    };
    std::vector<ModuleGroup> targetGroups;
    void buildTargetGroups();

    // Returns false for effect-slot params whose effect type is not currently active.
    bool shouldShowParam (const juce::String& groupLabel, const juce::String& paramId) const;

    struct Row
    {
        juce::Label index;

        // Common.
        juce::ComboBox typeBox;
        juce::ComboBox moduleBox, paramBox;     // two-level target picker
        std::unique_ptr<ComboAtt> typeAtt;
        std::unique_ptr<juce::ParameterAttachment> targetAtt;   // syncs the flat target param

        // LFO controls.
        juce::ComboBox shapeBox, syncRateBox;
        juce::ToggleButton syncButton;
        std::unique_ptr<fxme::FxmeSlider> rate, depth;
        std::unique_ptr<ComboAtt>  shapeAtt, syncRateAtt;
        std::unique_ptr<ButtonAtt> syncAtt;

        // ADSR controls.
        std::unique_ptr<fxme::FxmeSlider> attack, decay, sustain, release, amount;
        juce::ComboBox polarityBox;
        std::unique_ptr<ComboAtt> polarityAtt;

        bool updatingTarget { false };          // guards the target<->module/param sync
    };

    void syncTargetBoxesFromFlat (Row& row);    // flat index -> module/param boxes
    void applyTargetFromBoxes (Row& row);       // module/param boxes -> flat index
    void updateRowTypeVisibility (Row& row);

    juce::AudioProcessorValueTreeState& apvts;
    std::array<Row, ModEngine::numModulators> rows;
    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModulationComponent)
};

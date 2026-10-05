/*
  ==============================================================================

    EffectsTabComponent.h

    Full-tab effects view: a left column with 8 type selectors (4 Send + 4
    Master) each with an exclusive "show" button, and a right panel that
    displays the selected slot's effect controls at full size, under a preset
    bar for that effect in that slot (its module presets, shared with the
    effect's own FxmeFX plugin; "..." opens the browser).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Effect.h"
#include "EffectFactory.h"

class MechanOddAudioProcessor;

class EffectsTabComponent : public juce::Component
{
public:
    explicit EffectsTabComponent (MechanOddAudioProcessor& processor);
    ~EffectsTabComponent() override;

    void paint  (juce::Graphics& g) override;
    void resized() override;

private:
    struct SlotEntry
    {
        juce::String     slotPrefix;   // "bus_fx0"
        juce::ComboBox   typeBox;
        juce::TextButton showButton;

        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAtt;
    };

    MechanOddAudioProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;

    static constexpr int kSlotsPerChain = 4;
    static constexpr int kNumSlots      = 8;   // 2 chains × 4 slots

    std::array<std::unique_ptr<SlotEntry>, kNumSlots> slots;

    juce::Label sendLabel, masterLabel;
    juce::Label placeholderLabel;

    juce::ToggleButton prePostButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> prePostAtt;

    std::unique_ptr<fxme::FxmeSlider> sendVolumeSlider;

    // The shown effect's module presets. Remade when another slot or effect
    // is shown (a bar is tied to one bank for its lifetime).
    std::unique_ptr<fxme::PresetBarComponent> presetBar;
    fxme::PresetBank* presetBarBank = nullptr;

    // The shown effect's panel, made when shown and dropped when another one
    // is (rather than a panel for every effect in every slot up front: the
    // Cab and Reverb instances behind them load an IR as soon as they
    // exist). The GUI-side effect instance backs the panel; the audio one is
    // in the processor. The panel is declared after its effect, so it goes
    // first.
    std::unique_ptr<Effect> shownEffect;
    std::unique_ptr<juce::Component> shownPanel;
    int shownSlot = -1, shownType = -1;
    void showPanel (int slot, int type);

    int  activeSlot = -1;
    fxme::FxmeLookAndFeel laf;

    void initSlot    (int slotIdx, const juce::String& chainPrefix, int posInChain);
    void activateSlot (int slotIdx);
    void refreshRightPanel();

    juce::Rectangle<int> rightBounds() const;
    juce::Rectangle<int> centredBounds (const EffectTypeInfo& info) const;
    juce::Rectangle<int> presetBarBounds (const EffectTypeInfo& info) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectsTabComponent)
};

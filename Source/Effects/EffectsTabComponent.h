/*
  ==============================================================================

    EffectsTabComponent.h

    Full-tab effects view: a left column with 8 type selectors (4 Send + 4
    Master) each with an exclusive "show" button, and a right panel that
    displays the selected slot's effect controls at full size.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Effect.h"

class EffectsTabComponent : public juce::Component
{
public:
    EffectsTabComponent (juce::AudioProcessorValueTreeState& apvts);
    ~EffectsTabComponent() override;

    void paint  (juce::Graphics& g) override;
    void resized() override;

private:
    struct SlotEntry
    {
        juce::ComboBox   typeBox;
        juce::TextButton showButton;

        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAtt;
        std::vector<std::unique_ptr<Effect>>            guiEffects;
        std::vector<std::unique_ptr<juce::Component>>   typeComponents;
    };

    juce::AudioProcessorValueTreeState& apvts;

    static constexpr int kSlotsPerChain = 4;
    static constexpr int kNumSlots      = 8;   // 2 chains × 4 slots

    std::array<std::unique_ptr<SlotEntry>, kNumSlots> slots;

    juce::Label sendLabel, masterLabel;
    juce::Label placeholderLabel;

    int  activeSlot = -1;
    fxme::FxmeLookAndFeel laf;

    void initSlot    (int slotIdx, const juce::String& chainPrefix, int posInChain);
    void activateSlot (int slotIdx);
    void refreshRightPanel();

    juce::Rectangle<int> rightBounds() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectsTabComponent)
};

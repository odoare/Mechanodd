/*
  ==============================================================================

    EffectSlotComponent.h

    GUI for one effect slot: a type selector plus the active effect's own
    component. One component per effect type is created up front; only the active
    one is shown.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Effect.h"

class EffectSlotComponent : public juce::Component
{
public:
    EffectSlotComponent (juce::AudioProcessorValueTreeState& apvts, const juce::String& slotPrefix);
    ~EffectSlotComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void updateActiveComponent();

    juce::AudioProcessorValueTreeState& apvts;
    juce::String slotPrefix;

    juce::ComboBox typeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAtt;

    // Owned effect instances used purely to back the GUI components, aligned with
    // EffectFactory::types(). (The audio-thread instances live in EffectSlot.)
    std::vector<std::unique_ptr<Effect>> guiEffects;
    std::vector<std::unique_ptr<juce::Component>> typeComponents;

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectSlotComponent)
};

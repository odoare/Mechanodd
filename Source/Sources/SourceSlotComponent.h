/*
  ==============================================================================

    SourceSlotComponent.h

    GUI for one source slot: a type selector plus the active type's component.
    One component per type is created up front; only the active one is shown.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class SourceSlotComponent : public juce::Component
{
public:
    SourceSlotComponent (juce::AudioProcessorValueTreeState& apvts, const juce::String& slotPrefix);
    ~SourceSlotComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void updateActiveComponent();

    juce::AudioProcessorValueTreeState& apvts;
    juce::String slotPrefix;

    juce::Label slotLabel;
    juce::ComboBox typeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAtt;

    // Aligned with SourceFactory::types().
    std::vector<std::unique_ptr<juce::Component>> typeComponents;

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SourceSlotComponent)
};

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

    // Geometry of the top-left header (slot label + source-type selector).
    // Shared so a type component that spans the full slot can reserve this
    // corner and align its own controls directly under the type selector.
    static constexpr int headerWidth    = 90;
    static constexpr int labelHeight     = 20;
    static constexpr int typeBoxHeight   = 24;
    static constexpr int headerHeight    = labelHeight + typeBoxHeight;

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

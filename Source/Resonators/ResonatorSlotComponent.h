/*
  ==============================================================================

    ResonatorSlotComponent.h

    GUI for one resonator slot: a type selector, a per-voice/global toggle, a
    global base-frequency knob (relevant only when global), and the active
    type's component. One component per type is created up front; only the
    active one is shown.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class ResonatorSlotComponent : public juce::Component
{
public:
    ResonatorSlotComponent (juce::AudioProcessorValueTreeState& apvts, const juce::String& slotPrefix);
    ~ResonatorSlotComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void updateActiveComponent();

    juce::AudioProcessorValueTreeState& apvts;
    juce::String slotPrefix;

    juce::Label slotLabel;
    juce::ComboBox typeBox;
    juce::ToggleButton globalButton { "Global" };
    juce::Label freqLabel;
    std::unique_ptr<fxme::FxmeSlider> freqSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   globalAtt;

    // Aligned with ResonatorFactory::types().
    std::vector<std::unique_ptr<juce::Component>> typeComponents;

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonatorSlotComponent)
};

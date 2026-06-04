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
#include "VuMeterComponent.h"

class ResonatorSlotComponent : public juce::Component,
                                private juce::Timer
{
public:
    ResonatorSlotComponent (juce::AudioProcessorValueTreeState& apvts, const juce::String& slotPrefix);
    ~ResonatorSlotComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Supplies this resonator's post-level output level (linear peak) for the
    // vertical meter. Set by the editor from the processor's column-level data.
    void setLevelProvider (std::function<float()> fn) { levelProvider = std::move (fn); }

    // Tint the output meter with this slot's group colour (the knobs are tinted
    // separately by MechanoscTheme::applyAccent, which doesn't touch the meter).
    void setMeterColour (juce::Colour c) { levelMeter.setMeterColor (c); }

private:
    void updateActiveComponent();
    void timerCallback() override;

    juce::AudioProcessorValueTreeState& apvts;
    juce::String slotPrefix;

    juce::Label slotLabel;
    juce::ComboBox typeBox;
    juce::ToggleButton globalButton { "Global" };
    juce::Label freqLabel;
    std::unique_ptr<fxme::FxmeSlider> freqSlider;

    juce::Label levelLabel;
    std::unique_ptr<fxme::FxmeSlider> levelSlider;

    VuMeterComponent levelMeter;
    std::function<float()> levelProvider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   globalAtt;

    // Aligned with ResonatorFactory::types().
    std::vector<std::unique_ptr<juce::Component>> typeComponents;

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonatorSlotComponent)
};

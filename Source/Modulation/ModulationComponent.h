/*
  ==============================================================================

    ModulationComponent.h

    The LFO/modulation tab: a row per modulator with target, shape, rate, sync,
    sync rate and depth controls, all bound to the APVTS.

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

    struct Row
    {
        juce::Label index;
        juce::ComboBox targetBox, shapeBox, syncRateBox;
        juce::ToggleButton syncButton;
        std::unique_ptr<fxme::FxmeSlider> rate, depth;
        std::unique_ptr<ComboAtt>  targetAtt, shapeAtt, syncRateAtt;
        std::unique_ptr<ButtonAtt> syncAtt;
    };

    juce::AudioProcessorValueTreeState& apvts;
    std::array<Row, ModEngine::numModulators> rows;
    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModulationComponent)
};

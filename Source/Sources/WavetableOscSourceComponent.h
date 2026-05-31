/*
  ==============================================================================

    WavetableOscSourceComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SourceCommonComponent.h"

class WavetableOscSourceComponent : public juce::Component
{
public:
    WavetableOscSourceComponent (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    ~WavetableOscSourceComponent() override;

    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::String prefix;

    juce::Label   waveLabel, modeLabel, tuneLabel;
    juce::ComboBox waveBox, modeBox;
    std::unique_ptr<fxme::FxmeSlider> tuneSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveAtt, modeAtt;

    SourceCommonComponent common;
    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavetableOscSourceComponent)
};

/*
  ==============================================================================

    CracksSourceComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SourceCommonComponent.h"

class CracksSourceComponent : public juce::Component
{
public:
    CracksSourceComponent (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    ~CracksSourceComponent() override;

    void resized() override;

private:
    juce::Label densityLabel;
    std::unique_ptr<fxme::FxmeSlider> densitySlider;

    SourceCommonComponent common;
    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CracksSourceComponent)
};

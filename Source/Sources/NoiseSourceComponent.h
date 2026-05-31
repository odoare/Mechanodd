/*
  ==============================================================================

    NoiseSourceComponent.h

    GUI for a NoiseSource slot. Noise has no parameters beyond the shared set,
    so this is just the common controls.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SourceCommonComponent.h"

class NoiseSourceComponent : public juce::Component
{
public:
    NoiseSourceComponent (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
        : common (apvts, prefix)
    {
        addAndMakeVisible (common);
    }

    void resized() override { common.setBounds (getLocalBounds()); }

private:
    SourceCommonComponent common;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoiseSourceComponent)
};

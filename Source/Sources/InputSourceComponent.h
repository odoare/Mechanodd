/*
  ==============================================================================

    InputSourceComponent.h

    GUI for a Plugin Input source. It has no parameters beyond the shared set,
    so this is just the common controls. Used for both Input L and Input R.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SourceCommonComponent.h"

class InputSourceComponent : public juce::Component
{
public:
    InputSourceComponent (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
        : common (apvts, prefix)
    {
        addAndMakeVisible (common);
    }

    void resized() override { common.setBounds (getLocalBounds()); }

private:
    SourceCommonComponent common;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InputSourceComponent)
};

/*
  ==============================================================================

    EffectChainComponent.h

    Shows a chain's 4 effect slots side by side, with a chain title.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "EffectSlotComponent.h"

class EffectChainComponent : public juce::Component
{
public:
    EffectChainComponent (juce::AudioProcessorValueTreeState& apvts,
                          const juce::String& chainPrefix, const juce::String& title);

    void resized() override;

private:
    juce::Label titleLabel;
    std::vector<std::unique_ptr<EffectSlotComponent>> slots;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectChainComponent)
};

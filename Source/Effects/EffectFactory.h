/*
  ==============================================================================

    EffectFactory.h

    Registry of available effects. One entry per effect; slots, parameters and
    GUI derive from this list.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Effect.h"

struct EffectTypeInfo
{
    juce::String name;
    std::function<std::unique_ptr<Effect>()> create;
    std::function<std::unique_ptr<juce::Component> (Effect&, juce::AudioProcessorValueTreeState&, const juce::String&)> createComponent;
    int preferredWidth  = 600;
    int preferredHeight = 400;
};

namespace EffectFactory
{
    const std::vector<EffectTypeInfo>& types();

    // "Off" followed by every registered effect name.
    juce::StringArray typeChoices();
}

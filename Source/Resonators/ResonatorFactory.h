/*
  ==============================================================================

    ResonatorFactory.h

    Central registry of resonator types. Adding a new resonator type means one
    entry here (plus its files); slots, parameters and GUI all derive from this
    list. Unlike sources there is no "Off" entry: the "Block" type is the silent
    option and "Passthrough" the transparent one.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Resonator.h"

struct ResonatorTypeInfo
{
    juce::String name;   // id-safe key: parameter prefixes and lookups use this
    std::function<std::unique_ptr<Resonator>()> createResonator;
    std::function<std::unique_ptr<juce::Component> (juce::AudioProcessorValueTreeState&, const juce::String&)> createComponent;
    juce::String displayName;   // shown in the type selector; falls back to name if empty
};

namespace ResonatorFactory
{
    const std::vector<ResonatorTypeInfo>& types();

    // Type names in registry order.
    juce::StringArray typeChoices();

    // Index of a named type (e.g. the default "Block"), or 0 if not found.
    int indexOf (const juce::String& name);
}

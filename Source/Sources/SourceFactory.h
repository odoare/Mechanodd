/*
  ==============================================================================

    SourceFactory.h

    Central registry of source types. Adding a new source type means adding one
    entry here (and its files); slots, parameters and GUI all derive from this
    list, so nothing else needs editing.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Source.h"

struct SourceTypeInfo
{
    juce::String name;
    std::function<std::unique_ptr<Source>()> createSource;
    std::function<std::unique_ptr<juce::Component> (juce::AudioProcessorValueTreeState&, const juce::String&)> createComponent;
};

namespace SourceFactory
{
    const std::vector<SourceTypeInfo>& types();

    // "Off" followed by every registered type name, in registry order.
    juce::StringArray typeChoices();
}

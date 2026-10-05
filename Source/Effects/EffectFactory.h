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
    juce::String name;          // shown in the slot's type box, and part of the parameter IDs

    // The FxmeFX effect's module presets (FxmeFX Source/Common/EffectPresets.h):
    // the module is the effect's class name ("Compressor"), shared with the
    // effect's own plugin and every other host; the tag is the one the effect
    // puts after its prefix in its parameter IDs ("Comp").
    juce::String moduleName;
    juce::String paramTag;

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

    // The ID prefix of one effect's own parameters in a slot, given the
    // slot's per-type prefix (EffectSlot::perTypePrefix): the scope of its
    // module presets, "<perTypePrefix>_<Tag>_".
    inline juce::String presetPrefix (const juce::String& perTypePrefix, const EffectTypeInfo& info)
    {
        return perTypePrefix + "_" + info.paramTag + "_";
    }
}

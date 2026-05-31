/*
  ==============================================================================

    EffectFactory.cpp

  ==============================================================================
*/

#include "EffectFactory.h"

#include "StereoDelay.h"
#include "StereoDelayComponent.h"
#include "Tube.h"
#include "TubeComponent.h"
#include "Equalizer.h"
#include "EqualizerComponent.h"
#include "Oct.h"
#include "OctComponent.h"

namespace
{
    // Builds a registry entry for effect class FX with component class FXComponent.
    template <class FX, class FXComponent>
    EffectTypeInfo makeEntry (const juce::String& name)
    {
        return {
            name,
            []                                       { return std::unique_ptr<Effect> (new EffectAdapter<FX>()); },
            [] (Effect& e, juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
            {
                auto& impl = static_cast<EffectAdapter<FX>&> (e).get();
                return std::unique_ptr<juce::Component> (new FXComponent (impl, apvts, prefix));
            }
        };
    }
}

const std::vector<EffectTypeInfo>& EffectFactory::types()
{
    static const std::vector<EffectTypeInfo> registry = {
        makeEntry<StereoDelay, StereoDelayComponent> ("Delay"),
        makeEntry<Tube,        TubeComponent>        ("Tube"),
        makeEntry<Equalizer,   EqualizerComponent>  ("EQ"),
        makeEntry<Oct,         OctComponent>         ("Oct")
    };
    return registry;
}

juce::StringArray EffectFactory::typeChoices()
{
    juce::StringArray choices;
    choices.add ("Off");
    for (const auto& info : types())
        choices.add (info.name);
    return choices;
}

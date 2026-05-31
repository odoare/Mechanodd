/*
  ==============================================================================

    SourceFactory.cpp

  ==============================================================================
*/

#include "SourceFactory.h"
#include "NoiseSource.h"
#include "NoiseSourceComponent.h"
#include "WavetableOscSource.h"
#include "WavetableOscSourceComponent.h"
#include "CracksSource.h"
#include "CracksSourceComponent.h"
#include "InputSource.h"
#include "InputSourceComponent.h"

const std::vector<SourceTypeInfo>& SourceFactory::types()
{
    static const std::vector<SourceTypeInfo> registry = {
        { WavetableOscSource::typeName,
          []                                                              { return std::unique_ptr<Source> (new WavetableOscSource()); },
          [] (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
                                                                          { return std::unique_ptr<juce::Component> (new WavetableOscSourceComponent (apvts, prefix)); } },

        { NoiseSource::typeName,
          []                                                              { return std::unique_ptr<Source> (new NoiseSource()); },
          [] (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
                                                                          { return std::unique_ptr<juce::Component> (new NoiseSourceComponent (apvts, prefix)); } },

        { CracksSource::typeName,
          []                                                              { return std::unique_ptr<Source> (new CracksSource()); },
          [] (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
                                                                          { return std::unique_ptr<juce::Component> (new CracksSourceComponent (apvts, prefix)); } },

        { InputLeftSource::typeName,
          []                                                              { return std::unique_ptr<Source> (new InputLeftSource()); },
          [] (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
                                                                          { return std::unique_ptr<juce::Component> (new InputSourceComponent (apvts, prefix)); } },

        { InputRightSource::typeName,
          []                                                              { return std::unique_ptr<Source> (new InputRightSource()); },
          [] (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
                                                                          { return std::unique_ptr<juce::Component> (new InputSourceComponent (apvts, prefix)); } }
    };
    return registry;
}

juce::StringArray SourceFactory::typeChoices()
{
    juce::StringArray choices;
    choices.add ("Off");
    for (const auto& info : types())
        choices.add (info.name);
    return choices;
}

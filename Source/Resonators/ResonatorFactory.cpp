/*
  ==============================================================================

    ResonatorFactory.cpp

  ==============================================================================
*/

#include "ResonatorFactory.h"
#include "PassthroughResonator.h"
#include "BlockResonator.h"
#include "WaveguideResonator.h"
#include "PlateResonator.h"
#include "MembraneResonator.h"
#include "WaveguideComponent.h"
#include "ModalResonatorComponent.h"

namespace
{
    std::unique_ptr<juce::Component> makeInfoComponent (const juce::String& text)
    {
        auto label = std::make_unique<juce::Label>();
        label->setText (text, juce::dontSendNotification);
        label->setJustificationType (juce::Justification::centred);
        return label;
    }
}

const std::vector<ResonatorTypeInfo>& ResonatorFactory::types()
{
    static const std::vector<ResonatorTypeInfo> registry = {
        { PassthroughResonator::typeName,
          []                                                              { return std::unique_ptr<Resonator> (new PassthroughResonator()); },
          [] (juce::AudioProcessorValueTreeState&, const juce::String&)   { return makeInfoComponent ("Passthrough (transparent)"); } },

        { BlockResonator::typeName,
          []                                                              { return std::unique_ptr<Resonator> (new BlockResonator()); },
          [] (juce::AudioProcessorValueTreeState&, const juce::String&)   { return makeInfoComponent ("Block (silent)"); } },

        { WaveguideResonator::typeName,
          []                                                              { return std::unique_ptr<Resonator> (new WaveguideResonator()); },
          [] (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
                                                                          { return std::unique_ptr<juce::Component> (new WaveguideComponent (apvts, prefix)); } },

        { PlateResonator::typeName,
          []                                                              { return std::unique_ptr<Resonator> (new PlateResonator()); },
          [] (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
                                                                          { return std::unique_ptr<juce::Component> (new ModalResonatorComponent (apvts, prefix)); } },

        { MembraneResonator::typeName,
          []                                                              { return std::unique_ptr<Resonator> (new MembraneResonator()); },
          [] (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
                                                                          { return std::unique_ptr<juce::Component> (new ModalResonatorComponent (apvts, prefix)); } }
    };
    return registry;
}

juce::StringArray ResonatorFactory::typeChoices()
{
    juce::StringArray choices;
    for (const auto& info : types())
        choices.add (info.name);
    return choices;
}

int ResonatorFactory::indexOf (const juce::String& name)
{
    const auto& t = types();
    for (int i = 0; i < (int) t.size(); ++i)
        if (t[(size_t) i].name == name)
            return i;
    return 0;
}

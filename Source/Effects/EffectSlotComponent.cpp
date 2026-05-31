/*
  ==============================================================================

    EffectSlotComponent.cpp

  ==============================================================================
*/

#include "EffectSlotComponent.h"
#include "EffectFactory.h"
#include "EffectSlot.h"

EffectSlotComponent::EffectSlotComponent (juce::AudioProcessorValueTreeState& state, const juce::String& pfx)
    : apvts (state), slotPrefix (pfx)
{
    typeBox.addItemList (EffectFactory::typeChoices(), 1);
    typeBox.setLookAndFeel (&fxmeLookAndFeel);
    addAndMakeVisible (typeBox);
    typeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, EffectSlot::typeParamId (slotPrefix), typeBox);

    // GUI-only effect instances back the components (sliders bind to the APVTS,
    // so the audio-thread instances in the processor stay untouched by the GUI).
    for (const auto& info : EffectFactory::types())
    {
        guiEffects.push_back (info.create());
        auto comp = info.createComponent (*guiEffects.back(), apvts,
                                          EffectSlot::perTypePrefix (slotPrefix, info.name));
        addChildComponent (*comp);
        typeComponents.push_back (std::move (comp));
    }

    typeBox.onChange = [this] { updateActiveComponent(); resized(); };
    updateActiveComponent();
}

EffectSlotComponent::~EffectSlotComponent()
{
    typeBox.setLookAndFeel (nullptr);
}

void EffectSlotComponent::updateActiveComponent()
{
    const int active = typeBox.getSelectedItemIndex() - 1;   // 0 == Off
    for (int i = 0; i < (int) typeComponents.size(); ++i)
        typeComponents[(size_t) i]->setVisible (i == active);
}

void EffectSlotComponent::paint (juce::Graphics& g)
{
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (2.0f), 4.0f);
    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (2.0f), 4.0f, 1.0f);
}

void EffectSlotComponent::resized()
{
    auto area = getLocalBounds().reduced (4);
    typeBox.setBounds (area.removeFromTop (24));
    for (auto& comp : typeComponents)
        comp->setBounds (area);
}

/*
  ==============================================================================

    SourceSlotComponent.cpp

  ==============================================================================
*/

#include "SourceSlotComponent.h"
#include "SourceFactory.h"
#include "SourceSlot.h"
#include "WavetableOscSourceComponent.h"

SourceSlotComponent::SourceSlotComponent (juce::AudioProcessorValueTreeState& state, const juce::String& pfx)
    : apvts (state), slotPrefix (pfx)
{
    slotLabel.setText (slotPrefix, juce::dontSendNotification);
    slotLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (slotLabel);

    typeBox.addItemList (SourceFactory::typeChoices(), 1);
    typeBox.setLookAndFeel (&fxmeLookAndFeel);
    addAndMakeVisible (typeBox);
    typeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, SourceSlot::typeParamId (slotPrefix), typeBox);

    for (const auto& info : SourceFactory::types())
    {
        auto comp = info.createComponent (apvts, SourceSlot::perTypePrefix (slotPrefix, info.name));
        addChildComponent (*comp);
        typeComponents.push_back (std::move (comp));
    }

    typeBox.onChange = [this] { updateActiveComponent(); resized(); };
    updateActiveComponent();
}

SourceSlotComponent::~SourceSlotComponent()
{
    typeBox.setLookAndFeel (nullptr);
}

void SourceSlotComponent::updateActiveComponent()
{
    const int active = typeBox.getSelectedItemIndex() - 1;   // 0 == Off
    for (int i = 0; i < (int) typeComponents.size(); ++i)
        typeComponents[(size_t) i]->setVisible (i == active);
}

void SourceSlotComponent::paint (juce::Graphics& g)
{
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (2.0f), 4.0f);
    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (2.0f), 4.0f, 1.0f);
}

void SourceSlotComponent::resized()
{
    auto full = getLocalBounds().reduced (6);
    auto area = full;

    auto header = area.removeFromLeft (headerWidth);
    slotLabel.setBounds (header.removeFromTop (labelHeight));
    typeBox.setBounds (header.removeFromTop (typeBoxHeight));

    // Most type components live to the right of the header column. The
    // wavetable source stacks its Wave/Mode selectors under the type
    // selector, so it gets the full slot and reserves the header corner.
    for (auto& comp : typeComponents)
    {
        if (dynamic_cast<WavetableOscSourceComponent*> (comp.get()) != nullptr)
            comp->setBounds (full);
        else
            comp->setBounds (area);
    }
}

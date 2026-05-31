/*
  ==============================================================================

    EffectChainComponent.cpp

  ==============================================================================
*/

#include "EffectChainComponent.h"
#include "EffectChain.h"

EffectChainComponent::EffectChainComponent (juce::AudioProcessorValueTreeState& apvts,
                                            const juce::String& chainPrefix, const juce::String& title)
{
    titleLabel.setText (title, juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    for (int i = 0; i < EffectChain::numSlots; ++i)
    {
        auto slot = std::make_unique<EffectSlotComponent> (apvts, EffectChain::slotPrefix (chainPrefix, i));
        addAndMakeVisible (*slot);
        slots.push_back (std::move (slot));
    }
}

void EffectChainComponent::resized()
{
    auto area = getLocalBounds();
    titleLabel.setBounds (area.removeFromTop (18));

    const int n = (int) slots.size();
    const int w = juce::jmax (1, area.getWidth() / juce::jmax (1, n));
    for (auto& slot : slots)
        slot->setBounds (area.removeFromLeft (w).reduced (2));
}

/*
  ==============================================================================

    CracksSourceComponent.cpp

  ==============================================================================
*/

#include "CracksSourceComponent.h"
#include "Source.h"

CracksSourceComponent::CracksSourceComponent (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
    : common (apvts, prefix)
{
    densityLabel.setText ("Density", juce::dontSendNotification);
    densityLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (densityLabel);

    densitySlider = std::make_unique<fxme::FxmeSlider> (apvts, Source::makeId (prefix, "density"), "Density", juce::Colours::orange);
    densitySlider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    densitySlider->setLookAndFeel (&fxmeLookAndFeel);
    addAndMakeVisible (*densitySlider);

    addAndMakeVisible (common);
}

CracksSourceComponent::~CracksSourceComponent()
{
    if (densitySlider != nullptr)
        densitySlider->setLookAndFeel (nullptr);
}

void CracksSourceComponent::resized()
{
    auto area = getLocalBounds();
    auto left = area.removeFromLeft (70);
    densityLabel.setBounds (left.removeFromBottom (14));
    densitySlider->setBounds (left.reduced (2));

    common.setBounds (area);
}

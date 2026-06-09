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
    densitySlider = std::make_unique<fxme::FxmeSlider> (apvts, Source::makeId (prefix, "density"), "Density", juce::Colours::orange);
    densitySlider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    densitySlider->setShowLabel (true);     // name drawn below the knob by FxmeLookAndFeel
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
    // Density takes one knob-width so it matches the common knobs to its right
    // (Density + the N common knobs share the row equally).
    auto left = area.removeFromLeft (area.getWidth() / (SourceCommonComponent::numKnobs + 1));
    densitySlider->setBounds (left.reduced (2));   // showLabel draws "Density" below the knob

    common.setBounds (area);
}

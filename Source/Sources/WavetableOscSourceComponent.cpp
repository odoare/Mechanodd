/*
  ==============================================================================

    WavetableOscSourceComponent.cpp

  ==============================================================================
*/

#include "WavetableOscSourceComponent.h"
#include "Source.h"
#include "WavetableLibrary.h"

WavetableOscSourceComponent::WavetableOscSourceComponent (juce::AudioProcessorValueTreeState& state, const juce::String& pfx)
    : apvts (state), prefix (pfx), common (state, pfx)
{
    waveLabel.setText ("Wave", juce::dontSendNotification);
    addAndMakeVisible (waveLabel);
    waveBox.addItemList (WavetableLibrary::names(), 1);
    waveBox.setLookAndFeel (&fxmeLookAndFeel);
    addAndMakeVisible (waveBox);
    waveAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, Source::makeId (prefix, "wave"), waveBox);

    modeLabel.setText ("Mode", juce::dontSendNotification);
    addAndMakeVisible (modeLabel);
    modeBox.addItemList (juce::StringArray { "One-shot", "Loop" }, 1);
    modeBox.setLookAndFeel (&fxmeLookAndFeel);
    addAndMakeVisible (modeBox);
    modeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, Source::makeId (prefix, "mode"), modeBox);

    tuneLabel.setText ("Tune", juce::dontSendNotification);
    tuneLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (tuneLabel);
    tuneSlider = std::make_unique<fxme::FxmeSlider> (apvts, Source::makeId (prefix, "tune"), "Tune", juce::Colours::orange);
    tuneSlider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    tuneSlider->setLookAndFeel (&fxmeLookAndFeel);
    addAndMakeVisible (*tuneSlider);

    addAndMakeVisible (common);
}

WavetableOscSourceComponent::~WavetableOscSourceComponent()
{
    waveBox.setLookAndFeel (nullptr);
    modeBox.setLookAndFeel (nullptr);
    if (tuneSlider != nullptr)
        tuneSlider->setLookAndFeel (nullptr);
}

void WavetableOscSourceComponent::resized()
{
    auto area = getLocalBounds();
    auto left = area.removeFromLeft (170);

    auto waveRow = left.removeFromTop (24);
    waveLabel.setBounds (waveRow.removeFromLeft (44));
    waveBox.setBounds (waveRow);

    auto modeRow = left.removeFromTop (24);
    modeLabel.setBounds (modeRow.removeFromLeft (44));
    modeBox.setBounds (modeRow);

    tuneLabel.setBounds (left.removeFromBottom (14));
    tuneSlider->setBounds (left.reduced (2));

    common.setBounds (area);
}

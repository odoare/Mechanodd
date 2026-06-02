/*
  ==============================================================================

    WavetableOscSourceComponent.cpp

  ==============================================================================
*/

#include "WavetableOscSourceComponent.h"
#include "Source.h"
#include "WavetableLibrary.h"
#include "SourceSlotComponent.h"

WavetableOscSourceComponent::WavetableOscSourceComponent (juce::AudioProcessorValueTreeState& state, const juce::String& pfx)
    : apvts (state), prefix (pfx), common (state, pfx)
{
    // This component spans the whole slot (so Wave/Mode can sit under the type
    // selector), but the reserved header corner overlaps the slot's type box.
    // Let clicks there fall through to it; child controls still receive theirs.
    setInterceptsMouseClicks (false, true);

    waveLabel.setText ("Wave", juce::dontSendNotification);
    waveLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (waveLabel);
    waveBox.addItemList (WavetableLibrary::names(), 1);
    waveBox.setLookAndFeel (&fxmeLookAndFeel);
    addAndMakeVisible (waveBox);
    waveAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, Source::makeId (prefix, "wave"), waveBox);

    modeLabel.setText ("Mode", juce::dontSendNotification);
    modeLabel.setJustificationType (juce::Justification::centredLeft);
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

    // The Wave/Mode selectors stack in the slot's left column, directly under
    // the source-type selector. Reserve the slot header corner so they line up.
    auto left = area.removeFromLeft (SourceSlotComponent::headerWidth);
    left.removeFromTop (SourceSlotComponent::headerHeight);

    auto waveCell = left.removeFromTop (left.getHeight() / 2);
    waveLabel.setBounds (waveCell.removeFromTop (14));
    waveBox.setBounds (waveCell.removeFromTop (24));

    modeLabel.setBounds (left.removeFromTop (14));
    modeBox.setBounds (left.removeFromTop (24));

    // Tune sits at the head of the controls to the right of the column.
    auto tuneCol = area.removeFromLeft (56);
    tuneLabel.setBounds (tuneCol.removeFromBottom (14));
    tuneSlider->setBounds (tuneCol.reduced (2));

    common.setBounds (area);
}

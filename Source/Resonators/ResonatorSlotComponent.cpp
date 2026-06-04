/*
  ==============================================================================

    ResonatorSlotComponent.cpp

  ==============================================================================
*/

#include "ResonatorSlotComponent.h"
#include "ResonatorFactory.h"
#include "ResonatorSlot.h"

ResonatorSlotComponent::ResonatorSlotComponent (juce::AudioProcessorValueTreeState& state, const juce::String& pfx)
    : apvts (state), slotPrefix (pfx)
{
    slotLabel.setText (slotPrefix, juce::dontSendNotification);
    slotLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (slotLabel);

    typeBox.addItemList (ResonatorFactory::typeChoices(), 1);
    typeBox.setLookAndFeel (&fxmeLookAndFeel);
    addAndMakeVisible (typeBox);
    typeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, ResonatorSlot::typeParamId (slotPrefix), typeBox);

    globalButton.setLookAndFeel (&fxmeLookAndFeel);
    addAndMakeVisible (globalButton);
    globalAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, ResonatorSlot::globalParamId (slotPrefix), globalButton);

    freqLabel.setText ("Base Hz", juce::dontSendNotification);
    freqLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (freqLabel);

    freqSlider = std::make_unique<fxme::FxmeSlider> (
        apvts, ResonatorSlot::globalFreqParamId (slotPrefix), "Base Hz", juce::Colours::orange);
    freqSlider->setSliderStyle (juce::Slider::LinearHorizontal);
    freqSlider->setLookAndFeel (&fxmeLookAndFeel);
    addAndMakeVisible (*freqSlider);

    levelLabel.setText ("Level", juce::dontSendNotification);
    levelLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (levelLabel);

    levelSlider = std::make_unique<fxme::FxmeSlider> (
        apvts, ResonatorSlot::levelParamId (slotPrefix), "Level", juce::Colours::orange);
    levelSlider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    levelSlider->setLookAndFeel (&fxmeLookAndFeel);
    addAndMakeVisible (*levelSlider);

    levelMeter.setRange (ResonatorSlot::levelMinDb, ResonatorSlot::levelMaxDb);
    levelMeter.stopTimer();        // driven by this component's timer instead
    addAndMakeVisible (levelMeter);

    startTimerHz (30);

    for (const auto& info : ResonatorFactory::types())
    {
        auto comp = info.createComponent (apvts, ResonatorSlot::perTypePrefix (slotPrefix, info.name));
        addChildComponent (*comp);
        typeComponents.push_back (std::move (comp));
    }

    typeBox.onChange = [this] { updateActiveComponent(); resized(); };
    updateActiveComponent();
}

ResonatorSlotComponent::~ResonatorSlotComponent()
{
    typeBox.setLookAndFeel (nullptr);
    globalButton.setLookAndFeel (nullptr);
    if (freqSlider != nullptr)
        freqSlider->setLookAndFeel (nullptr);
    if (levelSlider != nullptr)
        levelSlider->setLookAndFeel (nullptr);
}

void ResonatorSlotComponent::timerCallback()
{
    if (! levelProvider)
        return;

    // setValue stores; the meter's own timer is stopped, so repaint here (as the
    // matrix does) to actually refresh the bar.
    levelMeter.setValue (juce::Decibels::gainToDecibels (levelProvider(), ResonatorSlot::levelMinDb));
    levelMeter.repaint();
}

void ResonatorSlotComponent::updateActiveComponent()
{
    const int active = typeBox.getSelectedItemIndex();
    for (int i = 0; i < (int) typeComponents.size(); ++i)
        typeComponents[(size_t) i]->setVisible (i == active);
}

void ResonatorSlotComponent::paint (juce::Graphics& g)
{
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (2.0f), 4.0f);
    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (2.0f), 4.0f, 1.0f);
}

void ResonatorSlotComponent::resized()
{
    auto area = getLocalBounds().reduced (6);

    auto header = area.removeFromLeft (96);
    slotLabel.setBounds (header.removeFromTop (18));
    typeBox.setBounds (header.removeFromTop (24));
    globalButton.setBounds (header.removeFromTop (22));
    freqLabel.setBounds (header.removeFromTop (14));
    freqSlider->setBounds (header.removeFromTop (24).reduced (2));

    // Right side: a narrow vertical output meter, then the Level knob. Size the
    // knob column to the knob height so it renders as a full circle the same size
    // as the type's own (height-limited) knobs, with its label below.
    constexpr int labelH = 14;
    levelMeter.setBounds (area.removeFromRight (10).reduced (1, 2));

    const int knobSide = juce::jmax (1, area.getHeight() - labelH);
    auto levelCol = area.removeFromRight (knobSide + 4);
    levelLabel.setBounds (levelCol.removeFromBottom (labelH));
    levelSlider->setBounds (levelCol.reduced (2));

    for (auto& comp : typeComponents)
        comp->setBounds (area);
}

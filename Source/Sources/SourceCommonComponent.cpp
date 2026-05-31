/*
  ==============================================================================

    SourceCommonComponent.cpp

  ==============================================================================
*/

#include "SourceCommonComponent.h"
#include "Source.h"

SourceCommonComponent::SourceCommonComponent (juce::AudioProcessorValueTreeState& state, const juce::String& prefix)
    : apvts (state)
{
    addKnob (attack,    Source::makeId (prefix, "attack"),    "A");
    addKnob (decay,     Source::makeId (prefix, "decay"),     "D");
    addKnob (sustain,   Source::makeId (prefix, "sustain"),   "S");
    addKnob (release,   Source::makeId (prefix, "release"),   "R");
    addKnob (cutoff,    Source::makeId (prefix, "cutoff"),    "Cutoff");
    addKnob (resonance, Source::makeId (prefix, "resonance"), "Reso");
    addKnob (level,     Source::makeId (prefix, "level"),     "Level");
    addKnob (velLevel,  Source::makeId (prefix, "velLevel"),  "Vel>Lvl");
    addKnob (velCutoff, Source::makeId (prefix, "velCutoff"), "Vel>Cut");
}

SourceCommonComponent::~SourceCommonComponent()
{
    for (auto* k : knobs)
        if (k->slider != nullptr)
            k->slider->setLookAndFeel (nullptr);
}

void SourceCommonComponent::addKnob (Knob& k, const juce::String& paramId, const juce::String& text)
{
    k.slider = std::make_unique<fxme::FxmeSlider> (apvts, paramId, text, juce::Colours::orange);
    k.slider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider->setLookAndFeel (&fxmeLookAndFeel);
    addAndMakeVisible (*k.slider);

    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (k.label);
}

void SourceCommonComponent::resized()
{
    auto area = getLocalBounds();
    const int n = (int) knobs.size();
    const int w = juce::jmax (1, area.getWidth() / n);
    const int labelH = 14;

    for (int i = 0; i < n; ++i)
    {
        auto col = area.removeFromLeft (w);
        knobs[(size_t) i]->label.setBounds (col.removeFromBottom (labelH));
        knobs[(size_t) i]->slider->setBounds (col.reduced (2));
    }
}

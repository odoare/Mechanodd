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
    k.slider->setShowLabel (true);          // name drawn below the knob by FxmeLookAndFeel
    k.slider->setLookAndFeel (&fxmeLookAndFeel);
    addAndMakeVisible (*k.slider);
}

void SourceCommonComponent::resized()
{
    auto area = getLocalBounds();
    const int n = (int) knobs.size();
    const int w = juce::jmax (1, area.getWidth() / n);

    for (int i = 0; i < n; ++i)
    {
        auto col = area.removeFromLeft (w);
        // Name drawn below the knob by FxmeLookAndFeel (showLabel): slider takes the column.
        knobs[(size_t) i]->slider->setBounds (col.reduced (2));
    }
}

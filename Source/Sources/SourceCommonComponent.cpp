/*
  ==============================================================================

    SourceCommonComponent.cpp

  ==============================================================================
*/

#include "SourceCommonComponent.h"
#include "Source.h"
#include "../Tooltips.h"

namespace tips = mechanodd::tips::source;

SourceCommonComponent::SourceCommonComponent (juce::AudioProcessorValueTreeState& state, const juce::String& prefix)
    : apvts (state)
{
    addKnob (attack,    Source::makeId (prefix, "attack"),    "A", tips::attack);
    addKnob (decay,     Source::makeId (prefix, "decay"),     "D", tips::decay);
    addKnob (sustain,   Source::makeId (prefix, "sustain"),   "S", tips::sustain);
    addKnob (release,   Source::makeId (prefix, "release"),   "R", tips::release);
    addKnob (cutoff,    Source::makeId (prefix, "cutoff"),    "Cutoff", tips::cutoff);
    addKnob (resonance, Source::makeId (prefix, "resonance"), "Reso", tips::resonance);
    addKnob (level,     Source::makeId (prefix, "level"),     "Level", tips::level);
    addKnob (velLevel,  Source::makeId (prefix, "velLevel"),  "Vel>Lvl", tips::velLevel);
    addKnob (velCutoff, Source::makeId (prefix, "velCutoff"), "Vel>Cut", tips::velCutoff);
}

SourceCommonComponent::~SourceCommonComponent()
{
    for (auto* k : knobs)
        if (k->slider != nullptr)
            k->slider->setLookAndFeel (nullptr);
}

void SourceCommonComponent::addKnob (Knob& k, const juce::String& paramId, const juce::String& text, const char* tooltip)
{
    k.slider = std::make_unique<fxme::FxmeSlider> (apvts, paramId, text, juce::Colours::orange);
    k.slider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider->setShowLabel (true);          // name drawn below the knob by FxmeLookAndFeel
    k.slider->setLookAndFeel (&fxmeLookAndFeel);
    k.slider->setTooltip (tooltip);
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

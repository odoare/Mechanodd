/*
  ==============================================================================

    BeamStringComponent.cpp

  ==============================================================================
*/

#include "BeamStringComponent.h"
#include "Resonator.h"
#include "../Tooltips.h"

namespace tips = mechanodd::tips::resonator;

BeamStringComponent::BeamStringComponent (juce::AudioProcessorValueTreeState& state, const juce::String& pfx)
    : apvts (state), prefix (pfx)
{
    addKnob (coarse,      Resonator::makeId (prefix, "coarse"),      "Coarse", tips::coarse, true);
    addKnob (fine,        Resonator::makeId (prefix, "fine"),        "Fine",   tips::fine, true);
    addKnob (mix,         Resonator::makeId (prefix, "mix"),         "Str/Beam", tips::mix);
    addKnob (modes,       Resonator::makeId (prefix, "modes"),       "Modes", tips::modes);
    addKnob (resOn,       Resonator::makeId (prefix, "resOn"),       "Res On", tips::resOn);
    addKnob (resOff,      Resonator::makeId (prefix, "resOff"),      "Res Off", tips::resOff);
    addKnob (resSlopeOn,  Resonator::makeId (prefix, "resSlopeOn"),  "RSlope On", tips::resSlopeOn);
    addKnob (resSlopeOff, Resonator::makeId (prefix, "resSlopeOff"), "RSlope Off", tips::resSlopeOff);
    addKnob (inPos,       Resonator::makeId (prefix, "inPos"),       "In Pos", tips::inPos);
    addKnob (outPos,      Resonator::makeId (prefix, "outPos"),      "Out Pos", tips::outPos);
    addKnob (noteRel,     Resonator::makeId (prefix, "noteRel"),     "N.Rel", tips::noteRel);
}

BeamStringComponent::~BeamStringComponent()
{
    for (auto* k : knobs)
        if (k->slider != nullptr)
            k->slider->setLookAndFeel (nullptr);
}

void BeamStringComponent::addKnob (Knob& k, const juce::String& paramId, const juce::String& text, const char* tooltip, bool bipolar)
{
    k.slider = std::make_unique<fxme::FxmeSlider> (apvts, paramId, text, juce::Colours::orange);
    k.slider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    if (bipolar)
        k.slider->setCentralValue (0.0);   // pitch offset: 0 semitones at centre
    k.slider->setShowLabel (true);          // name drawn below the knob by FxmeLookAndFeel
    k.slider->setLookAndFeel (&fxmeLookAndFeel);
    k.slider->setTooltip (tooltip);
    addAndMakeVisible (*k.slider);
}

void BeamStringComponent::resized()
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

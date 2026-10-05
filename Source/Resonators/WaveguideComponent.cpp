/*
  ==============================================================================

    WaveguideComponent.cpp

  ==============================================================================
*/

#include "WaveguideComponent.h"
#include "Resonator.h"
#include "../Tooltips.h"

namespace tips = mechanodd::tips::resonator;

WaveguideComponent::WaveguideComponent (juce::AudioProcessorValueTreeState& state, const juce::String& pfx)
    : apvts (state), prefix (pfx)
{
    addKnob (coarse,     Resonator::makeId (prefix, "coarse"),      "Coarse", tips::coarse, true);
    addKnob (fine,       Resonator::makeId (prefix, "fine"),        "Fine",   tips::fine, true);
    addKnob (fbOn,       Resonator::makeId (prefix, "fbGainOn"),    "Fb On", tips::fbOn);
    addKnob (fbOff,      Resonator::makeId (prefix, "fbGainOff"),   "Fb Off", tips::fbOff);
    addKnob (cutOn,      Resonator::makeId (prefix, "fbCutoffOn"),  "Cut On", tips::cutOn);
    addKnob (cutOff,     Resonator::makeId (prefix, "fbCutoffOff"), "Cut Off", tips::cutOff);
    addKnob (inPos,      Resonator::makeId (prefix, "inPos"),       "In Pos", tips::inPos);
    addKnob (outPos,     Resonator::makeId (prefix, "outPos"),      "Out Pos", tips::outPos);
    addKnob (dispersion, Resonator::makeId (prefix, "dispersion"),  "Disp", tips::dispersion);
    addKnob (noteRel,    Resonator::makeId (prefix, "noteRel"),     "N.Rel", tips::noteRel);
}

WaveguideComponent::~WaveguideComponent()
{
    for (auto* k : knobs)
        if (k->slider != nullptr)
            k->slider->setLookAndFeel (nullptr);
}

void WaveguideComponent::addKnob (Knob& k, const juce::String& paramId, const juce::String& text, const char* tooltip, bool bipolar)
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

void WaveguideComponent::resized()
{
    auto area = getLocalBounds();
    const int n = (int) knobs.size();
    const int w = juce::jmax (1, area.getWidth() / n);

    for (int i = 0; i < n; ++i)
    {
        auto col = area.removeFromLeft (w);
        // The knob's name is drawn below it by FxmeLookAndFeel (showLabel), so the
        // slider takes the whole column.
        knobs[(size_t) i]->slider->setBounds (col.reduced (2));
    }
}

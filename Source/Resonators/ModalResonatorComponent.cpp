/*
  ==============================================================================

    ModalResonatorComponent.cpp

  ==============================================================================
*/

#include "ModalResonatorComponent.h"
#include "Resonator.h"
#include "../Tooltips.h"

namespace tips = mechanodd::tips::resonator;

ModalResonatorComponent::ModalResonatorComponent (juce::AudioProcessorValueTreeState& state, const juce::String& pfx)
    : apvts (state), prefix (pfx)
{
    addKnob (coarse,   Resonator::makeId (prefix, "coarse"),   "Coarse", tips::coarse, true);
    addKnob (fine,     Resonator::makeId (prefix, "fine"),     "Fine",   tips::fine, true);
    addKnob (aspect,   Resonator::makeId (prefix, "aspect"),   "Aspect", tips::aspect);
    addKnob (modes,    Resonator::makeId (prefix, "modes"),    "Modes", tips::modes);
    addKnob (resOn,       Resonator::makeId (prefix, "resOn"),       "Res On", tips::resOn);
    addKnob (resOff,      Resonator::makeId (prefix, "resOff"),      "Res Off", tips::resOff);
    addKnob (resSlopeOn,  Resonator::makeId (prefix, "resSlopeOn"),  "RSlope On", tips::resSlopeOn);
    addKnob (resSlopeOff, Resonator::makeId (prefix, "resSlopeOff"), "RSlope Off", tips::resSlopeOff);
    addKnob (inX,      Resonator::makeId (prefix, "inX"),      "In X", tips::inX);
    addKnob (inY,      Resonator::makeId (prefix, "inY"),      "In Y", tips::inY);
    addKnob (outX,     Resonator::makeId (prefix, "outX"),     "Out X", tips::outX);
    addKnob (outY,     Resonator::makeId (prefix, "outY"),     "Out Y", tips::outY);
    addKnob (noteRel,  Resonator::makeId (prefix, "noteRel"),  "N.Rel", tips::noteRel);
}

ModalResonatorComponent::~ModalResonatorComponent()
{
    for (auto* k : knobs)
        if (k->slider != nullptr)
            k->slider->setLookAndFeel (nullptr);
}

void ModalResonatorComponent::addKnob (Knob& k, const juce::String& paramId, const juce::String& text, const char* tooltip, bool bipolar)
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

void ModalResonatorComponent::resized()
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

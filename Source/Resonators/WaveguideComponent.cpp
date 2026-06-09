/*
  ==============================================================================

    WaveguideComponent.cpp

  ==============================================================================
*/

#include "WaveguideComponent.h"
#include "Resonator.h"

WaveguideComponent::WaveguideComponent (juce::AudioProcessorValueTreeState& state, const juce::String& pfx)
    : apvts (state), prefix (pfx)
{
    addKnob (coarse,     Resonator::makeId (prefix, "coarse"),      "Coarse", true);
    addKnob (fine,       Resonator::makeId (prefix, "fine"),        "Fine",   true);
    addKnob (fbOn,       Resonator::makeId (prefix, "fbGainOn"),    "Fb On");
    addKnob (fbOff,      Resonator::makeId (prefix, "fbGainOff"),   "Fb Off");
    addKnob (cutOn,      Resonator::makeId (prefix, "fbCutoffOn"),  "Cut On");
    addKnob (cutOff,     Resonator::makeId (prefix, "fbCutoffOff"), "Cut Off");
    addKnob (inPos,      Resonator::makeId (prefix, "inPos"),       "In Pos");
    addKnob (outPos,     Resonator::makeId (prefix, "outPos"),      "Out Pos");
    addKnob (dispersion, Resonator::makeId (prefix, "dispersion"),  "Disp");
    addKnob (noteRel,    Resonator::makeId (prefix, "noteRel"),     "N.Rel");
}

WaveguideComponent::~WaveguideComponent()
{
    for (auto* k : knobs)
        if (k->slider != nullptr)
            k->slider->setLookAndFeel (nullptr);
}

void WaveguideComponent::addKnob (Knob& k, const juce::String& paramId, const juce::String& text, bool bipolar)
{
    k.slider = std::make_unique<fxme::FxmeSlider> (apvts, paramId, text, juce::Colours::orange);
    k.slider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    if (bipolar)
        k.slider->setCentralValue (0.0);   // pitch offset: 0 semitones at centre
    k.slider->setShowLabel (true);          // name drawn below the knob by FxmeLookAndFeel
    k.slider->setLookAndFeel (&fxmeLookAndFeel);
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

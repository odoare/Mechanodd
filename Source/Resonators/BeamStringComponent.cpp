/*
  ==============================================================================

    BeamStringComponent.cpp

  ==============================================================================
*/

#include "BeamStringComponent.h"
#include "Resonator.h"

BeamStringComponent::BeamStringComponent (juce::AudioProcessorValueTreeState& state, const juce::String& pfx)
    : apvts (state), prefix (pfx)
{
    addKnob (coarse,      Resonator::makeId (prefix, "coarse"),      "Coarse");
    addKnob (fine,        Resonator::makeId (prefix, "fine"),        "Fine");
    addKnob (mix,         Resonator::makeId (prefix, "mix"),         "Str/Beam");
    addKnob (modes,       Resonator::makeId (prefix, "modes"),       "Modes");
    addKnob (resOn,       Resonator::makeId (prefix, "resOn"),       "Res On");
    addKnob (resOff,      Resonator::makeId (prefix, "resOff"),      "Res Off");
    addKnob (resSlopeOn,  Resonator::makeId (prefix, "resSlopeOn"),  "RSlope On");
    addKnob (resSlopeOff, Resonator::makeId (prefix, "resSlopeOff"), "RSlope Off");
    addKnob (inPos,       Resonator::makeId (prefix, "inPos"),       "In Pos");
    addKnob (outPos,      Resonator::makeId (prefix, "outPos"),      "Out Pos");
    addKnob (noteRel,     Resonator::makeId (prefix, "noteRel"),     "N.Rel");
}

BeamStringComponent::~BeamStringComponent()
{
    for (auto* k : knobs)
        if (k->slider != nullptr)
            k->slider->setLookAndFeel (nullptr);
}

void BeamStringComponent::addKnob (Knob& k, const juce::String& paramId, const juce::String& text)
{
    k.slider = std::make_unique<fxme::FxmeSlider> (apvts, paramId, text, juce::Colours::orange);
    k.slider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider->setLookAndFeel (&fxmeLookAndFeel);
    addAndMakeVisible (*k.slider);

    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (k.label);
}

void BeamStringComponent::resized()
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

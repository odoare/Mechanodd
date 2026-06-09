/*
  ==============================================================================

    WaveguideComponent.h

    GUI for a waveguide resonator. Controls bind to the APVTS by prefixed id.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class WaveguideComponent : public juce::Component
{
public:
    WaveguideComponent (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    ~WaveguideComponent() override;

    void resized() override;

private:
    struct Knob
    {
        std::unique_ptr<fxme::FxmeSlider> slider;
    };

    void addKnob (Knob& k, const juce::String& paramId, const juce::String& text, bool bipolar = false);

    juce::AudioProcessorValueTreeState& apvts;
    juce::String prefix;

    Knob coarse, fine, fbOn, fbOff, cutOn, cutOff, inPos, outPos, dispersion, noteRel;
    std::array<Knob*, 10> knobs {
        &coarse, &fine, &fbOn, &fbOff, &cutOn, &cutOff, &inPos, &outPos, &dispersion, &noteRel };

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveguideComponent)
};

/*
  ==============================================================================

    BeamStringComponent.h

    GUI for the pinned-pinned beam/string resonator: the shared modal knobs
    (resonance bank) with a single Str/Beam morph in place of the aspect ratio
    and a single in/out position pair in place of the 2D taps.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class BeamStringComponent : public juce::Component
{
public:
    BeamStringComponent (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    ~BeamStringComponent() override;

    void resized() override;

private:
    struct Knob
    {
        std::unique_ptr<fxme::FxmeSlider> slider;
    };

    void addKnob (Knob& k, const juce::String& paramId, const juce::String& text, bool bipolar = false);

    juce::AudioProcessorValueTreeState& apvts;
    juce::String prefix;

    Knob coarse, fine, mix, modes, resOn, resOff, resSlopeOn, resSlopeOff, inPos, outPos, noteRel;
    std::array<Knob*, 11> knobs {
        &coarse, &fine, &mix, &modes, &resOn, &resOff, &resSlopeOn, &resSlopeOff,
        &inPos, &outPos, &noteRel };

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BeamStringComponent)
};

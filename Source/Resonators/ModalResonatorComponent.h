/*
  ==============================================================================

    ModalResonatorComponent.h

    GUI shared by plate and membrane resonators (identical parameter set): the
    modal knobs, all bound to the APVTS by prefixed id. The geometry/shape is now
    chosen by the resonator type selector, so there is no per-resonator shape box.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class ModalResonatorComponent : public juce::Component
{
public:
    ModalResonatorComponent (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    ~ModalResonatorComponent() override;

    void resized() override;

private:
    struct Knob
    {
        std::unique_ptr<fxme::FxmeSlider> slider;
    };

    void addKnob (Knob& k, const juce::String& paramId, const juce::String& text, bool bipolar = false);

    juce::AudioProcessorValueTreeState& apvts;
    juce::String prefix;

    Knob coarse, fine, aspect, modes, resOn, resOff, resSlopeOn, resSlopeOff, inX, inY, outX, outY, noteRel;
    std::array<Knob*, 13> knobs {
        &coarse, &fine, &aspect, &modes, &resOn, &resOff, &resSlopeOn, &resSlopeOff,
        &inX, &inY, &outX, &outY, &noteRel };

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModalResonatorComponent)
};

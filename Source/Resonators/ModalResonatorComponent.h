/*
  ==============================================================================

    ModalResonatorComponent.h

    GUI shared by plate and membrane resonators (identical parameter set). A
    shape selector plus the modal knobs; all controls bind to the APVTS by
    prefixed id.

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
        juce::Label label;
        std::unique_ptr<fxme::FxmeSlider> slider;
    };

    void addKnob (Knob& k, const juce::String& paramId, const juce::String& text);

    juce::AudioProcessorValueTreeState& apvts;
    juce::String prefix;

    juce::Label shapeLabel;
    juce::ComboBox shapeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAtt;

    Knob coarse, fine, aspect, modes, resOn, resOff, resSlopeOn, resSlopeOff, inX, inY, outX, outY, noteRel;
    std::array<Knob*, 13> knobs {
        &coarse, &fine, &aspect, &modes, &resOn, &resOff, &resSlopeOn, &resSlopeOff,
        &inX, &inY, &outX, &outY, &noteRel };

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModalResonatorComponent)
};

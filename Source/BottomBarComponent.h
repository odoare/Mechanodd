/*
  ==============================================================================

    BottomBarComponent.h

    Global output strip shown under the tabs: an output volume knob, a stereo
    output VU meter (FxmeJuceTools' three-colour HorizontalVuMeter, one per
    channel), a voice-count knob and a portamento knob.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class BottomBarComponent : public juce::Component
{
public:
    explicit BottomBarComponent (MechanoscAudioProcessor& processor);
    ~BottomBarComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Label label;
        std::unique_ptr<fxme::FxmeSlider> slider;
    };

    void addKnob (Knob& k, const juce::String& paramId, const juce::String& text);

    MechanoscAudioProcessor& audioProcessor;

    Knob outputVolume, voices, portamento;

    std::unique_ptr<fxme::HorizontalVuMeter> meterL, meterR;
    juce::Label meterLabelL, meterLabelR;

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BottomBarComponent)
};

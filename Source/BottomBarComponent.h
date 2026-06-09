/*
  ==============================================================================

    BottomBarComponent.h

    Global output strip shown under the tabs: an output volume knob, a stereo
    output VU meter (FxmeJuceTools' three-colour HorizontalVuMeter, one per
    channel), a voice-count knob, a portamento knob, and preset load/save
    buttons that read/write the full APVTS state as XML files.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class BottomBarComponent : public juce::Component
{
public:
    explicit BottomBarComponent (MechanOddAudioProcessor& processor);
    ~BottomBarComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        std::unique_ptr<fxme::FxmeSlider> slider;
    };

    void addKnob (Knob& k, const juce::String& paramId, const juce::String& text);

    MechanOddAudioProcessor& audioProcessor;

    Knob outputVolume, voices, portamento;

    std::unique_ptr<fxme::HorizontalVuMeter> meterL, meterR;
    juce::Label meterLabelL, meterLabelR;

    juce::TextButton loadButton { "L" };
    juce::TextButton saveButton { "S" };
    std::unique_ptr<juce::FileChooser> fileChooser;

    juce::Image logoImage;
    juce::Rectangle<int> logoRect;

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BottomBarComponent)
};

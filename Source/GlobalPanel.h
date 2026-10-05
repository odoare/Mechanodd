/*
  ==============================================================================

    GlobalPanel.h

    The global settings opened by the top bar's gear button, in a callout:
    the voice count and the portamento time. (The output level and meter
    are in the top bar itself.)

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class GlobalPanel : public juce::Component
{
public:
    explicit GlobalPanel (MechanOddAudioProcessor& processor);
    ~GlobalPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int preferredWidth  = 200;
    static constexpr int preferredHeight = 120;

private:
    fxme::FxmeLookAndFeel laf;

    fxme::FxmeSlider voices, portamento;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GlobalPanel)
};

//==============================================================================
/** A toothed wheel with a hole, in the given accent colour (after Dede's). */
class GearButton : public juce::Button
{
public:
    explicit GearButton (juce::Colour accentColour)
        : juce::Button ("global"), accent (accentColour) {}

    void paintButton (juce::Graphics&, bool over, bool down) override;

private:
    juce::Colour accent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GearButton)
};

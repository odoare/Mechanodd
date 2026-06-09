/*
  ==============================================================================

    SourceCommonComponent.h

    The controls shared by every source: ADSR, resonant LPF (cutoff/resonance),
    output level, and the velocity links. Reused by each source-type component.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class SourceCommonComponent : public juce::Component
{
public:
    SourceCommonComponent (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    ~SourceCommonComponent() override;

    void resized() override;

    // Number of equally-spaced knobs laid out across the row. Source-type components
    // that prepend their own knob (Density, Tune) use this so their extra knob ends
    // up the same width as these.
    static constexpr int numKnobs = 9;

private:
    struct Knob
    {
        std::unique_ptr<fxme::FxmeSlider> slider;
    };

    void addKnob (Knob& k, const juce::String& paramId, const juce::String& text);

    juce::AudioProcessorValueTreeState& apvts;

    Knob attack, decay, sustain, release, cutoff, resonance, level, velLevel, velCutoff;
    std::array<Knob*, 9> knobs { &attack, &decay, &sustain, &release, &cutoff, &resonance, &level, &velLevel, &velCutoff };

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SourceCommonComponent)
};

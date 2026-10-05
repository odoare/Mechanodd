/*
  ==============================================================================

    OutputMeter.h

    The stereo output meter in the top bar: FxmeTools' three-colour
    HorizontalVuMeter, one thin bar per channel stacked (left on top). Green
    moving peak, amber peak-hold, red over 0 dBFS; -60 to +6 dB. Reads the
    processor's post-master levels.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class OutputMeter : public juce::Component,
                    public juce::SettableTooltipClient
{
public:
    explicit OutputMeter (MechanOddAudioProcessor& processor)
        : left  (makeMeter (processor, 0)),
          right (makeMeter (processor, 1))
    {
        // The bars let the mouse through, so the tooltip window finds this
        // component (and its tip) under the pointer.
        for (auto* m : { left.get(), right.get() })
        {
            m->setInterceptsMouseClicks (false, false);
            addAndMakeVisible (*m);
        }
    }

    void resized() override
    {
        auto area = getLocalBounds();
        const int half = area.getHeight() / 2;
        left->setBounds (area.removeFromTop (half));
        right->setBounds (area);
    }

private:
    static std::unique_ptr<fxme::HorizontalVuMeter> makeMeter (MechanOddAudioProcessor& p, int ch)
    {
        auto m = std::make_unique<fxme::HorizontalVuMeter> (
            [&p, ch] { return p.getOutputLevelDb (ch); },
            [&p, ch] { return p.getOutputHoldDb  (ch); });
        m->setMaxColour         (juce::Colour (0xff3ad07a));   // green
        m->setSmoothedMaxColour (juce::Colour (0xffd9b13a));   // amber
        m->setOverColour        (juce::Colours::red);
        m->setBackgroundColour  (juce::Colours::black);
        m->minValue = -60.0f;
        m->maxValue =   6.0f;
        return m;
    }

    std::unique_ptr<fxme::HorizontalVuMeter> left, right;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OutputMeter)
};

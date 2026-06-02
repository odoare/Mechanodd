/*
  ==============================================================================

    FeedbackMatrixComponent.h

    Grid GUI for the feedback matrix: one small knob per (resonator-row,
    column) feedback gain, with self-feedback cells greyed out, plus
    level / pan / send knobs per resonator row.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "FeedbackMatrix.h"
#include "VuMeterComponent.h"

class FeedbackMatrixComponent : public juce::Component,
                                private juce::Timer
{
public:
    explicit FeedbackMatrixComponent (juce::AudioProcessorValueTreeState& apvts);
    ~FeedbackMatrixComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Source of the per-column "entering signal" level (linear peak; see
    // MechanoscAudioProcessor::getColumnLevelLinear). The meters scale it by each
    // cell's knob gain to show the post-gain contribution into the row's resonator.
    void setColumnLevelProvider (std::function<float (int)> fn) { columnLevelLinear = std::move (fn); }

private:
    std::unique_ptr<fxme::FxmeSlider> makeKnob (const juce::String& paramId);
    void timerCallback() override;

    juce::AudioProcessorValueTreeState& apvts;

    static constexpr int rows = FeedbackMatrix::numRows;
    static constexpr int cols = FeedbackMatrix::numColumns;

    static constexpr int meterH = 7;   // height of each cell's meter strip

    std::array<std::array<std::unique_ptr<fxme::FxmeSlider>, cols>, rows> gainKnobs;
    std::array<std::array<std::unique_ptr<VuMeterComponent>, cols>, rows> meters;
    std::array<std::unique_ptr<fxme::FxmeSlider>, rows> levelKnobs, panKnobs, sendKnobs;
    std::array<std::unique_ptr<VuMeterComponent>, rows> levelMeters, sendMeters;

    std::function<float (int)> columnLevelLinear;

    juce::StringArray colHeaders;   // cols feedback columns + Level/Pan/Send
    juce::StringArray rowHeaders;

    static constexpr int rowLabelW = 46;
    static constexpr int colLabelH = 18;

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FeedbackMatrixComponent)
};

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

class FeedbackMatrixComponent : public juce::Component
{
public:
    explicit FeedbackMatrixComponent (juce::AudioProcessorValueTreeState& apvts);
    ~FeedbackMatrixComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    std::unique_ptr<fxme::FxmeSlider> makeKnob (const juce::String& paramId);

    juce::AudioProcessorValueTreeState& apvts;

    static constexpr int rows = FeedbackMatrix::numRows;
    static constexpr int cols = FeedbackMatrix::numColumns;

    std::array<std::array<std::unique_ptr<fxme::FxmeSlider>, cols>, rows> gainKnobs;
    std::array<std::unique_ptr<fxme::FxmeSlider>, rows> levelKnobs, panKnobs, sendKnobs;

    juce::StringArray colHeaders;   // cols feedback columns + Level/Pan/Send
    juce::StringArray rowHeaders;

    static constexpr int rowLabelW = 46;
    static constexpr int colLabelH = 18;

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FeedbackMatrixComponent)
};

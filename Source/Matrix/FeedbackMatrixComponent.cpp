/*
  ==============================================================================

    FeedbackMatrixComponent.cpp

  ==============================================================================
*/

#include "FeedbackMatrixComponent.h"

FeedbackMatrixComponent::FeedbackMatrixComponent (juce::AudioProcessorValueTreeState& state)
    : apvts (state)
{
    for (int c = 0; c < FeedbackMatrix::numSources; ++c)
        colHeaders.add ("S" + juce::String (c));
    for (int c = 0; c < FeedbackMatrix::numResonators; ++c)
        colHeaders.add ("R" + juce::String (c));
    colHeaders.add ("Lvl");
    colHeaders.add ("Pan");
    colHeaders.add ("Snd");

    for (int r = 0; r < rows; ++r)
        rowHeaders.add ("R" + juce::String (r));

    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < cols; ++c)
            if (! FeedbackMatrix::isSelfCell (r, c))
                gainKnobs[(size_t) r][(size_t) c] = makeKnob (FeedbackMatrix::gainId (r, c));

        levelKnobs[(size_t) r] = makeKnob (FeedbackMatrix::levelId (r));
        panKnobs[(size_t) r]   = makeKnob (FeedbackMatrix::panId (r));
        sendKnobs[(size_t) r]  = makeKnob (FeedbackMatrix::sendId (r));
    }
}

FeedbackMatrixComponent::~FeedbackMatrixComponent()
{
    auto clear = [] (std::unique_ptr<fxme::FxmeSlider>& s) { if (s != nullptr) s->setLookAndFeel (nullptr); };
    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < cols; ++c)
            clear (gainKnobs[(size_t) r][(size_t) c]);
        clear (levelKnobs[(size_t) r]);
        clear (panKnobs[(size_t) r]);
        clear (sendKnobs[(size_t) r]);
    }
}

std::unique_ptr<fxme::FxmeSlider> FeedbackMatrixComponent::makeKnob (const juce::String& paramId)
{
    auto s = std::make_unique<fxme::FxmeSlider> (apvts, paramId, paramId, juce::Colours::orange);
    s->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s->setLookAndFeel (&fxmeLookAndFeel);
    addAndMakeVisible (*s);
    return s;
}

void FeedbackMatrixComponent::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().reduced (8);
    const int totalCols = cols + 3;
    const int gridW = area.getWidth() - rowLabelW;
    const int gridH = area.getHeight() - colLabelH;
    const int cellW = juce::jmax (1, gridW / totalCols);
    const int cellH = juce::jmax (1, gridH / rows);

    const int gridX = area.getX() + rowLabelW;
    const int gridY = area.getY() + colLabelH;

    g.setColour (juce::Colours::white.withAlpha (0.7f));
    g.setFont (12.0f);

    // Column headers.
    for (int c = 0; c < totalCols; ++c)
        g.drawText (colHeaders[c], gridX + c * cellW, area.getY(), cellW, colLabelH, juce::Justification::centred);

    // Row headers.
    for (int r = 0; r < rows; ++r)
        g.drawText (rowHeaders[r], area.getX(), gridY + r * cellH, rowLabelW, cellH, juce::Justification::centred);

    // Grey the self-feedback cells and draw gridlines.
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
        {
            juce::Rectangle<int> cell (gridX + c * cellW, gridY + r * cellH, cellW, cellH);
            if (FeedbackMatrix::isSelfCell (r, c))
            {
                g.setColour (juce::Colours::black.withAlpha (0.4f));
                g.fillRect (cell.reduced (1));
            }
            g.setColour (juce::Colours::white.withAlpha (0.08f));
            g.drawRect (cell, 1);
        }

    // Separator before the mix columns.
    const int sepX = gridX + cols * cellW;
    g.setColour (juce::Colours::white.withAlpha (0.3f));
    g.drawLine ((float) sepX, (float) area.getY(), (float) sepX, (float) (gridY + rows * cellH), 1.0f);
}

void FeedbackMatrixComponent::resized()
{
    auto area = getLocalBounds().reduced (8);
    const int totalCols = cols + 3;
    const int gridW = area.getWidth() - rowLabelW;
    const int gridH = area.getHeight() - colLabelH;
    const int cellW = juce::jmax (1, gridW / totalCols);
    const int cellH = juce::jmax (1, gridH / rows);

    const int gridX = area.getX() + rowLabelW;
    const int gridY = area.getY() + colLabelH;

    auto place = [&] (fxme::FxmeSlider* s, int r, int c)
    {
        if (s != nullptr)
            s->setBounds (juce::Rectangle<int> (gridX + c * cellW, gridY + r * cellH, cellW, cellH).reduced (3));
    };

    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < cols; ++c)
            place (gainKnobs[(size_t) r][(size_t) c].get(), r, c);

        place (levelKnobs[(size_t) r].get(), r, cols + 0);
        place (panKnobs[(size_t) r].get(),   r, cols + 1);
        place (sendKnobs[(size_t) r].get(),  r, cols + 2);
    }
}

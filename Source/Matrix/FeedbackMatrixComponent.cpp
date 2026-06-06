/*
  ==============================================================================

    FeedbackMatrixComponent.cpp

  ==============================================================================
*/

#include "FeedbackMatrixComponent.h"
#include "../Theme.h"

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

        // Each row feeds resonator r, so tint the whole row in its colour.
        const auto rc = MechanOddTheme::resonatorColour (r);
        for (int c = 0; c < cols; ++c)
            if (gainKnobs[(size_t) r][(size_t) c] != nullptr)
                MechanOddTheme::accentSlider (*gainKnobs[(size_t) r][(size_t) c], rc);
        MechanOddTheme::accentSlider (*levelKnobs[(size_t) r], rc);
        MechanOddTheme::accentSlider (*panKnobs[(size_t) r],   rc);
        MechanOddTheme::accentSlider (*sendKnobs[(size_t) r],  rc);

        auto makeMeter = [this] (juce::Colour colour)
        {
            auto m = std::make_unique<VuMeterComponent>();
            m->setHorizontal (true);
            m->setMeterColor (colour);
            m->setRange (-48.0f, 0.0f);
            m->stopTimer();                 // driven by this component's timer instead
            addAndMakeVisible (*m);
            return m;
        };

        // A horizontal meter above each gain knob, coloured by its column (the
        // entering signal that forces this row's resonator).
        for (int c = 0; c < cols; ++c)
            if (gainKnobs[(size_t) r][(size_t) c] != nullptr)
                meters[(size_t) r][(size_t) c] = makeMeter (
                    (c < FeedbackMatrix::numSources) ? MechanOddTheme::sourceColour (c)
                                                     : MechanOddTheme::resonatorColour (c - FeedbackMatrix::numSources));

        // Level and send meters show this resonator's output (scaled by the knob),
        // coloured by the row.
        levelMeters[(size_t) r] = makeMeter (rc);
        sendMeters[(size_t) r]  = makeMeter (rc);
    }

    startTimerHz (30);
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

    g.setFont (12.0f);

    // Column headers — sources / resonators in their own colours, mix cols neutral.
    for (int c = 0; c < totalCols; ++c)
    {
        juce::Colour hc = juce::Colours::white.withAlpha (0.7f);
        if (c < FeedbackMatrix::numSources)
            hc = MechanOddTheme::sourceColour (c);
        else if (c < cols)
            hc = MechanOddTheme::resonatorColour (c - FeedbackMatrix::numSources);

        g.setColour (hc);
        g.drawText (colHeaders[c], gridX + c * cellW, area.getY(), cellW, colLabelH, juce::Justification::centred);
    }

    // Row headers — each row's destination resonator colour.
    for (int r = 0; r < rows; ++r)
    {
        g.setColour (MechanOddTheme::resonatorColour (r));
        g.drawText (rowHeaders[r], area.getX(), gridY + r * cellH, rowLabelW, cellH, juce::Justification::centred);
    }

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

    // Place a knob in a cell, optionally with a meter strip carved off the top.
    auto place = [&] (fxme::FxmeSlider* s, VuMeterComponent* m, int r, int c)
    {
        auto cell = juce::Rectangle<int> (gridX + c * cellW, gridY + r * cellH, cellW, cellH).reduced (3);
        if (m != nullptr)
            m->setBounds (cell.removeFromTop (meterH));
        if (s != nullptr)
            s->setBounds (cell);
    };

    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < cols; ++c)
            place (gainKnobs[(size_t) r][(size_t) c].get(), meters[(size_t) r][(size_t) c].get(), r, c);

        place (levelKnobs[(size_t) r].get(), levelMeters[(size_t) r].get(), r, cols + 0);
        place (panKnobs[(size_t) r].get(),   nullptr,                       r, cols + 1);
        place (sendKnobs[(size_t) r].get(),  sendMeters[(size_t) r].get(),  r, cols + 2);
    }
}

void FeedbackMatrixComponent::timerCallback()
{
    if (! columnLevelLinear)
        return;

    constexpr float muteFloor = -100.0f;   // below the meter range -> shows empty

    // Show a post-gain level, or empty (no computation) when muted.
    auto show = [muteFloor] (VuMeterComponent* m, bool muted, float linTimesGain)
    {
        if (m == nullptr)
            return;
        m->setValue (muted ? muteFloor : juce::Decibels::gainToDecibels (linTimesGain, muteFloor));
        m->repaint();
    };

    for (int r = 0; r < rows; ++r)
    {
        // Gain cells: column entering signal scaled by the cell's gain.
        for (int c = 0; c < cols; ++c)
        {
            if (meters[(size_t) r][(size_t) c] == nullptr)
                continue;

            const float v     = (float) gainKnobs[(size_t) r][(size_t) c]->getValue();
            const bool  muted = std::abs (v) <= 1.0e-4f;
            const float g     = std::abs (FeedbackMatrix::gainFromParam (v));
            show (meters[(size_t) r][(size_t) c].get(), muted, columnLevelLinear (c) * g);
        }

        // Level / send: this resonator's output (its own column) scaled by the knob.
        const float resLevel = columnLevelLinear (FeedbackMatrix::numSources + r);

        const float lvDb = (float) levelKnobs[(size_t) r]->getValue();
        show (levelMeters[(size_t) r].get(), lvDb <= -59.9f,
              resLevel * juce::Decibels::decibelsToGain (lvDb, -60.0f));

        const float sendAmt = (float) sendKnobs[(size_t) r]->getValue();
        show (sendMeters[(size_t) r].get(), sendAmt <= 1.0e-4f, resLevel * sendAmt);
    }
}

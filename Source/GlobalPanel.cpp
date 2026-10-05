/*
  ==============================================================================

    GlobalPanel.cpp

  ==============================================================================
*/

#include "GlobalPanel.h"
#include "Theme.h"

GlobalPanel::GlobalPanel (MechanOddAudioProcessor& p)
    : voices     (p.apvts, MechanOddAudioProcessor::numVoicesId,  "Voices", MechanOddTheme::modulation),
      portamento (p.apvts, MechanOddAudioProcessor::portamentoId, "Porta",  MechanOddTheme::modulation)
{
    laf.setAccentColour (MechanOddTheme::modulation);

    for (auto* k : { &voices, &portamento })
    {
        k->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k->setShowLabel (true);          // name drawn below the knob by FxmeLookAndFeel
        k->setLookAndFeel (&laf);
        MechanOddTheme::accentSlider (*k, MechanOddTheme::modulation);
        addAndMakeVisible (*k);
    }

    setSize (preferredWidth, preferredHeight);
}

GlobalPanel::~GlobalPanel()
{
    for (auto* k : { &voices, &portamento })
        k->setLookAndFeel (nullptr);
}

void GlobalPanel::paint (juce::Graphics& g)
{
    MechanOddTheme::paintBackground (g, getLocalBounds().toFloat());
}

void GlobalPanel::resized()
{
    auto area = getLocalBounds().reduced (8);
    const int knobW = area.getWidth() / 2;
    voices.setBounds     (area.removeFromLeft (knobW).reduced (2));
    portamento.setBounds (area.reduced (2));
}

//==============================================================================
void GearButton::paintButton (juce::Graphics& g, bool over, bool down)
{
    auto b = getLocalBounds().toFloat().reduced (2.0f);
    const float d = juce::jmin (b.getWidth(), b.getHeight());
    const auto c = b.getCentre();

    const float outer = d * 0.5f;          // tooth tips
    const float root  = outer * 0.74f;     // between the teeth
    const float hole  = outer * 0.34f;
    constexpr int teeth = 8;

    // Each tooth: a flat top over half its pitch, flanks down to the root.
    juce::Path gear;
    const float pitch = juce::MathConstants<float>::twoPi / (float) teeth;
    for (int i = 0; i < teeth; ++i)
    {
        const float a = (float) i * pitch;
        const float angles[] { a - 0.30f * pitch, a - 0.18f * pitch, a + 0.18f * pitch, a + 0.30f * pitch };
        const float radii[]  { root, outer, outer, root };

        for (int j = 0; j < 4; ++j)
        {
            const auto pt = c.getPointOnCircumference (radii[j], angles[j]);
            if (i == 0 && j == 0)
                gear.startNewSubPath (pt);
            else
                gear.lineTo (pt);
        }

        // Along the root to the next tooth.
        gear.addCentredArc (c.x, c.y, root, root, 0.0f, a + 0.30f * pitch, a + 0.70f * pitch);
    }
    gear.closeSubPath();
    gear.addEllipse (juce::Rectangle<float> (2.0f * hole, 2.0f * hole).withCentre (c));
    gear.setUsingNonZeroWinding (false);

    g.setColour (down ? accent.darker (0.3f)
               : over ? accent.brighter (0.3f)
                      : accent);
    g.fillPath (gear);
}

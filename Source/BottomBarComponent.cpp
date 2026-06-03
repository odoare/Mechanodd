/*
  ==============================================================================

    BottomBarComponent.cpp

  ==============================================================================
*/

#include "BottomBarComponent.h"
#include "Theme.h"

BottomBarComponent::BottomBarComponent (MechanoscAudioProcessor& processor)
    : audioProcessor (processor)
{
    addKnob (outputVolume, MechanoscAudioProcessor::outputVolumeId, "Output");
    addKnob (voices,       MechanoscAudioProcessor::numVoicesId,    "Voices");
    addKnob (portamento,   MechanoscAudioProcessor::portamentoId,   "Porta");

    // Stereo output meter: green moving peak, yellow peak-hold, red over.
    auto makeMeter = [&processor] (int ch)
    {
        auto m = std::make_unique<fxme::HorizontalVuMeter> (
            [&processor, ch] { return processor.getOutputLevelDb (ch); },
            [&processor, ch] { return processor.getOutputHoldDb  (ch); });
        m->setMaxColour         (juce::Colour (0xff3ad07a));   // green
        m->setSmoothedMaxColour (juce::Colour (0xffd9b13a));   // amber
        m->setOverColour        (juce::Colours::red);
        m->setBackgroundColour  (juce::Colours::black);
        m->minValue = -60.0f;
        m->maxValue =   6.0f;
        return m;
    };
    meterL = makeMeter (0);
    meterR = makeMeter (1);
    addAndMakeVisible (*meterL);
    addAndMakeVisible (*meterR);

    for (auto* l : { &meterLabelL, &meterLabelR })
    {
        l->setJustificationType (juce::Justification::centredRight);
        l->setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.7f));
        addAndMakeVisible (*l);
    }
    meterLabelL.setText ("L", juce::dontSendNotification);
    meterLabelR.setText ("R", juce::dontSendNotification);
}

BottomBarComponent::~BottomBarComponent()
{
    for (auto* k : { &outputVolume, &voices, &portamento })
        if (k->slider != nullptr)
            k->slider->setLookAndFeel (nullptr);
}

void BottomBarComponent::addKnob (Knob& k, const juce::String& paramId, const juce::String& text)
{
    k.slider = std::make_unique<fxme::FxmeSlider> (audioProcessor.apvts, paramId, text, juce::Colours::orange);
    k.slider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider->setLookAndFeel (&fxmeLookAndFeel);
    MechanoscTheme::accentSlider (*k.slider, MechanoscTheme::modulation);
    addAndMakeVisible (*k.slider);

    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.85f));
    addAndMakeVisible (k.label);
}

void BottomBarComponent::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle (b, 4.0f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawRoundedRectangle (b, 4.0f, 1.0f);
}

void BottomBarComponent::resized()
{
    auto area = getLocalBounds().reduced (8, 6);
    constexpr int knobW   = 72;
    constexpr int labelH  = 14;

    // Place a labelled knob (label under the knob) in a fixed-width column.
    auto placeKnob = [labelH] (Knob& k, juce::Rectangle<int> col)
    {
        k.label.setBounds (col.removeFromBottom (labelH));
        k.slider->setBounds (col.reduced (2));
    };

    placeKnob (outputVolume, area.removeFromLeft (knobW));
    placeKnob (portamento,   area.removeFromRight (knobW));
    placeKnob (voices,       area.removeFromRight (knobW));

    // Stereo meter fills the middle: two stacked horizontal bars with a small
    // L/R gutter on the left.
    auto meterArea = area.reduced (10, 4);
    auto gutter = meterArea.removeFromLeft (16);
    const int half = meterArea.getHeight() / 2;
    auto top = meterArea.removeFromTop (half).reduced (0, 1);
    auto bot = meterArea.reduced (0, 1);
    meterL->setBounds (top);
    meterR->setBounds (bot);
    meterLabelL.setBounds (gutter.getX(), top.getY(), gutter.getWidth(), top.getHeight());
    meterLabelR.setBounds (gutter.getX(), bot.getY(), gutter.getWidth(), bot.getHeight());
}

/*
  ==============================================================================

    BottomBarComponent.cpp

  ==============================================================================
*/

#include "BottomBarComponent.h"
#include "Theme.h"
#include <BinaryData.h>

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

    logoImage = juce::ImageCache::getFromMemory (BinaryData::logo686_png,
                                                  BinaryData::logo686_pngSize);

    // Preset buttons.
    auto stylePresetButton = [this] (juce::TextButton& btn)
    {
        btn.setLookAndFeel (&fxmeLookAndFeel);
        btn.setColour (juce::TextButton::buttonColourId,   juce::Colours::black.withAlpha (0.4f));
        btn.setColour (juce::TextButton::textColourOffId,  MechanoscTheme::modulation.brighter (0.3f));
        addAndMakeVisible (btn);
    };
    stylePresetButton (loadButton);
    stylePresetButton (saveButton);

    loadButton.setTooltip ("Load preset");
    saveButton.setTooltip ("Save preset");

    loadButton.onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Load preset",
            juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
            "*.xml");
        fileChooser->launchAsync (
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc)
            {
                if (fc.getResults().isEmpty()) return;
                if (auto xml = juce::XmlDocument::parse (fc.getResults()[0]))
                    audioProcessor.apvts.replaceState (juce::ValueTree::fromXml (*xml));
            });
    };

    saveButton.onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Save preset",
            juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
            "*.xml");
        fileChooser->launchAsync (
            juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc)
            {
                if (fc.getResults().isEmpty()) return;
                fc.getResults()[0].withFileExtension ("xml")
                    .replaceWithText (audioProcessor.apvts.state.toXmlString());
            });
    };
}

BottomBarComponent::~BottomBarComponent()
{
    for (auto* k : { &outputVolume, &voices, &portamento })
        if (k->slider != nullptr)
            k->slider->setLookAndFeel (nullptr);
    loadButton.setLookAndFeel (nullptr);
    saveButton.setLookAndFeel (nullptr);
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

    if (logoImage.isValid() && ! logoRect.isEmpty())
    {
        g.setOpacity (0.85f);
        g.drawImageWithin (logoImage,
                           logoRect.getX(), logoRect.getY(),
                           logoRect.getWidth(), logoRect.getHeight(),
                           juce::RectanglePlacement::centred
                               | juce::RectanglePlacement::onlyReduceInSize);
        g.setOpacity (1.0f);
    }
}

void BottomBarComponent::resized()
{
    auto area = getLocalBounds().reduced (8, 6);
    constexpr int knobW   = 88;
    constexpr int labelH  = 14;

    // Place a labelled knob (label under the knob) in a fixed-width column.
    auto placeKnob = [labelH] (Knob& k, juce::Rectangle<int> col)
    {
        k.label.setBounds (col.removeFromBottom (labelH));
        k.slider->setBounds (col.reduced (2));
    };

    placeKnob (outputVolume, area.removeFromLeft (knobW));

    // Preset L/S buttons: two small squares stacked vertically.
    {
        constexpr int btnSize = 30;
        constexpr int gap     = 4;
        auto col = area.removeFromLeft (btnSize + 8);
        auto stack = col.withSizeKeepingCentre (btnSize, btnSize * 2 + gap);
        loadButton.setBounds (stack.removeFromTop (btnSize));
        stack.removeFromTop (gap);
        saveButton.setBounds (stack.removeFromTop (btnSize));
    }

    // Logo on the far right.
    if (logoImage.isValid())
    {
        const int h = area.getHeight();
        const int w = h * logoImage.getWidth() / juce::jmax (1, logoImage.getHeight());
        logoRect = area.removeFromRight (w + 6);
    }

    placeKnob (portamento,   area.removeFromRight (knobW));
    placeKnob (voices,       area.removeFromRight (knobW));

    // Stereo meter fills the middle: two stacked horizontal bars with a small
    // L/R gutter on the left.
    auto meterArea = area.reduced (10, 4);
    auto gutter = meterArea.removeFromLeft (24);
    const int half = meterArea.getHeight() / 2;
    auto top = meterArea.removeFromTop (half).reduced (0, 1);
    auto bot = meterArea.reduced (0, 1);
    meterL->setBounds (top);
    meterR->setBounds (bot);
    meterLabelL.setBounds (gutter.getX(), top.getY(), gutter.getWidth(), top.getHeight());
    meterLabelR.setBounds (gutter.getX(), bot.getY(), gutter.getWidth(), bot.getHeight());
}

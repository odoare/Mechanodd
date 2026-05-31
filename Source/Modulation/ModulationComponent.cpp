/*
  ==============================================================================

    ModulationComponent.cpp

  ==============================================================================
*/

#include "ModulationComponent.h"

ModulationComponent::ModulationComponent (juce::AudioProcessorValueTreeState& state)
    : apvts (state)
{
    for (int i = 0; i < ModEngine::numModulators; ++i)
    {
        auto& row = rows[(size_t) i];

        row.index.setText (juce::String (i), juce::dontSendNotification);
        row.index.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (row.index);

        if (auto* tp = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (ModEngine::targetId (i))))
            row.targetBox.addItemList (tp->choices, 1);
        row.targetBox.setLookAndFeel (&fxmeLookAndFeel);
        addAndMakeVisible (row.targetBox);
        row.targetAtt = std::make_unique<ComboAtt> (apvts, ModEngine::targetId (i), row.targetBox);

        row.shapeBox.addItemList (ModEngine::shapeChoices(), 1);
        row.shapeBox.setLookAndFeel (&fxmeLookAndFeel);
        addAndMakeVisible (row.shapeBox);
        row.shapeAtt = std::make_unique<ComboAtt> (apvts, ModEngine::shapeId (i), row.shapeBox);

        row.rate = std::make_unique<fxme::FxmeSlider> (apvts, ModEngine::rateId (i), "Rate", juce::Colours::orange);
        row.rate->setSliderStyle (juce::Slider::LinearHorizontal);
        row.rate->setLookAndFeel (&fxmeLookAndFeel);
        addAndMakeVisible (*row.rate);

        row.syncButton.setButtonText ("Sync");
        row.syncButton.setLookAndFeel (&fxmeLookAndFeel);
        addAndMakeVisible (row.syncButton);
        row.syncAtt = std::make_unique<ButtonAtt> (apvts, ModEngine::syncId (i), row.syncButton);

        row.syncRateBox.addItemList (ModEngine::syncRateChoices(), 1);
        row.syncRateBox.setLookAndFeel (&fxmeLookAndFeel);
        addAndMakeVisible (row.syncRateBox);
        row.syncRateAtt = std::make_unique<ComboAtt> (apvts, ModEngine::syncRateId (i), row.syncRateBox);

        row.depth = std::make_unique<fxme::FxmeSlider> (apvts, ModEngine::depthId (i), "Depth", juce::Colours::orange);
        row.depth->setSliderStyle (juce::Slider::LinearHorizontal);
        row.depth->setLookAndFeel (&fxmeLookAndFeel);
        addAndMakeVisible (*row.depth);
    }
}

ModulationComponent::~ModulationComponent()
{
    for (auto& row : rows)
    {
        row.targetBox.setLookAndFeel (nullptr);
        row.shapeBox.setLookAndFeel (nullptr);
        row.syncRateBox.setLookAndFeel (nullptr);
        row.syncButton.setLookAndFeel (nullptr);
        if (row.rate  != nullptr) row.rate->setLookAndFeel (nullptr);
        if (row.depth != nullptr) row.depth->setLookAndFeel (nullptr);
    }
}

void ModulationComponent::paint (juce::Graphics& g)
{
    g.setColour (juce::Colours::white.withAlpha (0.6f));
    g.setFont (12.0f);
    auto header = getLocalBounds().reduced (8).removeFromTop (16);
    header.removeFromLeft (24);
    auto col = [&header] (int w) { return header.removeFromLeft (w); };
    g.drawText ("Target",   col (180), juce::Justification::centredLeft);
    g.drawText ("Shape",    col (90),  juce::Justification::centredLeft);
    g.drawText ("Rate",     col (120), juce::Justification::centredLeft);
    g.drawText ("Sync",     col (60),  juce::Justification::centredLeft);
    g.drawText ("Division", col (90),  juce::Justification::centredLeft);
    g.drawText ("Depth",    col (120), juce::Justification::centredLeft);
}

void ModulationComponent::resized()
{
    auto area = getLocalBounds().reduced (8);
    area.removeFromTop (18);   // header

    const int rowH = juce::jmax (22, area.getHeight() / ModEngine::numModulators);

    for (auto& row : rows)
    {
        auto r = area.removeFromTop (rowH).reduced (0, 2);
        row.index.setBounds      (r.removeFromLeft (24));
        row.targetBox.setBounds  (r.removeFromLeft (180).reduced (2, 0));
        row.shapeBox.setBounds   (r.removeFromLeft (90).reduced (2, 0));
        row.rate->setBounds      (r.removeFromLeft (120).reduced (2, 0));
        row.syncButton.setBounds (r.removeFromLeft (60).reduced (2, 0));
        row.syncRateBox.setBounds (r.removeFromLeft (90).reduced (2, 0));
        row.depth->setBounds     (r.removeFromLeft (120).reduced (2, 0));
    }
}

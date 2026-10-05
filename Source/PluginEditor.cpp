/*
  ==============================================================================

    PluginEditor.cpp

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ResonatorSlot.h"
#include "Theme.h"
#include "Tooltips.h"
#include <BinaryData.h>

namespace
{
    // The info button's text: what the plugin is and how to find one's way
    // around it.
    const char* const helpBody =
        "MechanOdd is a modular physical-modelling synth: sources excite resonators "
        "(strings, plates, membranes, bars), which can feed back into each other "
        "through a matrix, then go through two effect chains.\n"
        "\n"
        "Sources: four slots of excitation (noise, wavetable, crackles, or the "
        "plugin's audio input), each with its own envelope and low-pass filter.\n"
        "\n"
        "Resonators: four slots, each a physical model. A resonator follows the "
        "notes (one per voice) or, with Global on, is one shared body at a fixed "
        "pitch. Most settings come in On / Off pairs: while the note is held, and "
        "after it is released.\n"
        "\n"
        "Matrix: each row feeds one resonator. Its knobs choose how much of each "
        "source (S), of each resonator (R, one block later) and of the send "
        "effects (Fx) go into it; centre is off, the sign flips the phase. Lvl, "
        "Pan and Snd send the resonator to the mix and to the send effects. "
        "Feedback loops can run away: raise them slowly, and keep a limiter last "
        "in the master chain.\n"
        "\n"
        "Effects: a send chain and a master chain of four slots each. Choose an "
        "effect in a slot, then its arrow to show it. Each effect has its own "
        "presets (shared with the FxmeFX plugins).\n"
        "\n"
        "Modulation: twelve LFOs or envelopes, each on any parameter "
        "(module, then parameter).\n"
        "\n"
        "Top bar: the output level and meter, the presets (\"...\" opens the "
        "browser to save, rename and delete), and the gear for the voices, the "
        "portamento and the \"?\" that turns these hover tips on and off.\n"
        "\n"
        "Right-click a knob to type a value.";
}

MechanOddAudioProcessorEditor::MechanOddAudioProcessorEditor (MechanOddAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      outputSlider (p.apvts, MechanOddAudioProcessor::outputVolumeId, "Output", MechanOddTheme::modulation)
{
    // ---- Top bar ----------------------------------------------------------------
    laf.setAccentColour (MechanOddTheme::modulation);
    topBar.setAccentColour (MechanOddTheme::modulation);
    addAndMakeVisible (topBar);

    outputSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    outputSlider.setTextValueSuffix (" dB");
    outputSlider.setLookAndFeel (&laf);
    MechanOddTheme::accentSlider (outputSlider, MechanOddTheme::modulation);
    outputSlider.setColour (juce::Slider::backgroundColourId, juce::Colours::black.withAlpha (0.4f));

    presetBar.setAccentColour (MechanOddTheme::modulation);
    presetBar.setBrowserButtonVisible (true);   // "...": the full browser in a callout
    presetBar.setBrowserSize (320, 380);

    globalButton.onClick = [this] { showGlobalPanel(); };

    infoButton.setColours ({ MechanOddTheme::modulation, juce::Colour (0xffd8d8e0),
                             juce::Colour (0xff20202c), juce::Colour (0xff3a3a4c) });
    infoButton.setInfo (MechanOddTheme::displayName, helpBody);

    outputSlider.setTooltip (mechanodd::tips::bar::output);
    outputMeter.setTooltip (mechanodd::tips::bar::meter);
    globalButton.setTooltip (mechanodd::tips::bar::global);

    topBar.setRightControls ({ { &outputSlider, 120 }, { &outputMeter, 130 },
                               { &presetBar, 236 }, { &globalButton, 24 }, { &infoButton, 22 } });

    // ---- Tabs ---------------------------------------------------------------------
    sourcesTab = std::make_unique<juce::Component>();
    for (int i = 0; i < SynthVoice::numSourceSlots; ++i)
    {
        auto slot = std::make_unique<SourceSlotComponent> (audioProcessor.apvts, SynthVoice::sourceSlotPrefix (i));
        MechanOddTheme::applyAccent (*slot, MechanOddTheme::sourceColour (i));
        sourcesTab->addAndMakeVisible (*slot);
        sourceSlots.push_back (std::move (slot));
    }

    resonatorsTab = std::make_unique<juce::Component>();
    for (int i = 0; i < ResonatorSlot::numSlots; ++i)
    {
        auto slot = std::make_unique<ResonatorSlotComponent> (audioProcessor.apvts, ResonatorSlot::slotPrefix (i));
        MechanOddTheme::applyAccent (*slot, MechanOddTheme::resonatorColour (i));
        slot->setMeterColour (MechanOddTheme::resonatorColour (i));
        // This resonator's output is column (numSources + i) of the feedback matrix.
        slot->setLevelProvider ([&p = audioProcessor, col = FeedbackMatrix::numSources + i]
                                { return p.getColumnLevelLinear (col); });
        resonatorsTab->addAndMakeVisible (*slot);
        resonatorSlots.push_back (std::move (slot));
    }

    matrixComponent = std::make_unique<FeedbackMatrixComponent> (audioProcessor.apvts);
    matrixComponent->setColumnLevelProvider ([&p = audioProcessor] (int c) { return p.getColumnLevelLinear (c); });

    effectsTabComponent = std::make_unique<EffectsTabComponent> (audioProcessor);

    modulationComponent = std::make_unique<ModulationComponent> (audioProcessor.apvts);
    // The modulation page stays homogeneous; the matrix colours itself per row.
    MechanOddTheme::applyAccent (*modulationComponent, MechanOddTheme::modulation);

    tabs.addTab ("Sources",    MechanOddTheme::tabButton, sourcesTab.get(),          false);
    tabs.addTab ("Resonators", MechanOddTheme::tabButton, resonatorsTab.get(),       false);
    tabs.addTab ("Matrix",     MechanOddTheme::tabButton, matrixComponent.get(),     false);
    tabs.addTab ("Effects",    MechanOddTheme::tabButton, effectsTabComponent.get(), false);
    tabs.addTab ("Modulation", MechanOddTheme::tabButton, modulationComponent.get(), false);
    addAndMakeVisible (tabs);

    setResizable (true, true);
    // The bottom bar this replaces was 100 px; the top bar takes 54 of it.
    setSize (1280, 654);
}

MechanOddAudioProcessorEditor::~MechanOddAudioProcessorEditor()
{
    outputSlider.setLookAndFeel (nullptr);
}

void MechanOddAudioProcessorEditor::showGlobalPanel()
{
    juce::CallOutBox::launchAsynchronously (std::make_unique<GlobalPanel> (audioProcessor),
                                            getLocalArea (&topBar, globalButton.getBounds()),
                                            this);
}

juce::Image MechanOddAudioProcessorEditor::logoImage()
{
    return juce::ImageCache::getFromMemory (BinaryData::logo686_png, BinaryData::logo686_pngSize);
}

void MechanOddAudioProcessorEditor::paint (juce::Graphics& g)
{
    MechanOddTheme::paintBackground (g, getLocalBounds().toFloat());
}

void MechanOddAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    topBar.setBounds (area.removeFromTop (MechanOddTheme::topBarHeight));
    tabs.setBounds (area);

    if (sourcesTab != nullptr)
    {
        auto area = sourcesTab->getLocalBounds().reduced (8);
        const int rowH = juce::jmax (110, area.getHeight() / juce::jmax (1, (int) sourceSlots.size()));
        for (auto& slot : sourceSlots)
            slot->setBounds (area.removeFromTop (rowH).reduced (0, 3));
    }

    if (resonatorsTab != nullptr)
    {
        auto area = resonatorsTab->getLocalBounds().reduced (8);
        const int rowH = juce::jmax (110, area.getHeight() / juce::jmax (1, (int) resonatorSlots.size()));
        for (auto& slot : resonatorSlots)
            slot->setBounds (area.removeFromTop (rowH).reduced (0, 3));
    }

    // EffectsTabComponent manages its own layout internally.
}

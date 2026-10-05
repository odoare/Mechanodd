/*
  ==============================================================================

    EffectsTabComponent.cpp

  ==============================================================================
*/

#include "EffectsTabComponent.h"
#include "EffectFactory.h"
#include "EffectSlot.h"
#include "EffectChain.h"
#include "../PluginProcessor.h"
#include "../Theme.h"
#include "../Tooltips.h"

namespace tips = mechanodd::tips::effects;

namespace
{
    constexpr int kLeftW      = 220;   // width of the selector column
    constexpr int kPad        = 8;
    constexpr int kLabelH     = 18;
    constexpr int kRowH       = 26;
    constexpr int kRowGap     = 3;
    constexpr int kSectionGap = 14;
    constexpr int kShowBtnW   = 26;
    constexpr int kContPad    = 14;    // the panel container around the effect
    constexpr int kPresetBarW = 300;
    constexpr int kPresetBarH = 26;
    constexpr int kPresetGap  = 8;     // between the preset bar and the container
}

EffectsTabComponent::EffectsTabComponent (MechanOddAudioProcessor& p)
    : processor (p), apvts (p.apvts)
{
    auto styleLabel = [&] (juce::Label& l, const juce::String& text)
    {
        l.setText (text, juce::dontSendNotification);
        l.setFont (juce::Font (12.0f, juce::Font::bold));
        l.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.55f));
        addAndMakeVisible (l);
    };
    styleLabel (sendLabel,  "SEND");
    styleLabel (masterLabel, "MASTER");

    placeholderLabel.setText ("Select a slot  \xe2\x96\xb6  to show its controls",
                              juce::dontSendNotification);
    placeholderLabel.setJustificationType (juce::Justification::centred);
    placeholderLabel.setColour (juce::Label::textColourId,
                                juce::Colours::white.withAlpha (0.2f));
    addAndMakeVisible (placeholderLabel);

    prePostButton.setButtonText ("POST MASTER");
    prePostButton.setLookAndFeel (&laf);
    prePostButton.setTooltip (tips::postMaster);
    prePostButton.setColour (juce::ToggleButton::tickColourId,
                             MechanOddTheme::busColour());
    addAndMakeVisible (prePostButton);
    prePostAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, MechanOddAudioProcessor::busPostMasterId, prePostButton);

    sendVolumeSlider = std::make_unique<fxme::FxmeSlider> (
        apvts, MechanOddAudioProcessor::busOutVolId, "Send Level", juce::Colours::orange);
    sendVolumeSlider->setSliderStyle (juce::Slider::LinearHorizontal);
    sendVolumeSlider->setLookAndFeel (&laf);
    sendVolumeSlider->setTooltip (tips::sendLevel);
    MechanOddTheme::accentSlider (*sendVolumeSlider, MechanOddTheme::busColour());
    addAndMakeVisible (*sendVolumeSlider);

    for (int i = 0; i < kSlotsPerChain; ++i)
        initSlot (i,                "bus",    i);
    for (int i = 0; i < kSlotsPerChain; ++i)
        initSlot (kSlotsPerChain + i, "master", i);
}

EffectsTabComponent::~EffectsTabComponent()
{
    prePostButton.setLookAndFeel (nullptr);
    if (sendVolumeSlider) sendVolumeSlider->setLookAndFeel (nullptr);
    for (auto& s : slots)
        if (s) s->typeBox.setLookAndFeel (nullptr);
}

// ── slot initialisation ────────────────────────────────────────────────────────

void EffectsTabComponent::initSlot (int idx, const juce::String& chain, int pos)
{
    auto e = std::make_unique<SlotEntry>();

    // Type selector
    e->typeBox.addItemList (EffectFactory::typeChoices(), 1);
    e->typeBox.setLookAndFeel (&laf);
    e->typeBox.setTooltip (chain == "bus" ? tips::sendSlot : tips::masterSlot);
    addAndMakeVisible (e->typeBox);

    const juce::String slotPfx = EffectChain::slotPrefix (chain, pos);
    e->slotPrefix = slotPfx;
    e->typeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, EffectSlot::typeParamId (slotPfx), e->typeBox);

    // Show button — exclusive radio-style activation
    e->showButton.setButtonText (juce::CharPointer_UTF8 ("\xe2\x96\xb6")); // ▶
    e->showButton.setClickingTogglesState (false);
    e->showButton.setTooltip (tips::show);
    addAndMakeVisible (e->showButton);
    e->showButton.onClick = [this, idx] { activateSlot (idx); };

    // When the type changes while this slot is shown, refresh the right panel
    e->typeBox.onChange = [this, idx] { if (activeSlot == idx) refreshRightPanel(); };

    slots[(size_t) idx] = std::move (e);
}

// ── activation logic ───────────────────────────────────────────────────────────

void EffectsTabComponent::activateSlot (int idx)
{
    activeSlot = idx;

    // Toggle-style button highlight: only the active one appears pressed
    for (int i = 0; i < kNumSlots; ++i)
        slots[(size_t) i]->showButton.setToggleState (i == idx,
                                                       juce::dontSendNotification);
    refreshRightPanel();
}

void EffectsTabComponent::showPanel (int slot, int type)
{
    if (slot == shownSlot && type == shownType)
        return;

    shownPanel.reset();
    shownSlot = slot;
    shownType = type;

    if (slot < 0 || type < 0)
        return;

    const auto& info = EffectFactory::types()[(size_t) type];
    const auto perTypePfx = EffectSlot::perTypePrefix (slots[(size_t) slot]->slotPrefix, info.name);
    auto& chain = slot < kSlotsPerChain ? processor.getBusChain() : processor.getMasterChain();
    auto& effect = chain.getSlot (slot % kSlotsPerChain).getEffect (type);
    // The slot is set to this effect, so the loader thread loads it anyway;
    // now, because the panel reads the IR list as it is made.
    effect.ensureLoaded();
    shownPanel = info.createComponent (effect, apvts, perTypePfx);
    addChildComponent (*shownPanel);
}

void EffectsTabComponent::refreshRightPanel()
{
    if (activeSlot < 0)
    {
        showPanel (-1, -1);
        placeholderLabel.setVisible (true);
        if (presetBar != nullptr)
            presetBar->setVisible (false);
        repaint();
        return;
    }

    auto& e       = *slots[(size_t) activeSlot];
    const int ti  = e.typeBox.getSelectedItemIndex() - 1;   // -1 = Off

    if (ti >= 0 && ti < (int) EffectFactory::types().size())
    {
        placeholderLabel.setVisible (false);
        const auto& info = EffectFactory::types()[(size_t) ti];
        showPanel (activeSlot, ti);
        shownPanel->setBounds (centredBounds (info));
        shownPanel->setVisible (true);

        // This effect's module presets, in this slot.
        auto* bank = processor.getEffectPresets (EffectSlot::perTypePrefix (e.slotPrefix, info.name));
        if (bank != presetBarBank)
        {
            presetBar.reset();
            presetBarBank = bank;
            if (bank != nullptr)
            {
                presetBar = std::make_unique<fxme::PresetBarComponent> (*bank);
                presetBar->setAccentColour (activeSlot < kSlotsPerChain ? MechanOddTheme::busColour()
                                                                        : MechanOddTheme::modulation);
                presetBar->setBrowserButtonVisible (true);
                presetBar->setBrowserSize (320, 380);
                addChildComponent (*presetBar);
            }
        }
        if (presetBar != nullptr)
        {
            presetBar->setBounds (presetBarBounds (info));
            presetBar->setVisible (true);
        }
    }
    else
    {
        // Slot is set to "Off" — show placeholder
        showPanel (-1, -1);
        placeholderLabel.setVisible (true);
        if (presetBar != nullptr)
            presetBar->setVisible (false);
    }

    repaint();
}

// ── geometry ──────────────────────────────────────────────────────────────────

juce::Rectangle<int> EffectsTabComponent::rightBounds() const
{
    // The top row is kept for the preset bar above the effect's container.
    return getLocalBounds().reduced (kPad).withTrimmedLeft (kLeftW)
                           .withTrimmedTop (kPresetBarH + kPresetGap + kContPad);
}

juce::Rectangle<int> EffectsTabComponent::presetBarBounds (const EffectTypeInfo& info) const
{
    const auto panel = centredBounds (info).expanded (kContPad);
    const int w = juce::jmin (kPresetBarW, panel.getWidth());
    return { panel.getCentreX() - w / 2, panel.getY() - kPresetGap - kPresetBarH, w, kPresetBarH };
}

juce::Rectangle<int> EffectsTabComponent::centredBounds (const EffectTypeInfo& info) const
{
    const auto rb = rightBounds();
    const int w = juce::jmin (info.preferredWidth,  rb.getWidth());
    const int h = juce::jmin (info.preferredHeight, rb.getHeight());
    return { rb.getX() + (rb.getWidth()  - w) / 2,
             rb.getY() + (rb.getHeight() - h) / 2,
             w, h };
}

// ── painting ──────────────────────────────────────────────────────────────────

void EffectsTabComponent::paint (juce::Graphics& g)
{
    // Subtle background for the left selector column
    g.setColour (juce::Colours::black.withAlpha (0.18f));
    g.fillRoundedRectangle (
        getLocalBounds().toFloat().withWidth ((float) kLeftW).reduced (3.0f), 5.0f);

    // Vertical divider
    const float divX = (float) (kLeftW + kPad / 2);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawLine (divX, (float) kPad, divX, (float) (getHeight() - kPad), 1.0f);

    // Panel container around the active effect
    if (activeSlot >= 0)
    {
        auto& e      = *slots[(size_t) activeSlot];
        const int ti = e.typeBox.getSelectedItemIndex() - 1;
        if (ti >= 0 && ti < (int) EffectFactory::types().size())
        {
            constexpr float kRadius  =  8.0f;
            const auto inner = centredBounds (EffectFactory::types()[(size_t) ti]).toFloat();
            const auto outer = inner.expanded ((float) kContPad);

            // Soft drop shadow
            g.setColour (juce::Colours::black.withAlpha (0.45f));
            g.fillRoundedRectangle (outer.translated (0.0f, 4.0f).reduced (2.0f), kRadius + 2.0f);

            // Panel fill
            g.setColour (juce::Colour::fromFloatRGBA (0.10f, 0.10f, 0.17f, 0.92f));
            g.fillRoundedRectangle (outer, kRadius);

            // Outer border
            g.setColour (juce::Colours::white.withAlpha (0.13f));
            g.drawRoundedRectangle (outer.reduced (0.5f), kRadius, 1.0f);

            // Inner highlight ring (bevel depth)
            g.setColour (juce::Colours::white.withAlpha (0.05f));
            g.drawRoundedRectangle (outer.reduced (1.5f), kRadius - 1.0f, 1.0f);
        }
    }
}

// ── layout ────────────────────────────────────────────────────────────────────

void EffectsTabComponent::resized()
{
    auto left = getLocalBounds().reduced (kPad).withWidth (kLeftW - kPad * 2);

    auto layoutSection = [&] (juce::Label& label, int firstSlot)
    {
        label.setBounds (left.removeFromTop (kLabelH));
        left.removeFromTop (4);

        for (int i = 0; i < kSlotsPerChain; ++i)
        {
            auto row = left.removeFromTop (kRowH);
            auto& e  = *slots[(size_t)(firstSlot + i)];

            e.showButton.setBounds (row.removeFromRight (kShowBtnW));
            row.removeFromRight (3);
            e.typeBox.setBounds (row);

            left.removeFromTop (kRowGap);
        }
    };

    layoutSection (sendLabel, 0);
    left.removeFromTop (kRowGap * 2);
    prePostButton.setBounds (left.removeFromTop (kRowH));
    left.removeFromTop (kRowGap);
    if (sendVolumeSlider) sendVolumeSlider->setBounds (left.removeFromTop (kRowH));
    left.removeFromTop (kSectionGap);
    layoutSection (masterLabel, kSlotsPerChain);

    // Right panel: placeholder and whichever effect is active
    placeholderLabel.setBounds (rightBounds());

    if (activeSlot >= 0)
    {
        auto& e  = *slots[(size_t) activeSlot];
        const int ti = e.typeBox.getSelectedItemIndex() - 1;
        if (ti >= 0 && ti < (int) EffectFactory::types().size() && shownPanel != nullptr)
        {
            const auto& info = EffectFactory::types()[(size_t) ti];
            shownPanel->setBounds (centredBounds (info));
            if (presetBar != nullptr)
                presetBar->setBounds (presetBarBounds (info));
        }
    }
}

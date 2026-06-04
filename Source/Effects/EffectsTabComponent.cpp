/*
  ==============================================================================

    EffectsTabComponent.cpp

  ==============================================================================
*/

#include "EffectsTabComponent.h"
#include "EffectFactory.h"
#include "EffectSlot.h"
#include "EffectChain.h"

namespace
{
    constexpr int kLeftW      = 220;   // width of the selector column
    constexpr int kPad        = 8;
    constexpr int kLabelH     = 18;
    constexpr int kRowH       = 26;
    constexpr int kRowGap     = 3;
    constexpr int kSectionGap = 14;
    constexpr int kShowBtnW   = 26;
}

EffectsTabComponent::EffectsTabComponent (juce::AudioProcessorValueTreeState& apvtsIn)
    : apvts (apvtsIn)
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

    for (int i = 0; i < kSlotsPerChain; ++i)
        initSlot (i,                "bus",    i);
    for (int i = 0; i < kSlotsPerChain; ++i)
        initSlot (kSlotsPerChain + i, "master", i);
}

EffectsTabComponent::~EffectsTabComponent()
{
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
    addAndMakeVisible (e->typeBox);

    const juce::String slotPfx = EffectChain::slotPrefix (chain, pos);
    e->typeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, EffectSlot::typeParamId (slotPfx), e->typeBox);

    // Show button — exclusive radio-style activation
    e->showButton.setButtonText (juce::CharPointer_UTF8 ("\xe2\x96\xb6")); // ▶
    e->showButton.setClickingTogglesState (false);
    addAndMakeVisible (e->showButton);
    e->showButton.onClick = [this, idx] { activateSlot (idx); };

    // GUI-side effect instances + their control panels
    for (const auto& info : EffectFactory::types())
    {
        e->guiEffects.push_back (info.create());
        auto comp = info.createComponent (*e->guiEffects.back(), apvts,
                                          EffectSlot::perTypePrefix (slotPfx, info.name));
        comp->setVisible (false);
        addChildComponent (*comp);
        e->typeComponents.push_back (std::move (comp));
    }

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

void EffectsTabComponent::refreshRightPanel()
{
    // Hide every effect component
    for (auto& s : slots)
        for (auto& c : s->typeComponents)
            c->setVisible (false);

    if (activeSlot < 0)
    {
        placeholderLabel.setVisible (true);
        return;
    }

    auto& e       = *slots[(size_t) activeSlot];
    const int ti  = e.typeBox.getSelectedItemIndex() - 1;   // -1 = Off

    if (ti >= 0 && ti < (int) e.typeComponents.size())
    {
        placeholderLabel.setVisible (false);
        auto* comp = e.typeComponents[(size_t) ti].get();
        comp->setBounds (rightBounds());
        comp->setVisible (true);
    }
    else
    {
        // Slot is set to "Off" — show placeholder
        placeholderLabel.setVisible (true);
    }
}

// ── geometry ──────────────────────────────────────────────────────────────────

juce::Rectangle<int> EffectsTabComponent::rightBounds() const
{
    return getLocalBounds().reduced (kPad).withTrimmedLeft (kLeftW);
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

    layoutSection (sendLabel,   0);
    left.removeFromTop (kSectionGap);
    layoutSection (masterLabel, kSlotsPerChain);

    // Right panel: placeholder and whichever effect is active
    placeholderLabel.setBounds (rightBounds());

    if (activeSlot >= 0)
    {
        auto& e  = *slots[(size_t) activeSlot];
        const int ti = e.typeBox.getSelectedItemIndex() - 1;
        if (ti >= 0 && ti < (int) e.typeComponents.size())
            e.typeComponents[(size_t) ti]->setBounds (rightBounds());
    }
}

/*
  ==============================================================================

    ModulationComponent.cpp

  ==============================================================================
*/

#include "ModulationComponent.h"
#include "../Resonators/ResonatorFactory.h"

namespace
{
    // Returns the APVTS slot prefix for a typed-slot group label, or "" for
    // groups with no per-type parameter filtering (Matrix, None, etc.).
    juce::String slotPrefixFromGroupLabel (const juce::String& label)
    {
        if (label.startsWith ("Bus FX "))    return "bus_fx"    + label.substring  (7).trim();
        if (label.startsWith ("Master FX ")) return "master_fx" + label.substring (10).trim();
        if (label.startsWith ("Resonator ")) return "res"       + label.substring (10).trim();
        if (label.startsWith ("Source "))    return "src"       + label.substring  (7).trim();
        return {};
    }

    // Map a target parameter id onto a human-readable module group label.
    juce::String moduleLabelForId (const juce::String& id)
    {
        if (id == "None")
            return "None";

        auto numAfter = [&id] (const juce::String& prefix) -> juce::String
        {
            auto rest = id.substring (prefix.length());
            return rest.initialSectionContainingOnly ("0123456789");
        };

        if (id.startsWith ("master_fx")) return "Master FX " + numAfter ("master_fx");
        if (id.startsWith ("bus_fx"))    return "Bus FX "    + numAfter ("bus_fx");
        if (id.startsWith ("src"))       return "Source "    + numAfter ("src");
        if (id.startsWith ("res"))       return "Resonator " + numAfter ("res");
        if (id.startsWith ("mtx_"))      return "Matrix";

        return id.upToFirstOccurrenceOf ("_", false, false);
    }
}

bool ModulationComponent::shouldShowParam (const juce::String& groupLabel,
                                            const juce::String& paramId) const
{
    const auto slotPfx = slotPrefixFromGroupLabel (groupLabel);
    if (slotPfx.isEmpty())
        return true;    // Matrix / None / unknown group — show everything

    // Global slot params (e.g. res0_globalFreq, res0_level) have no underscore
    // in the segment that follows the slot prefix, so they are always relevant.
    const auto afterPrefix = paramId.substring (slotPfx.length() + 1);
    if (! afterPrefix.contains ("_"))
        return true;

    auto* typeParam = dynamic_cast<juce::AudioParameterChoice*> (
        apvts.getParameter (slotPfx + "_type"));
    if (typeParam == nullptr)
        return true;

    juce::String activeName;
    if (slotPfx.startsWith ("res"))
    {
        // Resonator choices use display names ("Plate rect.") that differ from
        // the internal type names ("Plate") used in parameter IDs.  Look up the
        // canonical name via the factory index instead.
        const int idx = typeParam->getIndex();
        const auto& types = ResonatorFactory::types();
        if (idx >= 0 && idx < (int) types.size())
            activeName = types[(size_t) idx].name;
    }
    else
    {
        // Effects and sources: "Off" is index 0, choice names equal type names.
        activeName = typeParam->getCurrentChoiceName();
    }

    if (activeName.isEmpty() || activeName == "Off")
        return false;   // slot is Off or unrecognised — nothing to modulate

    return paramId.startsWith (slotPfx + "_" + activeName + "_");
}

void ModulationComponent::buildTargetGroups()
{
    targetGroups.clear();

    juce::StringArray ids;
    if (auto* tp = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (ModEngine::targetId (0))))
        ids = tp->choices;

    for (int fi = 0; fi < ids.size(); ++fi)
    {
        const auto& id = ids[fi];
        const auto moduleLabel = moduleLabelForId (id);

        juce::String paramLabel = "None";
        if (id != "None")
            if (auto* p = apvts.getParameter (id))
                paramLabel = p->getName (64);

        // Find or create the group, preserving first-seen order.
        auto it = std::find_if (targetGroups.begin(), targetGroups.end(),
                                [&] (const ModuleGroup& g) { return g.label == moduleLabel; });
        if (it == targetGroups.end())
        {
            targetGroups.push_back ({ moduleLabel, {} });
            it = std::prev (targetGroups.end());
        }
        it->params.push_back ({ paramLabel, fi, id });
    }
}

ModulationComponent::ModulationComponent (juce::AudioProcessorValueTreeState& state)
    : apvts (state)
{
    buildTargetGroups();

    for (int i = 0; i < ModEngine::numModulators; ++i)
    {
        auto& row = rows[(size_t) i];

        row.index.setText (juce::String (i), juce::dontSendNotification);
        row.index.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (row.index);

        // Type (LFO / ADSR).
        row.typeBox.addItemList (ModEngine::typeChoices(), 1);
        row.typeBox.setLookAndFeel (&fxmeLookAndFeel);
        addAndMakeVisible (row.typeBox);
        row.typeAtt = std::make_unique<ComboAtt> (apvts, ModEngine::typeId (i), row.typeBox);
        row.typeBox.onChange = [this, &row] { updateRowTypeVisibility (row); };

        // Two-level target picker.
        for (size_t g = 0; g < targetGroups.size(); ++g)
            row.moduleBox.addItem (targetGroups[g].label, (int) g + 1);
        row.moduleBox.setLookAndFeel (&fxmeLookAndFeel);
        addAndMakeVisible (row.moduleBox);

        row.paramBox.setLookAndFeel (&fxmeLookAndFeel);
        addAndMakeVisible (row.paramBox);

        row.moduleBox.onChange = [this, &row]
        {
            if (row.updatingTarget)
                return;
            const int g = row.moduleBox.getSelectedId() - 1;
            if (g < 0 || g >= (int) targetGroups.size())
                return;
            // Repopulate the parameter box for the chosen module, filtered to
            // only the parameters that belong to the currently active effect type.
            row.updatingTarget = true;
            row.paramBox.clear (juce::dontSendNotification);
            const auto& grp = targetGroups[(size_t) g];
            for (size_t pi = 0; pi < grp.params.size(); ++pi)
                if (shouldShowParam (grp.label, grp.params[pi].paramId))
                    row.paramBox.addItem (grp.params[pi].label, (int) pi + 1);
            row.paramBox.setSelectedItemIndex (0, juce::dontSendNotification);
            row.updatingTarget = false;
            applyTargetFromBoxes (row);
        };

        row.paramBox.onChange = [this, &row]
        {
            if (row.updatingTarget)
                return;
            applyTargetFromBoxes (row);
        };

        // Keep the two boxes in sync with the underlying flat target parameter
        // (handles preset loads and host automation).
        if (auto* tparam = apvts.getParameter (ModEngine::targetId (i)))
        {
            row.targetAtt = std::make_unique<juce::ParameterAttachment> (
                *tparam,
                [this, &row] (float flat) { syncTargetBoxesFromFlat (row); },
                nullptr);
            row.targetAtt->sendInitialUpdate();
        }

        // LFO controls.
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
        // Bipolar control (centre = 0 = no modulation): fill the bar from the centre.
        row.depth->getProperties().set ("drawFromCentre", true);
        row.depth->setLookAndFeel (&fxmeLookAndFeel);
        addAndMakeVisible (*row.depth);

        // ADSR controls.
        auto makeAdsrSlider = [&] (std::unique_ptr<fxme::FxmeSlider>& s, const juce::String& id, const juce::String& name)
        {
            s = std::make_unique<fxme::FxmeSlider> (apvts, id, name, juce::Colours::orange);
            s->setSliderStyle (juce::Slider::LinearHorizontal);
            s->setLookAndFeel (&fxmeLookAndFeel);
            addAndMakeVisible (*s);
        };
        makeAdsrSlider (row.attack,  ModEngine::attackId (i),  "A");
        makeAdsrSlider (row.decay,   ModEngine::decayId (i),   "D");
        makeAdsrSlider (row.sustain, ModEngine::sustainId (i), "S");
        makeAdsrSlider (row.release, ModEngine::releaseId (i), "R");
        makeAdsrSlider (row.amount,  ModEngine::amountId (i),  "Amt");

        row.polarityBox.addItemList (ModEngine::polarityChoices(), 1);
        row.polarityBox.setLookAndFeel (&fxmeLookAndFeel);
        addAndMakeVisible (row.polarityBox);
        row.polarityAtt = std::make_unique<ComboAtt> (apvts, ModEngine::polarityId (i), row.polarityBox);

        updateRowTypeVisibility (row);
    }
}

ModulationComponent::~ModulationComponent()
{
    for (auto& row : rows)
    {
        row.typeBox.setLookAndFeel (nullptr);
        row.moduleBox.setLookAndFeel (nullptr);
        row.paramBox.setLookAndFeel (nullptr);
        row.shapeBox.setLookAndFeel (nullptr);
        row.syncRateBox.setLookAndFeel (nullptr);
        row.syncButton.setLookAndFeel (nullptr);
        row.polarityBox.setLookAndFeel (nullptr);
        if (row.rate    != nullptr) row.rate->setLookAndFeel (nullptr);
        if (row.depth   != nullptr) row.depth->setLookAndFeel (nullptr);
        if (row.attack  != nullptr) row.attack->setLookAndFeel (nullptr);
        if (row.decay   != nullptr) row.decay->setLookAndFeel (nullptr);
        if (row.sustain != nullptr) row.sustain->setLookAndFeel (nullptr);
        if (row.release != nullptr) row.release->setLookAndFeel (nullptr);
        if (row.amount  != nullptr) row.amount->setLookAndFeel (nullptr);
    }
}

void ModulationComponent::syncTargetBoxesFromFlat (Row& row)
{
    // Find the (group, position) of the flat target index currently held by the param.
    auto* tparam = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (
        ModEngine::targetId ((int) (&row - rows.data()))));
    if (tparam == nullptr)
        return;

    const int flat = tparam->getIndex();

    for (size_t g = 0; g < targetGroups.size(); ++g)
        for (size_t pi = 0; pi < targetGroups[g].params.size(); ++pi)
            if (targetGroups[g].params[pi].flatIndex == flat)
            {
                row.updatingTarget = true;
                row.moduleBox.setSelectedId ((int) g + 1, juce::dontSendNotification);
                row.paramBox.clear (juce::dontSendNotification);
                const auto& grp = targetGroups[g];
                for (size_t p = 0; p < grp.params.size(); ++p)
                {
                    // Always show the currently-saved target even if the effect type
                    // has since changed (so a stale preset target remains visible).
                    if (shouldShowParam (grp.label, grp.params[p].paramId) || p == pi)
                        row.paramBox.addItem (grp.params[p].label, (int) p + 1);
                }
                row.paramBox.setSelectedId ((int) pi + 1, juce::dontSendNotification);
                row.updatingTarget = false;
                return;
            }
}

void ModulationComponent::applyTargetFromBoxes (Row& row)
{
    const int g = row.moduleBox.getSelectedId() - 1;
    const int p = row.paramBox.getSelectedId() - 1;
    if (g < 0 || g >= (int) targetGroups.size())
        return;
    if (p < 0 || p >= (int) targetGroups[(size_t) g].params.size())
        return;

    const int flat = targetGroups[(size_t) g].params[(size_t) p].flatIndex;
    if (row.targetAtt != nullptr)
        row.targetAtt->setValueAsCompleteGesture ((float) flat);
}

void ModulationComponent::updateRowTypeVisibility (Row& row)
{
    const bool adsr = row.typeBox.getSelectedId() - 1 == ModEngine::typeAdsr;

    row.shapeBox.setVisible (! adsr);
    row.rate->setVisible (! adsr);
    row.syncButton.setVisible (! adsr);
    row.syncRateBox.setVisible (! adsr);
    row.depth->setVisible (! adsr);

    row.attack->setVisible (adsr);
    row.decay->setVisible (adsr);
    row.sustain->setVisible (adsr);
    row.release->setVisible (adsr);
    row.amount->setVisible (adsr);
    row.polarityBox.setVisible (adsr);

    resized();
}

void ModulationComponent::paint (juce::Graphics& g)
{
    g.setColour (juce::Colours::white.withAlpha (0.6f));
    g.setFont (12.0f);
    auto header = getLocalBounds().reduced (8).removeFromTop (16);
    header.removeFromLeft (24);
    auto col = [&header] (int w) { return header.removeFromLeft (w); };
    g.drawText ("Type",   col (105), juce::Justification::centredLeft);
    g.drawText ("Module", col (110), juce::Justification::centredLeft);
    g.drawText ("Param",  col (195), juce::Justification::centredLeft);
    g.drawText ("Settings", col (400), juce::Justification::centredLeft);
}

void ModulationComponent::resized()
{
    auto area = getLocalBounds().reduced (8);
    area.removeFromTop (18);   // header

    const int rowH = juce::jmax (22, area.getHeight() / ModEngine::numModulators);

    for (auto& row : rows)
    {
        auto r = area.removeFromTop (rowH).reduced (0, 2);

        // During construction some rows aren't fully built yet; their rect is
        // still consumed above so alignment is preserved once everything exists.
        if (row.rate == nullptr || row.attack == nullptr)
            continue;

        row.index.setBounds     (r.removeFromLeft (24));
        row.typeBox.setBounds    (r.removeFromLeft (105).reduced (2, 0));
        row.moduleBox.setBounds  (r.removeFromLeft (110).reduced (2, 0));
        row.paramBox.setBounds   (r.removeFromLeft (195).reduced (2, 0));

        // Variable cluster occupies the remaining width.
        auto cluster = r;
        if (row.typeBox.getSelectedId() - 1 == ModEngine::typeAdsr)
        {
            const int n = 6;   // A D S R Amt + polarity
            const int w = juce::jmax (40, cluster.getWidth() / n);
            row.attack->setBounds   (cluster.removeFromLeft (w).reduced (2, 0));
            row.decay->setBounds    (cluster.removeFromLeft (w).reduced (2, 0));
            row.sustain->setBounds  (cluster.removeFromLeft (w).reduced (2, 0));
            row.release->setBounds  (cluster.removeFromLeft (w).reduced (2, 0));
            row.amount->setBounds   (cluster.removeFromLeft (w).reduced (2, 0));
            row.polarityBox.setBounds (cluster.removeFromLeft (w).reduced (2, 0));
        }
        else
        {
            row.shapeBox.setBounds    (cluster.removeFromLeft (90).reduced (2, 0));
            row.rate->setBounds       (cluster.removeFromLeft (120).reduced (2, 0));
            row.syncButton.setBounds  (cluster.removeFromLeft (60).reduced (2, 0));
            row.syncRateBox.setBounds (cluster.removeFromLeft (90).reduced (2, 0));
            row.depth->setBounds      (cluster.removeFromLeft (120).reduced (2, 0));
        }
    }
}

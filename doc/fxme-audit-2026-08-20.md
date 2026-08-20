# MechanOdd — FX-Mechanics compliance audit

Audited 2026-08-20 at commit `6b505a3` ("Adapt to the new Lfo API of FxmeTools").

FxmeTools reaches this project through FxmeFX (`lib/FxmeFX/lib/FxmeTools`), at
`b1e29d98`, checkout clean. That checkout has `core/`, so the project is on the
far side of the core/shell split and was migrated onto it earlier the same day.
The plugin was built against the pre-split library (green) and against the
post-split one (green) before this audit ran.

This audit is static: greps and reads, no build. See "What a static audit could
not see" at the end.

Single target: `MechanOdd` (synth, AU + VST3 + Standalone).

## Silent bugs

None found. This is the section that lists things already broken in a shipped
build, and every check in it came back clean. Details are in "Already correct"
below, because a clean result is worth recording explicitly.

## Retrofit

Mechanical, low risk unless marked otherwise.

- [ ] **R1. No `fxme::TextEntryFocusFixer` anywhere, and the editor declines
      keyboard focus.** There are 15 `fxme::FxmeSlider` instances, and every one
      of them opens an inline `TextEditor` on right-click, so typing a value into
      a knob is likely dead in a hosted plugin window. Two things are needed and
      they go together. **safe to apply**
  - [ ] Add `fxme::TextEntryFocusFixer textEntryFixer { *this };` to
        [../Source/PluginEditor.h](../Source/PluginEditor.h), declared after the
        child components.
  - [ ] Change `EDITOR_WANTS_KEYBOARD_FOCUS` from `FALSE` to `TRUE` in
        [../CMakeLists.txt:45](../CMakeLists.txt#L45).

- [ ] **R2. `FxmeLookAndFeel::setAccentColour()` is never called.** Thirteen
      components own their own `fxme::FxmeLookAndFeel`, six of them carry combo
      boxes (13 boxes in total), and none tints its drop-down menus, so every
      menu opens in neutral grey against a coloured panel. A drop-down is its own
      window and cannot see the box that opened it, which is why the call is
      needed per look-and-feel instance rather than per widget. The two existing
      `setAccentColour` calls in
      [../Source/PluginEditor.cpp:49](../Source/PluginEditor.cpp#L49) and
      [:62](../Source/PluginEditor.cpp#L62) are `PresetComponent`'s and
      `PresetBarComponent`'s own methods, not the look-and-feel's, so they do not
      cover this. **safe to apply**
  - [ ] `SourceSlotComponent`, `WavetableOscSourceComponent`,
        `EffectsTabComponent`, `EffectSlotComponent`, `ResonatorSlotComponent`,
        `ModulationComponent` (the six owning combo boxes).
  - [ ] The remaining seven for consistency, since they own tooltipped or
        menu-bearing widgets too.

- [ ] **R3. Two `setTooltip()` calls with no `TooltipWindow` in the project, so
      neither tooltip is ever visible.** They are on the preset load and save
      buttons in
      [../Source/BottomBarComponent.cpp:62-63](../Source/BottomBarComponent.cpp#L62).
      Adding the window makes tooltips start appearing across the whole editor,
      which is a product change rather than a cleanup, so it is yours to call.
      **decision**

- [ ] **R4. One deprecated `juce::Font` constructor.**
      [../Source/Effects/EffectsTabComponent.cpp:33](../Source/Effects/EffectsTabComponent.cpp#L33)
      uses `juce::Font (12.0f, juce::Font::bold)`; the replacement is
      `juce::Font (juce::FontOptions (12.0f).withStyle ("Bold"))`. The two
      `g.setFont (12.0f)` calls in
      [../Source/Matrix/FeedbackMatrixComponent.cpp:119](../Source/Matrix/FeedbackMatrixComponent.cpp#L119)
      and
      [../Source/Modulation/ModulationComponent.cpp:325](../Source/Modulation/ModulationComponent.cpp#L325)
      are the genuine `Graphics::setFont (float)` overload, are not deprecated,
      and must be left alone. **safe to apply**

- [ ] **R5. Three parameter-bound toggles are hand-attached rather than using
      `fxme::FxmeButton`,** which wraps the button and its `ButtonAttachment`
      together. **safe to apply**
  - [ ] `prePostButton` in [../Source/Effects/EffectsTabComponent.h:49](../Source/Effects/EffectsTabComponent.h#L49)
  - [ ] `globalButton` in [../Source/Resonators/ResonatorSlotComponent.h:46](../Source/Resonators/ResonatorSlotComponent.h#L46)
  - [ ] `syncButton` in [../Source/Modulation/ModulationComponent.h:62](../Source/Modulation/ModulationComponent.h#L62)

- [ ] **R6. The modulation rate control and its tempo-sync counterpart never
      call `setEnabled()`.** In each modulation row, `row.rate` (free Hz) is
      ignored whenever sync is on, and `row.syncRateBox` is ignored whenever it
      is off, but both stay looking live. Since 2026-08 the look-and-feel
      actually renders a disabled control (desaturate plus fade), so wiring this
      up now shows. Poll from a ~10 Hz `Timer` rather than the button's
      `onClick`, or host automation of the sync parameter is missed. The
      surrounding `setVisible` calls at
      [../Source/Modulation/ModulationComponent.cpp:306-316](../Source/Modulation/ModulationComponent.cpp#L306)
      handle the LFO/ADSR mode switch and are correct as they are; this is a
      different axis. **safe to apply**

## House style

- [ ] **H1. Plugin state carries no version attribute.**
      [../Source/PluginProcessor.cpp:520](../Source/PluginProcessor.cpp#L520)
      writes the APVTS XML with no `xml->setAttribute ("version", 1)`, and
      `setStateInformation` reads none. Nothing is broken yet, but this cannot be
      added retroactively: any session or preset saved before the attribute
      exists is indistinguishable from version 1 forever, so the longer it waits
      the more state there is to guess about. **safe to apply**

- [ ] **H2. `Source/Resonators/Biquad.h` duplicates `fxme::Biquad`.** The local
      48-line transposed-direct-form-II biquad with an RBJ band-pass is the same
      thing as `<FxmeTools/dsp/Biquad.h>` (`fxme::BiquadCoeffs::bandpass` plus
      `fxme::Biquad::processSample`), down to the reasoning in its header comment
      about avoiding `juce::dsp::IIR`'s reference-counted coefficients. It has
      one user, `ModalResonator.h`. The FxmeTools version lives in `core/`, so it
      carries no JUCE dependency either. **safe to apply**

- [ ] **H3. The resonator and matrix DSP is a candidate for promotion into
      FxmeTools `core/`.** `PlateResonator.h` already names no `juce::` type at
      all; `WaveguideResonator.h` (5 references), `ModalResonator.h` (8) and
      `FeedbackMatrix.h` (10) are close. Physical-modelling resonators and a
      feedback matrix are exactly the kind of thing the split exists to make
      reusable, and writing them against `fxme::AudioBufferView` instead of
      `juce::AudioBuffer<float>&` costs no call-site change because the view
      converts implicitly. This is a real piece of design work on a library
      shared with thirteen other projects, not a cleanup, and it is only worth
      doing if the reuse is real. **decision**

## Already correct

Recorded so that a later reader can tell a checked-and-clean area from an
unexamined one.

**The shipped-silent-bug list is clean, all of it.** The `if(APPLE)` block sits
before `project()` with a 10.13 deployment target and both architectures
([../CMakeLists.txt:13-18](../CMakeLists.txt#L13)). `AU_MAIN_TYPE` is absent,
which is correct rather than missing: JUCE derives `kAudioUnitType_MusicDevice`
from `IS_SYNTH TRUE` (verified in JUCE's own `JUCEUtils.cmake`, the
`_juce_set_property_if_not_set` chain), so a synth needs no explicit setting. The
release workflow's "Verify universal binaries" step can genuinely fail: it checks
the bundle exists and greps for both architecture slices, setting a non-zero
status. The README has an Installing section with the macOS quarantine
instructions for all three formats. The per-host MIDI routing note does not apply
here, since that caveat is about audio effects that want notes, and this is a
synth.

**The core/shell split is fully wired and verified.** The helper is included
through the nested path
([../CMakeLists.txt:30](../CMakeLists.txt#L30)) and `fxmetools_attach(MechanOdd)`
links `FxmeTools` and `FxmeCore` together. The auxiliary-target trap does not
apply: this repository declares exactly one target. (The `add_executable` hits
under `Builds/JUCE/` are JUCE's own imported `juceaide` configuration files, not
project targets, and `Builds/` is gitignored.) The renamed-API sweep is clean now
that `ModEngine::shapeChoices()` and `syncRateChoices()` forward to
`Lfo::shapeNames`/`numShapes` and `syncRateNames`/`numSyncRates`; no
`getScaleTypeNames`, no `MidiTools`, no `getSortedSet`. The behavioural changes
have no exposure here, since nothing uses `fxme::Random` or `Downsampler`.

**Registration is consistent.** The single target appears in the root
CMakeLists, the release workflow and the README.

**Realtime safety looks sound.** `processBlock` opens with
`juce::ScopedNoDenormals`, reads parameters through atomic loads, guards
`getPlayHead()` before dereferencing it and uses the modern
`getPosition()`/`orFallback` API, and works into a pre-allocated `inputCapture`
buffer. No allocation, locking, or I/O on that path.

**Four generic findings did not apply, and the greps that suggest them are
misleading here.** Recording them so they are not re-raised:

- *Bare `juce::Slider` instead of `FxmeSlider`.* The 20 `juce::Slider` matches
  are colour identifiers and a `Theme.h` helper signature. Every real slider is
  an `fxme::FxmeSlider`.
- *Combo boxes missing `setLookAndFeel`.* All 13 have it, with matching `nullptr`
  clears in the destructors. This project uses the per-widget look-and-feel
  pattern correctly.
- *Knobs missing `setName`.* The count of literal `setName` calls is zero because
  the sliders are built through `FxmeSlider`'s APVTS constructor, which calls
  `setName (labelText)` itself. All eight labelled knobs pair it with
  `setShowLabel (true)`.
- *`CracksSource` reimplementing a crackle generator.* It already wraps
  `fxme::CracksGenerator`.

`Theme.h` centralises the per-section accents with `accentSlider`,
`accentComboBox`, `accentToggleButton` and a recursive `applyAccent`, which is
the house pattern for a panel with many controls.

## What a static audit could not see

- [ ] Paste the warnings from the last build if you want full deprecation
      coverage. Step 9 greps find the common `juce::Font` and `createWriterFor`
      cases, but only the compiler enumerates them all.
- Whether R1 is actually biting. That the fixer is absent is certain; that
  right-click value entry is broken in your hosts is an inference from the
  absence, and the symptom (caret blinks, keystrokes go to the DAW) is
  host-specific. Trying it in one DAW settles it in a minute.
- Anything only a running plugin shows: voice-stealing behaviour, CPU under
  load, denormal stalls on long resonator tails.

The project has been built against the current FxmeTools, so unlike an unmigrated
one there is no outstanding question about renamed APIs breaking at compile time
where no grep would find them.

## Commit plan

Nothing here is committed, including this file.

- [ ] Nothing to commit in `lib/FxmeFX` or its nested FxmeTools: both checkouts
      are clean, and none of these findings needs a shared-library edit.
      (H2 consumes an existing FxmeTools header rather than changing one, and H3
      would change the library but is a decision, not an action.)
- [ ] Commit whichever findings you apply, plus this file, in MechanOdd.
- [ ] Rebuild and check:
      `cmake --build build -j2 --target MechanOdd_VST3`
- [ ] Copy the bundle to the VST3 folder and let the DAW rescan; it caches the
      loaded module.

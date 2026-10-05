# MechanOdd: done

Implemented items, newest first. Items that came from [todo.md](todo.md) keep
their number there.

## Effect panels show what plays: bound to the processor's effects (2026-10-05)

Picking another IR in a Cab or Reverb panel did not update its drawing (nor
Length, Shape or Offset the reverb's curve). Each panel was bound to a
GUI-side copy of its effect, never prepared, so the copy's loader thread
(started by `prepare`) never ran: the copy never loaded the IR the combo
box selected, and the panel draws from it. The sound was right, since the
processor's own instance does load it. In FxmeFX's plugins the editor binds
to the processor's instance, which is why they work. This dates from FxmeFX
moving IR loading to a background thread, not from today's changes.

Now the effects tab binds each panel to the processor's instance of that
effect in that slot (`EffectChain::getSlot`, `EffectSlot::getEffect`,
`getBusChain` / `getMasterChain` on the processor), as FxmeFX's editors do;
the effects are written for it. Also fixes anything else a panel shows from
its effect (meters, gain reduction: a copy never processed audio), and
showing a Cab or Reverb panel no longer loads an IR of its own.

The effects' parameter connections (`EffectChain::assignParameters`) moved
from `prepareToPlay` to the processor's constructor, so a panel opened
before the host prepares the plugin is bound to a connected effect (a
reverb embedding an external IR needs its link to the state). Every
effect's `assignParameters` only looks up parameters (and, for the reverb,
registers a state listener), so it does not depend on `prepare`.

Checked: no build run yet.

FxmeFX also bumped to 0.4.2 (`Source/Common/Version.h`). It said 0.4.0: the
v0.4.1 tag was made without bumping it, so 0.4.1 builds show 0.4.0.

## 14. FxmeFX: Cab and ConvolReverb IRs at the session rate (2026-10-05)

A bug in FxmeFX, found while making the IR loading lazy. Cab and
ConvolReverb resample an IR to their current rate when they load it, and
start at 44.1 kHz; every host gives them their IR list (which loads the
selected IR) before the first `prepare`, and `prepare` rebuilt the engine
from that buffer without reloading it. So in any session not at 44.1 kHz,
the IR selected at startup played at the wrong speed: at 48 kHz about 9 %
fast, 1.5 semitones high and 8 % shorter. An IR picked later was right
(the loader thread loads at the current rate), until the next rate change.

Fixed in the FxmeFX submodule (`Source/Cab/Cab.cpp`,
`Source/ConvolReverb/ConvolReverb.{h,cpp}`): `prepare` reloads the current
IRs from their source when the rate changed (`ConvolReverb::reloadCurrentIR`,
now also used by the normalisation switch; Cab reloads its two slots),
then rebuilds as before. The rate is now written under the loader lock, as
the loader thread reads it. External reverb IRs reload too (from the
embedded audio).

Affects every FxmeFX plugin built on these two effects (FxmeCab,
FxmeConvolReverb, MechanOdd, FxmeSampler): existing sessions at 48 kHz and
above will sound different, now as intended. `fxme::FirFilter` (FxmeTools)
was checked and does not have the problem: it keeps the IR at its source
rate and resamples on every rebuild.

Checked: no build run yet.

## Faster loading: IRs loaded only by slots that use them (2026-10-05)

The second half of the load-time work: every slot still held a Cab and a
ConvolReverb that loaded an IR (and, once prepared, ran a polling thread)
whether the slot used them or not.

- `DeferredIRAdapter` (in `EffectFactory.cpp`), the base of the Cab and
  ConvolReverb adapters: nothing is loaded at construction; `prepare` only
  records the rate and block size; `ensureLoaded()` (a new `Effect` hook,
  never on the audio thread) gives the effect its IR list, which loads the
  IR, and then prepares it. Until then `process` outputs silence. Once
  loaded, an effect stays loaded.
- The processor's `EffectLoader` thread polls the slot types every 20 ms
  and calls `ensureLoaded()` on the effect each slot is set to
  (`EffectChain::loadActiveEffects`), so a slot switched to Reverb by the
  GUI, automation, a preset or a session loads within a moment, with a
  short silence on that slot. `prepareToPlay` pauses the thread, does the
  same synchronously (a restored session plays its reverb from the first
  block), and restarts it. The destructor stops it first.
- The effects tab's GUI-side instance calls `ensureLoaded()` before its
  panel is made (the panel reads the IR list).
- The order inside `ensureLoaded()` (list, then prepare) is the one used
  before; the other way round the effects' polling thread would mark the
  IR parameter handled while the list is still empty. In a session not at
  44.1 kHz the IR is therefore loaded twice when a slot first uses it (at
  the effects' 44.1 kHz default, then again at the session rate by
  `prepare`, since the fix of todo 14).

Remaining load cost: whatever Cab or Reverb slots are in use. A host that
renders offline right after setting the state, without a `prepareToPlay`
in between, could get a short silence at the start of such a slot.

Checked: no build run yet.

## Faster loading: no throwaway effects, effect panels made on demand (2026-10-05)

Load time was dominated by the convolution reverb: every instance decodes a
5 s, 96 kHz stereo IR, builds a convolution engine and starts a polling
thread as soon as it is created, and they were created far more often than
used.

- **No probe instances for the parameter layout.** `EffectSlot::addParameters`
  made an instance of every effect in every slot just to ask for its
  parameters (8 reverbs and 8 cabs, thrown away). `EffectTypeInfo` now has
  a static `addParameters`; the Cab and ConvolReverb adapters keep their IR
  tables as class members so they know the IR count without an instance.
  `Effect::addParametersToLayout` is gone. The parameter layout is the
  same as before (same IDs, same IR counts: 19 and 6).
- **Effect panels made when shown.** The Effects tab made a GUI-side effect
  and its panel for every effect in every slot (72, 8 of them reverbs) each
  time the editor opened. Now only the shown one exists; it is made when
  shown and dropped when another is. (Later the same day the panels were
  bound to the processor's instances instead of GUI-side copies: see
  above.)

Left as is: the 8 processor-side reverbs, one per slot whether used or not
(see todo 13 for the options).

Checked: no build run yet.

## 3, 4. Tooltips, "?" switch, info button (2026-10-05)

- `Source/Tooltips.h`: every hover-help string, one sub-namespace per pane
  (`mechanodd::tips::bar`, `global`, `source`, `resonator`, `matrix`,
  `effects`, `modulation`). No `setTooltip ("...")` literal anywhere else.
  Covered: the top bar (output, meter, gear), the gear's callout, every
  source and resonator control, every matrix knob (by column kind: source,
  resonator, Fx), the effects tab's selectors, show buttons, post master and
  send level, and every modulation row control.
- `Source/AppSettings.h`: `getUiTooltips` / `setUiTooltips`, saved in
  `FX-Mechanics/MechanOdd.settings` (Linux: `~/.config/FX-Mechanics`, not
  `~`).
- The editor's `TooltipWindow` asks the setting before each tip, so the
  switch covers the callouts too. The "?" (`fxme::AccentToggle`) is in the
  gear's callout, now with a "Global" title row (150 px high instead of
  120).
- `fxme::InfoButton` at the right end of the top bar, with an overview of
  the plugin (in `PluginEditor.cpp`).
- `OutputMeter` is a tooltip client; its bars let the mouse through.
- This settles the audit's R3: tips now appear everywhere, on by default.

Not covered: the small meters in the resonator slots and the matrix (todo
10), and the FxmeFX effect panels beyond their own tips (todo 11).

Checked: no build run yet.

## FX-Mechanics shell: top bar, preset folders, effect module presets (2026-10-05)

Brings the plugin to the FX-Mechanics shell (the FxMechanicsSuperSkill's
`plugin-shell.md`), first part.

**Top bar.** `fxme::TopBar` (logo, name, tagline, version and the
fx-mechanics.com link) replaces the bottom bar. Its right side carries, left
to right: the output level as a horizontal `FxmeSlider` (dB), the stereo
output meter (`OutputMeter.h`, the same two `HorizontalVuMeter`s as before,
stacked), the global preset bar with its "..." browser
(`setBrowserButtonVisible`), and a gear (`GlobalPanel.h`, after Dede's) that
opens Voices and Portamento in a callout. The preset strip that sat on the
tab row has moved to the top bar, and the Presets tab is gone (the browser
replaces it). The old L / S
buttons (raw XML through a file chooser) are gone: the browser covers loading
and saving. The default window is 654 px high instead of 700, so the tabs
keep their size.

**Global presets folder.** User presets move to
`FX-Mechanics/MechanOdd/Presets` (`getVendorPresetDirectory`). The old
`MechanOdd/Presets` is imported once on first run (`importLegacyUserPresets`:
copies, leaves a marker, the old folder stays as it was).

**Effect module presets (local presets).** Each of the nine FxmeFX effects a
slot can hold has its module presets, the same as FxmeFX's own effect plugins
(`Modules/<Effect>/Presets`, shared with them and with every other host):

- One `fxme::ModulePresetLibrary` per effect type, shared by all slots, and
  one `fxme::ModulePresetTarget` per effect per slot (2 chains x 4 slots x 9
  effects = 72), owned by the processor (`getEffectPresets (perTypePrefix)`).
  Scope `<chain>_fx<n>_<Type>_<Tag>_`; module names and tags are in
  `EffectFactory` (`moduleName`, `paramTag`).
- The Effects tab shows a preset bar with its browser above the shown
  effect's panel, for that effect in that slot.
- Factory presets come from FxmeFX (`fxmefx_add_module_presets` in
  CMakeLists.txt). Tube has none yet.

**Fixed on the way.** The reverb adapter did not pass the mid / side list to
`ConvolReverb::setImpulseList`, so Council Chamber and both Forest IRs were
convolved as left / right (loud, lopsided to the left). They are now decoded
like in FxmeFX's own plugin. This changes the sound of existing sessions and
presets that use those three IRs.

**Also.** `EDITOR_WANTS_KEYBOARD_FOCUS` is on and the editor owns a
`fxme::TextEntryFocusFixer` (audit R1), for the browsers' name fields and the
knobs' right-click value entry.

**Checked.** CMake configure only; not compiled or run yet.

**Changes for existing sessions.** Parameters and state are unchanged, so
sessions load as before (apart from the reverb fix above). Sessions now also
store each effect slot's current module preset name.

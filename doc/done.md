# MechanOdd: done

Implemented items, newest first. Items that came from [todo.md](todo.md) keep
their number there.

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
  shown and dropped when another is. The reverb's external IR lives in the
  plugin state (`EmbeddedAudio`), not in the instance, so nothing is lost
  when a panel is remade.

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

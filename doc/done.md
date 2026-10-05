# MechanOdd: done

Implemented items, newest first. Items that came from [todo.md](todo.md) keep
their number there.

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

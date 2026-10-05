# MechanOdd: todo

Numbered, scoped items. When one is done it moves to [done.md](done.md) with
its number. The compliance audit has its own checklist
([fxme-audit-2026-08-20.md](fxme-audit-2026-08-20.md)).

## FX-Mechanics shell, the rest

1. **Splash.** `fxme::SplashOverlay` with the fx-mechanics.com link: 3 s once
   per instance (`takeFirstSplash()` on the processor), until clicked from the
   top bar's logo (`topBar.onLogoClicked`). Placeholder art until there is
   some. Editor only. Medium priority.
2. **Plugin icon** in the top bar (`topBar.setDecoration`), a transparent
   PNG in `Source/Assets`. Needs the artwork. Low priority.
3. **Tooltips.** Every string in `Source/Tooltips.h`, a toggleable
   `TooltipWindow` in the editor, the "?" switch (in the gear's callout)
   saved machine-wide in `Source/AppSettings.h` (mind the Linux folder trap).
   This makes tips appear all over the editor, which is the audit's R3
   decision. Medium priority.
4. **InfoButton** in the top bar with a short help text. Low priority.
5. **The doc triplet**: `doc/architecture.md` is missing (signal flow,
   classes, parameters, threading, state). Medium priority.

## Presets

6. **ConvolReverb external IR in module presets.** FxmeFX's own plugin saves
   the external IR path in the preset's `<Extra>`
   (`onWriteExtra` / `onReadExtra`); MechanOdd's targets do not, so a
   reverb preset saved here holds only the built-in IR index. Needs the
   audio-thread `ConvolReverb` of each slot reachable from the processor
   (`EffectSlot` keeps it private). Low priority.
7. **Listen to the effect factory presets** in MechanOdd's context (they
   were drafted for FxmeFX by hand, see FxmeTools'
   `doc/local-presets-plan.md`, phase 4). Low priority.

## Code

8. **Promote `GearButton` to FxmeTools** (`components/`): Dede and MechanOdd
   now each have a copy. Touches FxmeTools (shared library; through the
   FxmeFX submodule here).
9. **Remove the unused `EffectChainComponent` / `EffectSlotComponent`**: no
   longer referenced by the editor (the Effects tab replaced them).

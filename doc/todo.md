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
   longer referenced by the editor (the Effects tab replaced them). They
   also no longer call `ensureLoaded()`, so their Cab and Reverb panels
   would show an empty IR list if they were ever used again.
10. **Tooltips for the meters.** The resonator slots' and the matrix's meters
    are FxmeFX's `VuMeterComponent` (`lib/FxmeFX/Source/VuMeter`), which is
    not a tooltip client, so they have no tip (the top bar's meter has one).
    Either a tooltip-client wrapper here or `SettableTooltipClient` on the
    FxmeFX class.
    Low priority.
11. **Tooltips in the FxmeFX effect panels** are partial (one to five per
    effect). They come from FxmeFX, so the fix belongs there, for every
    host at once. Low priority.

## Resonators

12. **IR resonator: a resonator from an impulse response file** (a body, a
    bell, a room, any recorded object). New feature; touches `Source/
    Resonators/` only, unless the IR analysis below goes to FxmeTools
    `core/`.

    Pitching an IR means time-scaling it: played at ratio r = target /
    base frequency, every mode moves by r and every decay time by 1 / r (a
    smaller or larger copy of the same object). Base Hz is the pitch the
    recorded IR is taken to have (set by the user; detecting it is
    unreliable for inharmonic bodies). Both requested modes are possible,
    at very different costs:

    - **Global, fixed pitch, with a pitch control.** One convolver
      (`fxme::FirFilter`, WDL, zero latency: needed, since any latency adds
      to the loop delay in the matrix). The pitch control resamples the IR
      off the audio thread and swaps it in. `FirFilter` as it is does that
      on the message thread and, while a load is in flight, passes blocks
      through unconvolved: here that would leak the raw excitation into the
      matrix for a few blocks. So two convolvers and a short crossfade (or
      a muted gap) around each swap. A pitch change is then a jump, not a
      glide: a setting, not something to modulate. Cheap and faithful (the
      whole IR, noise and diffuse tail included). The natural first step.
    - **Per voice, following the note.** Each voice needs the IR at its
      own ratio and its own convolver: 8 convolutions, and an IR per note.
      Resampling and re-partitioning an IR at note-on is too heavy for the
      audio thread, so the IRs would be prepared in advance (one per
      semitone over the playable range, a few MB), and continuous pitch
      (Fine, portamento, pitch modulation) would not follow. Workable
      for short IRs, awkward otherwise.
    - **Per voice, the better route: fit modes to the IR.** At load, off the
      audio thread, extract the IR's strongest modes (frequency, decay,
      amplitude; peak picking on the spectrum refined per peak, or a
      subspace method such as ESPRIT / matrix pencil) and drive the
      existing modal engine (`ModalResonator`'s biquad bank, up to 128
      modes) with them, frequencies scaled by the note. Continuous pitch,
      the same cost as a plate, and it gets the On / Off resonance pair
      and the note release for free (the measured decays become the On
      values, scaled). It loses what modes do not capture (noise, the
      diffuse part of a long tail), which matters little for objects and
      a lot for rooms. The analysis is JUCE-free and reusable (FxmeTools
      `core/`).

    Proposal: one resonator type, "IR", with a Mode switch: Convolution
    (global only, exact) and Modal (global or per voice, follows the note).
    The IR file travels with sessions and presets through
    `fxme::EmbeddedAudio`, never as a bare path.

## Load time

13. **Optional, for later: an algorithmic reverb in place of the IR
    reverb.** The convolution reverb is what makes MechanOdd slow to load
    (each instance decodes a 5 s, 96 kHz stereo IR, builds a convolution
    engine and starts a polling thread, and every slot holds one). A
    Freeverb-type reverb costs nothing to create and is closer to what the
    send chain needs. The kernel exists: `fxme::Reverb` (FxmeTools `core/`,
    the WDL Freeverb engine), as used by Dede.

    - **A new FxmeFX effect** (say `AlgoReverb`, tag `ARev`: room size,
      damping, width, pre-delay, dry, wet), with its own plugin, module
      presets and Pd external like the others, so other hosts get it too.
      Work in FxmeFX first, then here.
    - **Adding it is backward compatible** if it is appended at the end of
      `EffectFactory::types()`: the slot type parameter stores an index, so
      existing indices keep their meaning, and the new type has its own
      parameter IDs (`<slot>_<NewName>_ARev_*`), which clash with nothing.
      The convolution reverb stays and is simply no longer the obvious
      choice. Nothing to migrate.
    - **Removing the convolution reverb is what breaks things:** old sessions
      and presets with a Reverb slot would load an empty slot (or the
      wrong effect, if the list shifts), and host automation of its
      parameters would be lost. If it ever goes, keep its entry in the type
      list (renamed "Reverb (legacy)" or similar) so indices stay put, and
      convert on load: in `setStateInformation`, a state older than the
      change gets each legacy reverb slot switched to the new type with
      its settings mapped approximately (Length to room size, Dry / Wet
      gains to dry / wet). That needs a version attribute on the state,
      which MechanOdd does not have yet (audit H1): add it now, before
      anything needs it, since a state saved without it can only ever be
      guessed at. The factory presets in `Source/Assets` that use the
      reverb would be regenerated.
    - **The cheaper alternative**, loading a slot's IR only once the slot is
      set to Reverb or Cab, is done (see done.md), so unused slots no
      longer cost anything. What remains is the cost of a reverb actually
      in use.

    Order: H1 now (small), then the FxmeFX effect, then decide whether the
    convolution reverb is kept or retired.

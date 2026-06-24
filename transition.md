# MechanOdd — library reorganisation transition plan

Goal: make **FxmeFX the single library dependency** of MechanOdd (besides JUCE
itself). FxmeFX already bundles FxmeTools as a submodule, and FxmeTools bundles
WDL. So the target dependency chain is:

```
MechanOdd ──▶ FxmeFX ──▶ FxmeTools ──▶ WDL
                 (effects)   (GUI/DSP/look&feel)   (convolution engine)
```

After the transition MechanOdd must **no longer depend on**:
- `FxmeJuceTools` (the JUCE user-module currently installed under
  `../JUCE/usermodules/FxmeJuceTools`), and
- the **top-level `lib/FxmeTools` submodule** (added by mistake — it is a
  byte-for-byte duplicate of `lib/FxmeFX/lib/FxmeTools`).

---

## 1. Findings (current state)

### 1.1 What MechanOdd actually uses from FxmeJuceTools

A full scan of `Source/` shows MechanOdd depends on only a handful of symbols
from the `FxmeJuceTools` module:

| Symbol | Uses | Already in FxmeTools? |
|---|---|---|
| `fxme::FxmeSlider` | 33 | ✅ `FxmeTools/components/FxmeSlider.h` (identical API, `namespace fxme`) |
| `fxme::FxmeLookAndFeel` | ~13 | ✅ `FxmeTools/lookandfeels/FxmeLookAndFeel.h` (identical drawing code) |
| `fxme::HorizontalVuMeter` | 2 | ✅ `FxmeTools/components/FxmeMeters.h` (identical API) |
| `CracksGenerator` | 3 | ❌ **missing — must be ported** |

`CracksGenerator` is included directly (`Source/Sources/CracksSource.h` →
`#include "CracksGenerator.h"`) and its `.cpp` is compiled into MechanOdd via an
explicit rule in `CMakeLists.txt` (lines 105–108), pulling from
`../JUCE/usermodules/FxmeJuceTools/Dsp/`.

**Conclusion:** the GUI components are already in FxmeTools. The *only* missing
piece is `CracksGenerator` (≈45 lines, header + cpp, no external deps).

### 1.2 The LFO / ADSR question — important correction

The request assumed the LFO/ADSR "functions and components" live in **FxmeFX**.
They do **not**. Searches of `lib/FxmeFX/` find no LFO/ADSR/envelope classes
(only compressor/limiter envelope *followers*, unrelated).

The LFO + ADSR modulation system lives **inside MechanOdd itself**, in
`Source/Modulation/`:

- `ModEngine.{h,cpp}` — global modulator bank (LFO shapes + `juce::ADSR`)
- `VoiceModEngine.{h,cpp}` — per-voice variant
- `ModulationComponent.{h,cpp}` — the GUI tab
- `ParamSource.h` — per-voice APVTS shadowing

This system is **tightly coupled to MechanOdd's architecture**: APVTS parameter
ids, per-voice override atomics (`ParamSource`), target naming conventions
(`src*` / `res*` / `mtx_*` → `isPerVoiceTarget`), the feedback matrix, and
MechanOdd's "overwrite the raw-value atomic each block" modulation strategy. It
is *not* a clean lift-and-shift.

What is genuinely generic and reusable is a small **LFO kernel**:
- the `Shape` enum (`sine/triangle/square/sawUp/sawDown`),
- `evalLfo(shape, phase)`,
- `syncRateBeats(index)` + the tempo-sync rate table,
- phase advance.

ADSR itself is already `juce::ADSR` — nothing to move.

**Recommendation:** treat LFO/ADSR promotion as *optional* and *scoped*: extract
only the LFO kernel into `FxmeTools/dsp/Lfo.h` (see step 2.B). Leave the
orchestration (`ModEngine`, `VoiceModEngine`, `ParamSource`, GUI) in MechanOdd.
See **Open question Q1**.

### 1.3 Submodule / WDL layout discovered

```
Mechanodd/
├─ .gitmodules
│    ├─ lib/FxmeFX     → github.com/odoare/FxmeFX        (KEEP)
│    └─ lib/FxmeTools  → github.com/odoare/FxmeTools     (REMOVE — duplicate)
├─ lib/FxmeFX/
│    ├─ WDL/WDL/                         ← STALE leftover WDL working tree
│    │                                     (no longer in FxmeFX/.gitmodules;
│    │                                      MechanOdd's CMake still points here)
│    ├─ lib/FxmeTools/                   ← the real FxmeTools (submodule @ f8b584b)
│    │    ├─ cmake/FxmeTools.cmake       ← registers module + fxmetools_attach()
│    │    ├─ FxmeTools/  (the JUCE module: components/ dsp/ lookandfeels/)
│    │    └─ WDL/WDL/                    ← canonical WDL (FxmeTools' submodule)
│    └─ Source/<Effect>/ …
└─ lib/FxmeTools/                        ← DUPLICATE submodule @ f8b584b (REMOVE)
```

- Both `lib/FxmeTools` and `lib/FxmeFX/lib/FxmeTools` are pinned to the **same
  commit `f8b584b`** — confirmed duplicate.
- `lib/FxmeFX/WDL/WDL` is a **stale** copy. MechanOdd's `CMakeLists.txt`
  (lines 97–103) still compiles `convoengine.cpp/fft.c/resample.cpp` from there.
  The canonical WDL is now reached through FxmeTools via `fxmetools_attach()`,
  which compiles those same three files from `lib/FxmeFX/lib/FxmeTools/WDL/WDL`.

### 1.4 How FxmeTools is meant to be consumed

`lib/FxmeFX/lib/FxmeTools/cmake/FxmeTools.cmake` provides:
- `juce_add_module(<FxmeTools module>)` → registers the `FxmeTools` target;
- `fxmetools_attach(<target>)` → links `FxmeTools` **and** compiles the WDL
  convolution engine + adds its include dir + sets `NOMINMAX` on MSVC.

FxmeFX's own effect `CMakeLists.txt` files already use exactly this pattern
(`include(.../lib/FxmeTools/cmake/FxmeTools.cmake)` + link `FxmeTools`). The
reused effect components include FxmeTools by path, e.g.
`#include <FxmeTools/dsp/VuMeter.h>`, `<FxmeTools/components/VuMeterComponent.h>`,
`<FxmeTools/dsp/Biquad.h>` — so MechanOdd **must** have the `FxmeTools` module on
its include path once it compiles those effect sources (it gets this for free
from `juce_add_module(FxmeTools)`).

### 1.5 Namespace note

- MechanOdd source already qualifies the GUI controls as `fxme::…` and reaches
  them through `<JuceHeader.h>`. Because `FxmeTools` is a JUCE module,
  `juce_generate_juce_header(MechanOdd)` will include it in `JuceHeader.h`
  automatically once it is linked — so **no include changes are needed** in
  MechanOdd source for the GUI controls.
- `CracksGenerator` is currently in the **global** namespace. When ported into
  FxmeTools it should be wrapped in `namespace fxme` for consistency, which means
  updating its 3 usages in `Source/Sources/CracksSource.{h,cpp}` to
  `fxme::CracksGenerator` (and changing the include to
  `<FxmeTools/dsp/CracksGenerator.h>`).

---

## 2. Transition steps

> Two repos are touched: **FxmeTools** (inside the FxmeFX submodule) and
> **MechanOdd**. Changes to FxmeTools must be committed/pushed in the FxmeTools
> repo, then the FxmeFX submodule pointer bumped, then MechanOdd's FxmeFX pointer
> bumped. See step 6 for the commit/push ordering.

### Step A — Port `CracksGenerator` into FxmeTools  *(FxmeTools repo)*

Work in `lib/FxmeFX/lib/FxmeTools/`.

1. Create `FxmeTools/dsp/CracksGenerator.h`, copied from
   `../JUCE/usermodules/FxmeJuceTools/Dsp/CracksGenerator.h`, with these edits:
   - remove `#include <JuceHeader.h>` (module headers must not include it; the
     umbrella `FxmeTools.h` already includes `juce_core`/`juce_dsp` for
     `juce::Random` and `juce::dsp::ProcessSpec`);
   - wrap the class in `namespace fxme { … }` (matching the other dsp headers).

2. Create `FxmeTools/dsp/CracksGenerator.cpp`, copied from the FxmeJuceTools
   `.cpp`, with `#include "CracksGenerator.h"` and the body wrapped in
   `namespace fxme { … }` (matching `SpectrumAnalyzer.cpp`).

3. Register them in the module:
   - in `FxmeTools/FxmeTools.h`, add under the DSP section:
     `#include "dsp/CracksGenerator.h"`
   - in `FxmeTools/FxmeTools.cpp`, add:
     `#include "dsp/CracksGenerator.cpp"`

4. (Optional consistency) bump the module `version:` in `FxmeTools.h`.

No CMake change is needed in FxmeTools — module `.cpp`s are aggregated through
`FxmeTools.cpp`.

### Step B — *(Optional)* Extract the LFO kernel into FxmeTools  *(FxmeTools repo)*

Only if you want the LFO reusable across projects (see Q1). Scope it small:

1. Create `FxmeTools/dsp/Lfo.h` in `namespace fxme` containing:
   - `enum Shape { sine, triangle, square, sawUp, sawDown };`
   - `static float eval (Shape, float phase)` (body from `ModEngine::evalLfo`);
   - `static float syncRateBeats (int index)` + the rate table
     (from `ModEngine::syncRateBeats` / `syncRateChoices`);
   - optionally a tiny stateful `class Lfo { float phase; advance(...); }`.
2. Add `#include "dsp/Lfo.h"` to `FxmeTools.h`.
3. In MechanOdd, refactor `ModEngine`/`VoiceModEngine` to call
   `fxme::Lfo::eval(...)` / `fxme::Lfo::syncRateBeats(...)` instead of their
   private statics. Keep all the orchestration in MechanOdd.

ADSR stays `juce::ADSR` — nothing to extract.

**If you prefer to defer this**, skip Step B entirely; it is independent of the
core goal (dropping FxmeJuceTools + the duplicate submodule).

### Step C — Rewire MechanOdd's CMakeLists.txt  *(MechanOdd repo)*

Edit `/home/doare/src/Mechanodd/CMakeLists.txt`:

1. **Replace the FxmeJuceTools module registration.** Remove:
   ```cmake
   juce_add_module(../JUCE/usermodules/FxmeJuceTools)
   ```
   and add (after `add_subdirectory(../JUCE JUCE)`):
   ```cmake
   include(${CMAKE_CURRENT_SOURCE_DIR}/lib/FxmeFX/lib/FxmeTools/cmake/FxmeTools.cmake)
   ```
   This registers the `FxmeTools` module and the `fxmetools_attach()` helper.

2. **Attach FxmeTools to the target** (gives the module + WDL convolution
   engine + include dirs + MSVC `NOMINMAX`). After `juce_add_plugin(...)` /
   `target_sources(...)`:
   ```cmake
   fxmetools_attach(MechanOdd)
   ```

3. **Delete the manual WDL block** (lines ~97–103) that compiles from the stale
   `lib/FxmeFX/WDL/WDL` — `fxmetools_attach()` now provides WDL from
   `lib/FxmeFX/lib/FxmeTools/WDL/WDL`.

4. **Delete the FxmeJuceTools DSP block** (lines ~105–108) that pulled in
   `CracksGenerator.cpp` from `../JUCE/usermodules/FxmeJuceTools/Dsp`. It is now
   part of the FxmeTools module.

5. **Update `target_link_libraries`:** replace `FxmeJuceTools` with `FxmeTools`
   (or rely on `fxmetools_attach` having linked it — then just remove the
   `FxmeJuceTools` line). Keep all the `juce::…` entries.

6. The FxmeFX effect-reuse block (`FXMEFX_EFFECTS`, lines ~60–95) **stays as
   is** — it globs effect `.cpp`/components and assets directly and does not
   depend on FxmeJuceTools. (Those effect components include `<FxmeTools/…>`,
   which now resolves because the FxmeTools module is on the include path.)

### Step D — Fix MechanOdd source for `CracksGenerator`  *(MechanOdd repo)*

In `Source/Sources/CracksSource.h`:
- change `#include "CracksGenerator.h"` →
  `#include <FxmeTools/dsp/CracksGenerator.h>`
- change the member type `CracksGenerator generator;` →
  `fxme::CracksGenerator generator;`

In `Source/Sources/CracksSource.cpp`: qualify any remaining `CracksGenerator`
references with `fxme::` (the 3 total usages are here and in the header).

No other MechanOdd source changes are expected — `fxme::FxmeSlider`,
`fxme::FxmeLookAndFeel`, `fxme::HorizontalVuMeter` resolve unchanged via
`JuceHeader.h`.

### Step E — Remove the duplicate top-level `lib/FxmeTools` submodule  *(MechanOdd repo)*

```bash
cd /home/doare/src/Mechanodd
git submodule deinit -f lib/FxmeTools
git rm -f lib/FxmeTools
rm -rf .git/modules/lib/FxmeTools
```

`git rm -f` also drops the `[submodule "lib/FxmeTools"]` stanza from
`.gitmodules`. Verify `.gitmodules` afterwards contains only the `lib/FxmeFX`
entry, and `git submodule status` lists only `lib/FxmeFX`.

### Step F — Update CI (`.github/workflows/release.yml`)  *(MechanOdd repo)*

The workflow has **three jobs** (Linux/macOS/Windows) that each:
- check out with `submodules: recursive` (✅ keep — now pulls FxmeFX→FxmeTools→WDL),
- **separately check out `odoare/FxmeJuceTools`** and `mv` it into
  `JUCE/usermodules/FxmeJuceTools` (steps "Checkout FxmeJuceTools" /
  "Install FxmeJuceTools module", lines ~35–45, ~83–93, ~135–145).

Remove the "Checkout FxmeJuceTools" and "Install FxmeJuceTools module" steps
from **all three** jobs. `submodules: recursive` already provides everything.
Confirm no other step references `usermodules` or `FxmeJuceTools`.

### Step G — *(Optional)* Update the Projucer file

`Mechanodd.jucer` still lists JUCE modules but **not** FxmeJuceTools, and the
build is CMake-driven. If the `.jucer` is no longer used it can be left alone or
deleted. If it is still used, regenerate/adjust module paths to point at the
FxmeTools module instead. **Recommend:** treat CMake as source of truth; leave
`.jucer` out of scope (note in PR).

---

## 3. Build & verification

After steps A–F:

```bash
cd /home/doare/src/Mechanodd
rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Checklist:
- [ ] CMake configures with no `FxmeJuceTools` / `usermodules` references.
- [ ] `FxmeTools` module is registered exactly once.
- [ ] Cracks source compiles and the crackle generator works at runtime.
- [ ] Convolution-based effects (Cab, ConvolReverb) still build & load (WDL now
      via `fxmetools_attach`).
- [ ] GUI renders identically (sliders, look-and-feel, VU meter).
- [ ] Standalone + VST3 (+ AU on macOS) build and load in a host.
- [ ] `git submodule status` shows only `lib/FxmeFX`.

---

## 4. Risks & gotchas

- **Two `FxmeTools.cmake` includes.** MechanOdd must include FxmeTools.cmake
  itself (it never `add_subdirectory`s FxmeFX). Don't also let any FxmeFX CMake
  run, or `juce_add_module(FxmeTools)` could be called twice (target redefinition).
  MechanOdd only globs FxmeFX *sources*, so this is fine — just don't add
  `add_subdirectory(lib/FxmeFX)`.
- **Stale `lib/FxmeFX/WDL`.** Left untouched it's harmless but confusing; the
  real fix belongs in the FxmeFX repo (remove the stale WDL working tree there).
  Out of scope for MechanOdd but worth a follow-up issue.
- **CracksGenerator namespace change** is a breaking source change — make sure
  all 3 usages are updated, or compilation fails fast (acceptable).
- **`juce::dsp::ProcessSpec`** in CracksGenerator.h requires `juce_dsp`;
  FxmeTools' module declares `juce_dsp` as a dependency and the umbrella includes
  it, so this is satisfied.

---

## 5. Open questions

- **Q1 — LFO/ADSR promotion (Step B):** do you want the LFO kernel extracted to
  FxmeTools now, or deferred? (The full modulation system is MechanOdd-specific
  and should stay; only the small LFO shape/sync kernel is worth sharing.)

  Response : Yes, do the LFO shape/sync sharing.

- **Q2 — `.jucer`:** still in use, or can it be dropped/ignored (Step G)?

Response : Drop .jucer file

- **Q3 — Stale `lib/FxmeFX/WDL`:** open a follow-up to clean it in the FxmeFX
  repo, or leave it?

  I think WDL can be safely removed from lib/FxmeFX since it is already in FxmeTools

---

## 6. Commit / push ordering (because of nested submodules)

1. **FxmeTools repo** (`lib/FxmeFX/lib/FxmeTools`): commit Step A (+ B) → push.
2. **FxmeFX repo** (`lib/FxmeFX`): bump the FxmeTools submodule pointer to the
   new commit → commit → push.
3. **MechanOdd repo**: bump the `lib/FxmeFX` submodule pointer, apply steps
   C–F, remove the duplicate submodule → commit.

If you are not ready to push the library repos, you can validate everything
locally first (the working trees are already at the needed state); just remember
the submodule pointer bumps before CI will pass.

---

## 7. Summary of files touched

**FxmeTools repo**
- `FxmeTools/dsp/CracksGenerator.h` (new)
- `FxmeTools/dsp/CracksGenerator.cpp` (new)
- `FxmeTools/dsp/Lfo.h` (new, optional — Step B)
- `FxmeTools/FxmeTools.h` (add includes)
- `FxmeTools/FxmeTools.cpp` (add include)

**MechanOdd repo**
- `CMakeLists.txt` (FxmeTools.cmake include + `fxmetools_attach`, drop
  FxmeJuceTools/WDL blocks, fix link libs)
- `Source/Sources/CracksSource.h` / `.cpp` (include + `fxme::` qualification)
- `Source/Modulation/ModEngine.*`, `VoiceModEngine.*` (only if Step B)
- `.github/workflows/release.yml` (remove FxmeJuceTools steps ×3)
- `.gitmodules` (drop `lib/FxmeTools` — via `git rm`)
- remove submodule `lib/FxmeTools`
</content>
</invoke>

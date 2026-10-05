/*
  ==============================================================================

    Tooltips.h

    Every hover-help string in the plugin, in one place. One sub-namespace
    per pane. The "?" button of the global settings (the gear) turns them all
    on and off (see AppSettings.h: uiTooltips).

    The FxmeFX effects' own panels and the preset bars carry their own tips
    (in FxmeFX and FxmeTools).

  ==============================================================================
*/

#pragma once

namespace mechanodd::tips
{

//==============================================================================
/** The top bar (PluginEditor). */
namespace bar
{
    inline constexpr auto output = "Output: the final level, after the master effects. "
                                   "Right-click to type a value.";
    inline constexpr auto meter  = "Output level, left over right: peak, held peak, red over 0 dBFS.";
    inline constexpr auto global = "Global settings: voices and portamento. "
                                   "Its \"?\" turns these tips on and off.";
}

//==============================================================================
/** The gear's callout (GlobalPanel). */
namespace global
{
    inline constexpr auto voices     = "Voices: how many notes can sound at once (1 to 8). "
                                       "Fewer voices cost less CPU.";
    inline constexpr auto portamento = "Portamento: glide time from one note's pitch to the next, "
                                       "0 for none.";
    inline constexpr auto tooltips   = "Hover help on the controls: on or off (for every instance).";
}

//==============================================================================
/** A source slot (SourceSlotComponent and the source-type components). */
namespace source
{
    inline constexpr auto type = "Source type: Noise, Wavetable, Cracks, the plugin's audio input "
                                 "(left or right), or Off. Sources feed the matrix's S columns.";

    inline constexpr auto attack    = "Attack: the envelope's rise time after note-on.";
    inline constexpr auto decay     = "Decay: time to fall from the peak to the sustain level.";
    inline constexpr auto sustain   = "Sustain: the level held while the note is down.";
    inline constexpr auto release   = "Release: fade time after note-off.";
    inline constexpr auto cutoff    = "Cutoff: the source's low-pass filter.";
    inline constexpr auto resonance = "Resonance: the low-pass filter's peak at the cutoff "
                                      "(0.71 is flat).";
    inline constexpr auto level     = "Level: the source's output, in dB.";
    inline constexpr auto velLevel  = "Velocity to level: how much softer notes play quieter "
                                      "(0 = velocity ignored).";
    inline constexpr auto velCutoff = "Velocity to cutoff: how much softer notes play darker "
                                      "(0 = velocity ignored).";

    inline constexpr auto density = "Density: crackle events per second.";
    inline constexpr auto wave    = "Wave: the stored waveform the oscillator reads.";
    inline constexpr auto mode    = "Mode: One-shot plays the wave once per note (an excitation); "
                                    "Loop repeats it (a sustained tone).";
    inline constexpr auto tune    = "Tune: pitch offset in semitones (plus or minus two octaves).";
}

//==============================================================================
/** A resonator slot (ResonatorSlotComponent and the resonator-type components). */
namespace resonator
{
    inline constexpr auto type   = "Resonator type: the physical object the sources excite "
                                   "(string, plate, membrane, string / beam), Passthrough or Block.";
    inline constexpr auto global = "Global: one instance shared by all voices at the Base Hz pitch "
                                   "(a body or room) instead of one per note. Global resonators "
                                   "always use their On settings.";
    inline constexpr auto baseHz = "Base Hz: the pitch of a global resonator (per-note resonators "
                                   "follow the note).";
    inline constexpr auto level  = "Level: the resonator's output into the mix and the send "
                                   "(the matrix's Lvl and Snd scale it further).";

    inline constexpr auto coarse = "Coarse: pitch offset in semitones.";
    inline constexpr auto fine   = "Fine: pitch offset within a semitone.";

    inline constexpr auto noteRel = "Note release: how long the resonator takes to move from its "
                                    "On settings to its Off settings after note-off.";

    // Waveguide string.
    inline constexpr auto fbOn       = "Feedback, note on: loop gain while the note is held; "
                                       "closer to 1 rings longer.";
    inline constexpr auto fbOff      = "Feedback, note off: loop gain after the release (lower damps "
                                       "the string).";
    inline constexpr auto cutOn      = "Loop cutoff, note on: the bridge's low-pass while held; lower "
                                       "makes high partials die sooner.";
    inline constexpr auto cutOff     = "Loop cutoff, note off: the bridge's low-pass after the release.";
    inline constexpr auto inPos      = "Input position: where along the length the excitation enters "
                                       "(near an end: brighter).";
    inline constexpr auto outPos     = "Output position: where along the length the sound is picked up.";
    inline constexpr auto dispersion = "Dispersion: stiffness. 0 is an ideal string; higher stretches "
                                       "the overtones (inharmonic, piano-like).";

    // Modal (plate, membrane, string / beam).
    inline constexpr auto modes       = "Modes: how many partials are computed. More is richer and "
                                        "costs more CPU.";
    inline constexpr auto resOn       = "Resonance, note on: how long the modes ring while the note is "
                                        "held (dB scale; 100 rings almost forever).";
    inline constexpr auto resOff      = "Resonance, note off: how long they ring after the release.";
    inline constexpr auto resSlopeOn  = "Resonance slope, note on: how much faster the high modes decay "
                                        "than the low ones while held (lower: high modes die sooner).";
    inline constexpr auto resSlopeOff = "Resonance slope, note off: the same after the release.";
    inline constexpr auto aspect      = "Aspect: the ratio of the two sides; changes how the modes "
                                        "spread.";
    inline constexpr auto inX         = "Input X: where the excitation strikes, across the width.";
    inline constexpr auto inY         = "Input Y: where the excitation strikes, across the height.";
    inline constexpr auto outX        = "Output X: where the sound is picked up, across the width.";
    inline constexpr auto outY        = "Output Y: where the sound is picked up, across the height.";
    inline constexpr auto mix         = "String / beam: from harmonic string modes (0) to the stretched "
                                        "modes of a stiff bar (1), marimba-like.";
}

//==============================================================================
/** The feedback matrix (FeedbackMatrixComponent). Rows feed the resonators. */
namespace matrix
{
    inline constexpr auto sourceCell    = "How much of this source drives this row's resonator. "
                                          "Centre is off; the sign flips the phase.";
    inline constexpr auto resonatorCell = "Feedback from that resonator into this row's resonator "
                                          "(one block later). Centre is off; the sign flips the phase. "
                                          "Raise slowly: loops can run away.";
    inline constexpr auto busCell       = "Feedback from the send effects (Fx) into this row's "
                                          "resonator. Centre is off; the sign flips the phase.";
    inline constexpr auto level         = "Level: this resonator in the main mix.";
    inline constexpr auto pan           = "Pan: this resonator's place in the stereo mix and the send.";
    inline constexpr auto send          = "Send: this resonator into the send effects chain.";
}

//==============================================================================
/** The effects tab (EffectsTabComponent). */
namespace effects
{
    inline constexpr auto sendSlot   = "Send chain slot: the effect in this place of the send chain "
                                       "(runs on what the matrix's Snd knobs send; its output also "
                                       "feeds the matrix's Fx column).";
    inline constexpr auto masterSlot = "Master chain slot: the effect in this place of the master "
                                       "chain (runs on the whole mix).";
    inline constexpr auto show       = "Show this slot's effect on the right.";
    inline constexpr auto postMaster = "Post master: run the send chain after the master chain "
                                       "rather than before it.";
    inline constexpr auto sendLevel  = "Send level: the send chain's output into the mix.";
}

//==============================================================================
/** The modulation tab (ModulationComponent). */
namespace modulation
{
    inline constexpr auto type     = "Type: LFO (a continuous wave) or ADSR (an envelope from the notes).";
    inline constexpr auto module   = "Target module: the part of the synth the modulator acts on.";
    inline constexpr auto param    = "Target parameter, within the module (only the active "
                                     "type's parameters are listed).";
    inline constexpr auto shape    = "LFO shape.";
    inline constexpr auto rate     = "LFO rate in Hz (when not synced).";
    inline constexpr auto sync     = "Sync: lock the LFO's rate to the host tempo.";
    inline constexpr auto syncRate = "LFO rate as a note length (when synced).";
    inline constexpr auto depth    = "Depth: how far the LFO moves the target, either way "
                                     "(centre = none; 1 = the whole range).";
    inline constexpr auto attack   = "Envelope attack time.";
    inline constexpr auto decay    = "Envelope decay time.";
    inline constexpr auto sustain  = "Envelope sustain level.";
    inline constexpr auto release  = "Envelope release time.";
    inline constexpr auto amount   = "Amount: how far the envelope moves the target.";
    inline constexpr auto polarity = "Polarity: the envelope pushes the target up (+) or down (-).";
}

} // namespace mechanodd::tips

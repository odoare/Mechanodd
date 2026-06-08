/*
  ==============================================================================

    FeedbackMatrix.h

    Routing / mixing core. Columns are the feedback sources: the N source slots
    followed by the M resonator slots. Rows are the M resonators that receive a
    weighted sum of all columns. Self-feedback cells do not exist. Three extra
    per-row controls set each resonator's output level, pan and send level.

    Each resonator slot is either per-voice (PV) or global (G), chosen by its
    "_global" parameter. The matrix runs in two roles that share the same gains
    and mask:

      processVoice   - run once per voice. Processes the PV rows sample-accurately
                       with a one-sample feedback delay. Sources and PV-resonator
                       columns are local to the voice; G-resonator columns are read
                       from the previous block's global output (broadcast). PV
                       outputs are mixed to the voice output and also accumulated
                       into a shared column-sum buffer for the global rows.

      processGlobal  - run once in the processor after all voices. Processes the G
                       rows. Source and PV-resonator columns come from the shared
                       column-sum (this block, summed over voices); G-resonator
                       columns use the one-sample-delayed global feedback. G outputs
                       are mixed to the main output.

    One instance lives per voice and one for the global rows; the feedback delay
    state is therefore per role.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "ResonatorSlot.h"

class FeedbackMatrix
{
public:
    static constexpr int numSources      = 4;                    // == SynthVoice::numSourceSlots
    static constexpr int numResonators   = ResonatorSlot::numSlots;
    static constexpr int numBusFeedback  = 1;                    // processed send-bus output (prev block)
    static constexpr int busFeedbackCol  = numSources + numResonators;   // index of the bus column
    static constexpr int numColumns      = numSources + numResonators + numBusFeedback;
    static constexpr int numRows         = numResonators;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void assignParameters (ParamSource& apvts);
    void checkParameters();

    bool isGlobal (int slot) const { return globalMask[(size_t) slot]; }

    void processVoice (const float* const* sourceSamples,
                       std::array<ResonatorSlot, numResonators>& resonators,
                       const float* const* globalPrev,
                       const float* busFeedback,
                       float* outL, float* outR, float* sendL, float* sendR,
                       float* const* columnSum,
                       int numSamples);

    void processGlobal (const float* const* columnSum,
                        std::array<ResonatorSlot, numResonators>& resonators,
                        const float* busFeedback,
                        float* outL, float* outR, float* sendL, float* sendR,
                        float* const* globalOut,
                        int numSamples);

    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params);

    // Routing-gain cells are a bipolar, centre-mute control: the stored value v in
    // [-1, 1] maps to a linear-in-dB magnitude (|v| -> gainMinDb..gainMaxDb, with
    // gainMinDb treated as -inf so the centre is a true zero) and v's sign sets the
    // feedback polarity. Conversion lives here so the parameter readout and the DSP
    // agree.
    static constexpr float gainMinDb = -60.0f;   // centre (|v|->0): -inf / true mute
    static constexpr float gainMaxDb =   6.0f;   // edges (|v|->1): full level (~2x)

    static float gainFromParam (float v) noexcept
    {
        const float mag = std::abs (v);
        const float dB  = gainMinDb + mag * (gainMaxDb - gainMinDb);
        const float g   = juce::Decibels::decibelsToGain (dB, gainMinDb);   // 0 at the floor
        return v < 0.0f ? -g : g;
    }

    // Full-scale linear gain (|v| = 1, i.e. gainMaxDb). Modulation is scaled against
    // this so a full-depth modulator can swing the whole linear range.
    static float gainMaxLinear() noexcept { return gainFromParam (1.0f); }

    // Combine a gain cell's unmodulated knob (base01, normalised) with its modulated
    // value (mod01, normalised) so that the modulation acts on the LINEAR gain and the
    // phase (sign) stays locked to the knob: the magnitude is offset linearly and
    // clamped at 0, so a modulator never drags the cell through the centre and flips
    // its sign. With no modulation (mod01 == base01) this is exactly gainFromParam.
    static float modulatedGain (float base01, float mod01) noexcept
    {
        const float baseV = base01 * 2.0f - 1.0f;                       // range is [-1, 1]
        const float sign  = (baseV < 0.0f) ? -1.0f : 1.0f;
        const float off   = (mod01 - base01) * gainMaxLinear();         // normalised -> linear
        const float mag   = juce::jlimit (0.0f, gainMaxLinear(),
                                          std::abs (gainFromParam (baseV)) + off);
        return sign * mag;
    }

    static bool isSelfCell (int row, int col) { return col == numSources + row; }
    static juce::String gainId  (int row, int col) { return "mtx_g_" + juce::String (row) + "_" + juce::String (col); }
    static juce::String levelId (int row) { return "mtx_level_" + juce::String (row); }
    static juce::String panId   (int row) { return "mtx_pan_"   + juce::String (row); }
    static juce::String sendId  (int row) { return "mtx_send_"  + juce::String (row); }

private:
    void mixRow (int r, float out, int n, float* outL, float* outR, float* sendL, float* sendR);

    // Per-sample linear smoothers: the routing gains, per-row output level, pan and
    // send are all applied as per-sample multipliers in the process loops, so ramping
    // them removes zipper on parameter / modulation changes (feedback-gain steps are
    // the most audible). Coefficient-driven resonator params are smoothed elsewhere.
    using SmoothedGain = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>;
    std::array<std::array<SmoothedGain, numColumns>, numRows> gains {};
    std::array<SmoothedGain, numRows> level {};
    std::array<SmoothedGain, numRows> panL  {};
    std::array<SmoothedGain, numRows> panR  {};
    std::array<SmoothedGain, numRows> send  {};
    std::array<bool,  numResonators> globalMask {};

    double sampleRate { 44100.0 };
    bool   smoothPrimed { false };   // first checkParameters snaps; later ones ramp

    std::array<float, numResonators> prevResonatorOut {};

    std::array<std::array<std::atomic<float>*, numColumns>, numRows> gainParam {};
    // The base (unmodulated) gain parameters: getValue() is unaffected by the engines'
    // atomic overwrites, so it gives the knob's true position for sign + base magnitude.
    std::array<std::array<juce::RangedAudioParameter*, numColumns>, numRows> gainParamObj {};
    std::array<std::atomic<float>*, numRows> levelParam {};
    std::array<std::atomic<float>*, numRows> panParam   {};
    std::array<std::atomic<float>*, numRows> sendParam  {};
    std::array<std::atomic<float>*, numResonators> globalParam {};
};

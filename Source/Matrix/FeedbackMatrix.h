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
    static constexpr int numSources    = 3;                    // == SynthVoice::numSourceSlots
    static constexpr int numResonators = ResonatorSlot::numSlots;
    static constexpr int numColumns    = numSources + numResonators;
    static constexpr int numRows        = numResonators;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void assignParameters (juce::AudioProcessorValueTreeState& apvts);
    void checkParameters();

    bool isGlobal (int slot) const { return globalMask[(size_t) slot]; }

    void processVoice (const float* const* sourceSamples,
                       std::array<ResonatorSlot, numResonators>& resonators,
                       const float* const* globalPrev,
                       float* outL, float* outR, float* sendL, float* sendR,
                       float* const* columnSum,
                       int numSamples);

    void processGlobal (const float* const* columnSum,
                        std::array<ResonatorSlot, numResonators>& resonators,
                        float* outL, float* outR, float* sendL, float* sendR,
                        float* const* globalOut,
                        int numSamples);

    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params);

    static bool isSelfCell (int row, int col) { return col == numSources + row; }
    static juce::String gainId  (int row, int col) { return "mtx_g_" + juce::String (row) + "_" + juce::String (col); }
    static juce::String levelId (int row) { return "mtx_level_" + juce::String (row); }
    static juce::String panId   (int row) { return "mtx_pan_"   + juce::String (row); }
    static juce::String sendId  (int row) { return "mtx_send_"  + juce::String (row); }

private:
    void mixRow (int r, float out, int n, float* outL, float* outR, float* sendL, float* sendR);

    std::array<std::array<float, numColumns>, numRows> gains {};
    std::array<float, numRows> level {};
    std::array<float, numRows> panL  {};
    std::array<float, numRows> panR  {};
    std::array<float, numRows> send  {};
    std::array<bool,  numResonators> globalMask {};

    std::array<float, numResonators> prevResonatorOut {};

    std::array<std::array<std::atomic<float>*, numColumns>, numRows> gainParam {};
    std::array<std::atomic<float>*, numRows> levelParam {};
    std::array<std::atomic<float>*, numRows> panParam   {};
    std::array<std::atomic<float>*, numRows> sendParam  {};
    std::array<std::atomic<float>*, numResonators> globalParam {};
};

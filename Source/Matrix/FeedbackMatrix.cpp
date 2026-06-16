/*
  ==============================================================================

    FeedbackMatrix.cpp

  ==============================================================================
*/

#include "FeedbackMatrix.h"
#include "../Modulation/ParamSource.h"
#include "SynthVoice.h"

static_assert (FeedbackMatrix::numSources == SynthVoice::numSourceSlots,
               "FeedbackMatrix::numSources must match SynthVoice::numSourceSlots");

void FeedbackMatrix::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate   = spec.sampleRate;
    smoothPrimed = false;

    // DC-blocker pole from the corner frequency: R = 1 - 2*pi*fc/fs (one-pole HP).
    dcR = juce::jlimit (0.9f, 0.99999f,
                        1.0f - juce::MathConstants<float>::twoPi * dcBlockHz / (float) sampleRate);

    constexpr double smoothSeconds = 0.01;   // 20 ms linear ramp
    for (auto& row : gains)
        for (auto& g : row)
            g.reset (sampleRate, smoothSeconds);
    for (auto& l : level) l.reset (sampleRate, smoothSeconds);
    for (auto& p : panL)  p.reset (sampleRate, smoothSeconds);
    for (auto& p : panR)  p.reset (sampleRate, smoothSeconds);
    for (auto& s : send)  s.reset (sampleRate, smoothSeconds);

    reset();
}

void FeedbackMatrix::reset()
{
    prevResonatorOut.fill (0.0f);
    for (auto& b : dcBlockers)
        b.reset();

    // Snap the gain smoothers to their current targets so a (re)started voice routes
    // correctly from sample 0; in-flight ramps from a previous note must not leak into
    // the attack. Smoothing still applies to parameter/modulation changes during sustain.
    auto snap = [] (SmoothedGain& s) { s.setCurrentAndTargetValue (s.getTargetValue()); };
    for (auto& row : gains) for (auto& g : row) snap (g);
    for (auto& l : level) snap (l);
    for (auto& p : panL)  snap (p);
    for (auto& p : panR)  snap (p);
    for (auto& s : send)  snap (s);
}

void FeedbackMatrix::assignParameters (ParamSource& apvts)
{
    for (int r = 0; r < numRows; ++r)
    {
        for (int c = 0; c < numColumns; ++c)
        {
            if (isSelfCell (r, c))
            {
                gainParam[(size_t) r][(size_t) c]    = nullptr;
                gainParamObj[(size_t) r][(size_t) c] = nullptr;
            }
            else
            {
                gainParam[(size_t) r][(size_t) c]    = apvts.getRawParameterValue (gainId (r, c));
                gainParamObj[(size_t) r][(size_t) c] = apvts.getApvts().getParameter (gainId (r, c));
            }
        }

        levelParam[(size_t) r] = apvts.getRawParameterValue (levelId (r));
        panParam[(size_t) r]   = apvts.getRawParameterValue (panId (r));
        sendParam[(size_t) r]  = apvts.getRawParameterValue (sendId (r));
    }

    for (int k = 0; k < numResonators; ++k)
        globalParam[(size_t) k] = apvts.getRawParameterValue (ResonatorSlot::globalParamId (ResonatorSlot::slotPrefix (k)));
}

void FeedbackMatrix::checkParameters()
{
    if (levelParam[0] == nullptr)
        return;

    // Aim each smoother at its new target. The very first call after prepare snaps
    // (no audible slew at start-up); later calls ramp toward the target.
    auto aim = [this] (SmoothedGain& s, float target)
    {
        if (smoothPrimed) s.setTargetValue (target);
        else              s.setCurrentAndTargetValue (target);
    };

    for (int r = 0; r < numRows; ++r)
    {
        for (int c = 0; c < numColumns; ++c)
        {
            auto* atom = gainParam[(size_t) r][(size_t) c];
            auto* prm  = gainParamObj[(size_t) r][(size_t) c];

            float target = 0.0f;
            if (atom != nullptr && prm != nullptr)
            {
                // base01 = knob position (unmodulated); mod01 = value after the engines'
                // modulation. modulatedGain applies the offset to the linear gain with the
                // sign locked, so modulation can't flip the cell's phase.
                const float base01 = prm->getValue();
                const float mod01  = prm->getNormalisableRange().convertTo0to1 (atom->load());
                target = modulatedGain (base01, mod01);
            }
            aim (gains[(size_t) r][(size_t) c], target);
        }

        aim (level[(size_t) r], juce::Decibels::decibelsToGain (levelParam[(size_t) r]->load(), -60.0f));

        const float pan   = panParam[(size_t) r]->load();
        const float angle = (pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
        aim (panL[(size_t) r], std::cos (angle));
        aim (panR[(size_t) r], std::sin (angle));

        aim (send[(size_t) r], juce::Decibels::decibelsToGain (sendParam[(size_t) r]->load(), -60.0f));
    }

    smoothPrimed = true;

    // Rebuild the per-row active-column lists. A cell counts as active while its
    // current OR target gain is non-zero: a cell ramping down to mute stays active
    // (target 0, current still ringing out) until its ramp reaches exactly 0, after
    // which both are 0 and it drops out. Self-cells and untouched (centre/mute) cells
    // are always 0, so they never enter the loop.
    for (int r = 0; r < numRows; ++r)
    {
        int n = 0;
        for (int c = 0; c < numColumns; ++c)
        {
            const auto& g = gains[(size_t) r][(size_t) c];
            if (g.getCurrentValue() != 0.0f || g.getTargetValue() != 0.0f)
                activeCol[(size_t) r][(size_t) n++] = c;
        }
        numActiveCols[(size_t) r] = n;
    }

    for (int k = 0; k < numResonators; ++k)
        globalMask[(size_t) k] = globalParam[(size_t) k] != nullptr && globalParam[(size_t) k]->load() > 0.5f;
}

void FeedbackMatrix::mixRow (int r, float out, int n, float* outL, float* outR, float* sendL, float* sendR)
{
    // One getNextValue per smoother per sample keeps all ramps sample-aligned.
    const float pl = panL[(size_t) r].getNextValue();
    const float pr = panR[(size_t) r].getNextValue();
    const float o  = out * level[(size_t) r].getNextValue();
    outL[n] += o * pl;
    outR[n] += o * pr;

    const float s = out * send[(size_t) r].getNextValue();   // pre-fader, panned
    if (sendL != nullptr)
    {
        sendL[n] += s * pl;
        sendR[n] += s * pr;
    }
}

void FeedbackMatrix::processVoice (const float* const* sourceSamples,
                                   std::array<ResonatorSlot, numResonators>& resonators,
                                   const float* const* globalPrev,
                                   const float* busFeedback,
                                   float* outL, float* outR, float* sendL, float* sendR,
                                   float* const* columnSum,
                                   int numSamples)
{
    std::array<float, numResonators> curOut {};

    for (int i = 0; i < numSamples; ++i)
    {
        for (int r = 0; r < numResonators; ++r)
        {
            if (globalMask[(size_t) r])
                continue;   // handled by processGlobal

            float in = 0.0f;
            const int na = numActiveCols[(size_t) r];
            for (int idx = 0; idx < na; ++idx)
            {
                const int c = activeCol[(size_t) r][(size_t) idx];
                float colSample;
                if (c < numSources)
                    colSample = sourceSamples[c][i];
                else if (c == busFeedbackCol)
                    colSample = busFeedback != nullptr ? busFeedback[i] : 0.0f;
                else
                {
                    const int k = c - numSources;
                    colSample = globalMask[(size_t) k] ? globalPrev[k][i]
                                                       : prevResonatorOut[(size_t) k];
                }
                in += gains[(size_t) r][(size_t) c].getNextValue() * colSample;
            }
            // Bracket the resonator with finite-guards: sanitize the input so a stray
            // NaN can't latch into its state; block DC so it can't accumulate in the
            // loop; soft-limit so the matrix feedback can't run away (see softLimit).
            const float raw = resonators[(size_t) r].processSample (sanitize (in));
            curOut[(size_t) r] = softLimit (dcBlockers[(size_t) r].process (raw, dcR));
        }

        // Mix PV rows, accumulate PV-resonator columns, then advance feedback state.
        for (int r = 0; r < numResonators; ++r)
        {
            if (globalMask[(size_t) r])
                continue;
            mixRow (r, curOut[(size_t) r], i, outL, outR, sendL, sendR);
            columnSum[numSources + r][i] += curOut[(size_t) r];
            prevResonatorOut[(size_t) r] = curOut[(size_t) r];
        }

        // Sources are per-voice; accumulate them for the global rows to hear.
        for (int j = 0; j < numSources; ++j)
            columnSum[j][i] += sourceSamples[j][i];
    }
}

void FeedbackMatrix::processGlobal (const float* const* columnSum,
                                    std::array<ResonatorSlot, numResonators>& resonators,
                                    const float* busFeedback,
                                    float* outL, float* outR, float* sendL, float* sendR,
                                    float* const* globalOut,
                                    int numSamples)
{
    std::array<float, numResonators> curOut {};

    for (int i = 0; i < numSamples; ++i)
    {
        for (int r = 0; r < numResonators; ++r)
        {
            if (! globalMask[(size_t) r])
                continue;

            float in = 0.0f;
            const int na = numActiveCols[(size_t) r];
            for (int idx = 0; idx < na; ++idx)
            {
                const int c = activeCol[(size_t) r][(size_t) idx];
                float colSample;
                if (c < numSources)
                    colSample = columnSum[c][i];
                else if (c == busFeedbackCol)
                    colSample = busFeedback != nullptr ? busFeedback[i] : 0.0f;
                else
                {
                    const int k = c - numSources;
                    colSample = globalMask[(size_t) k] ? prevResonatorOut[(size_t) k]
                                                       : columnSum[numSources + k][i];
                }
                in += gains[(size_t) r][(size_t) c].getNextValue() * colSample;
            }
            // See processVoice: sanitize the input, block DC, soft-limit the output to
            // keep the global feedback bounded and finite.
            const float raw = resonators[(size_t) r].processSample (sanitize (in));
            curOut[(size_t) r] = softLimit (dcBlockers[(size_t) r].process (raw, dcR));
        }

        for (int r = 0; r < numResonators; ++r)
        {
            if (! globalMask[(size_t) r])
                continue;
            mixRow (r, curOut[(size_t) r], i, outL, outR, sendL, sendR);
            globalOut[r][i] = curOut[(size_t) r];
            prevResonatorOut[(size_t) r] = curOut[(size_t) r];
        }
    }
}

void FeedbackMatrix::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
{
    using FloatParam = juce::AudioParameterFloat;

    for (int r = 0; r < numRows; ++r)
    {
        for (int c = 0; c < numColumns; ++c)
        {
            if (isSelfCell (r, c))
                continue;

            // Bipolar, centre-mute gain: v in [-1, 1], 0 = mute, sign = phase, magnitude
            // is linear-in-dB (see gainFromParam). Readout shows dB and a phase mark.
            const auto gainAttrs = juce::AudioParameterFloatAttributes()
                .withStringFromValueFunction ([] (float v, int)
                {
                    if (std::abs (v) <= 1.0e-4f)
                        return juce::String ("-inf dB");
                    const float dB = gainMinDb + std::abs (v) * (gainMaxDb - gainMinDb);
                    juce::String s (dB, 1);
                    s << " dB";
                    if (v < 0.0f)
                        s << " " << juce::String::fromUTF8 ("\xC3\x98");   // phase-invert mark
                    return s;
                })
                .withValueFromStringFunction ([] (const juce::String& t)
                {
                    if (t.containsIgnoreCase ("inf"))
                        return 0.0f;
                    // The leading '-' belongs to the dB figure (e.g. "-12 dB" is a quiet
                    // in-phase gain); phase inversion is signalled only by the mark.
                    const float dB  = t.getFloatValue();
                    const float mag = juce::jlimit (0.0f, 1.0f, (dB - gainMinDb) / (gainMaxDb - gainMinDb));
                    const bool  inv = t.contains (juce::String::fromUTF8 ("\xC3\x98"));
                    return inv ? -mag : mag;
                });

            params.push_back (std::make_unique<FloatParam> (
                gainId (r, c), "Mtx " + juce::String (r) + "<-" + juce::String (c),
                juce::NormalisableRange<float> (-1.0f, 1.0f, 1e-3f), 0.0f, gainAttrs));
        }

        params.push_back (std::make_unique<FloatParam> (levelId (r), "Res " + juce::String (r) + " Level",
            juce::NormalisableRange<float> (-60.0f, 6.0f, 1e-2f), 0.0f));
        params.push_back (std::make_unique<FloatParam> (panId (r), "Res " + juce::String (r) + " Pan",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 1e-3f), 0.0f));
        params.push_back (std::make_unique<FloatParam> (sendId (r), "Res " + juce::String (r) + " Send",
            juce::NormalisableRange<float> (-60.0f, 12.0f, 0.1f), -60.0f,
            juce::AudioParameterFloatAttributes()
                .withLabel ("dB")
                .withStringFromValueFunction ([] (float v, int)
                {
                    return v <= -59.9f ? juce::String ("-inf") : juce::String (v, 1);
                })));
    }
}

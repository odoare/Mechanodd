/*
  ==============================================================================

    FeedbackMatrix.cpp

  ==============================================================================
*/

#include "FeedbackMatrix.h"
#include "SynthVoice.h"

static_assert (FeedbackMatrix::numSources == SynthVoice::numSourceSlots,
               "FeedbackMatrix::numSources must match SynthVoice::numSourceSlots");

void FeedbackMatrix::prepare (const juce::dsp::ProcessSpec&)
{
    reset();
}

void FeedbackMatrix::reset()
{
    prevResonatorOut.fill (0.0f);
}

void FeedbackMatrix::assignParameters (juce::AudioProcessorValueTreeState& apvts)
{
    for (int r = 0; r < numRows; ++r)
    {
        for (int c = 0; c < numColumns; ++c)
            gainParam[(size_t) r][(size_t) c] = isSelfCell (r, c) ? nullptr
                                                                  : apvts.getRawParameterValue (gainId (r, c));

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

    for (int r = 0; r < numRows; ++r)
    {
        for (int c = 0; c < numColumns; ++c)
        {
            auto* p = gainParam[(size_t) r][(size_t) c];
            gains[(size_t) r][(size_t) c] = (p != nullptr) ? p->load() : 0.0f;
        }

        level[(size_t) r] = juce::Decibels::decibelsToGain (levelParam[(size_t) r]->load(), -60.0f);

        const float pan   = panParam[(size_t) r]->load();
        const float angle = (pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
        panL[(size_t) r]  = std::cos (angle);
        panR[(size_t) r]  = std::sin (angle);

        send[(size_t) r]  = sendParam[(size_t) r]->load();
    }

    for (int k = 0; k < numResonators; ++k)
        globalMask[(size_t) k] = globalParam[(size_t) k] != nullptr && globalParam[(size_t) k]->load() > 0.5f;
}

void FeedbackMatrix::mixRow (int r, float out, int n, float* outL, float* outR, float* sendL, float* sendR)
{
    const float o = out * level[(size_t) r];
    outL[n] += o * panL[(size_t) r];
    outR[n] += o * panR[(size_t) r];

    if (sendL != nullptr)
    {
        const float s = out * send[(size_t) r];   // pre-fader, panned
        sendL[n] += s * panL[(size_t) r];
        sendR[n] += s * panR[(size_t) r];
    }
}

void FeedbackMatrix::processVoice (const float* const* sourceSamples,
                                   std::array<ResonatorSlot, numResonators>& resonators,
                                   const float* const* globalPrev,
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
            for (int c = 0; c < numColumns; ++c)
            {
                float colSample;
                if (c < numSources)
                    colSample = sourceSamples[c][i];
                else
                {
                    const int k = c - numSources;
                    colSample = globalMask[(size_t) k] ? globalPrev[k][i]
                                                       : prevResonatorOut[(size_t) k];
                }
                in += gains[(size_t) r][(size_t) c] * colSample;
            }
            curOut[(size_t) r] = resonators[(size_t) r].processSample (in);
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
            for (int c = 0; c < numColumns; ++c)
            {
                float colSample;
                if (c < numSources)
                    colSample = columnSum[c][i];
                else
                {
                    const int k = c - numSources;
                    colSample = globalMask[(size_t) k] ? prevResonatorOut[(size_t) k]
                                                       : columnSum[numSources + k][i];
                }
                in += gains[(size_t) r][(size_t) c] * colSample;
            }
            curOut[(size_t) r] = resonators[(size_t) r].processSample (in);
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

            params.push_back (std::make_unique<FloatParam> (
                gainId (r, c), "Mtx " + juce::String (r) + "<-" + juce::String (c),
                juce::NormalisableRange<float> (-2.0f, 2.0f, 1e-3f), 0.0f));
        }

        params.push_back (std::make_unique<FloatParam> (levelId (r), "Res " + juce::String (r) + " Level",
            juce::NormalisableRange<float> (-60.0f, 6.0f, 1e-2f), 0.0f));
        params.push_back (std::make_unique<FloatParam> (panId (r), "Res " + juce::String (r) + " Pan",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 1e-3f), 0.0f));
        params.push_back (std::make_unique<FloatParam> (sendId (r), "Res " + juce::String (r) + " Send",
            juce::NormalisableRange<float> (0.0f, 1.0f, 1e-3f), 0.0f));
    }
}

/*
  ==============================================================================

    Biquad.h

    Minimal transposed-direct-form-II biquad with in-place RBJ coefficient
    computation. Used as one resonant mode in the modal resonators. Unlike
    juce::dsp::IIR, setting coefficients here allocates nothing, so the modal
    recompute path is fully realtime-safe.

  ==============================================================================
*/

#pragma once

#include <cmath>

struct Biquad
{
    float b0 { 1.0f }, b1 { 0.0f }, b2 { 0.0f }, a1 { 0.0f }, a2 { 0.0f };
    float z1 { 0.0f }, z2 { 0.0f };

    inline float process (float x) noexcept
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    void reset() noexcept { z1 = z2 = 0.0f; }

    // Constant 0 dB peak-gain band-pass (RBJ cookbook).
    void setBandPass (double sampleRate, double freqHz, double Q) noexcept
    {
        constexpr double twoPi = 6.283185307179586476925286766559;
        const double w0    = twoPi * freqHz / sampleRate;
        const double cosw0 = std::cos (w0);
        const double alpha = std::sin (w0) / (2.0 * Q);
        const double a0    = 1.0 + alpha;

        b0 = (float) ( alpha / a0);
        b1 = 0.0f;
        b2 = (float) (-alpha / a0);
        a1 = (float) (-2.0 * cosw0 / a0);
        a2 = (float) ((1.0 - alpha) / a0);
    }
};

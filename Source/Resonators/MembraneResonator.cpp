/*
  ==============================================================================

    MembraneResonator.cpp

  ==============================================================================
*/

#include "MembraneResonator.h"

float MembraneResonator::modeFrequency (int m, int n, float fundamental, float aspectRatio) const
{
    const float fm = (float) m;
    const float fn = (float) n / aspectRatio;
    const float num = std::sqrt (fm * fm + fn * fn);
    const float den = std::sqrt (1.0f + 1.0f / (aspectRatio * aspectRatio));
    return fundamental * num / den;
}

float MembraneResonator::modeGainCompensation (float zeta) const noexcept
{
    constexpr float zetaRef = 0.02f;   // 2x the plate's reference -> +3 dB everywhere
    const float g = std::sqrt (zetaRef / juce::jmax (zeta, 1.0e-7f));
    return juce::jlimit (0.5f, 64.0f, g);
}

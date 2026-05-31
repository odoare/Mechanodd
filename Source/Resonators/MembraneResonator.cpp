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

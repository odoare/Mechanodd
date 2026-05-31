/*
  ==============================================================================

    PlateResonator.cpp

  ==============================================================================
*/

#include "PlateResonator.h"

float PlateResonator::modeFrequency (int m, int n, float fundamental, float aspectRatio) const
{
    // aspectRatio = b/a >= 1, with a along x. Normalise so f(1,1) == fundamental.
    const float fm = (float) m;
    const float fn = (float) n / aspectRatio;
    const float num = fm * fm + fn * fn;
    const float den = 1.0f + 1.0f / (aspectRatio * aspectRatio);
    return fundamental * num / den;
}

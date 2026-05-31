/*
  ==============================================================================

    PlateResonator.h

    Simply-supported rectangular thin (Kirchhoff) plate. Eigenfrequencies obey
        f_mn ~ (m/a)^2 + (n/b)^2
    normalised so the (1,1) mode equals the tuned fundamental.

  ==============================================================================
*/

#pragma once

#include "ModalResonator.h"

class PlateResonator : public ModalResonator
{
public:
    static constexpr const char* typeName = "Plate";

protected:
    float modeFrequency (int m, int n, float fundamental, float aspectRatio) const override;
};

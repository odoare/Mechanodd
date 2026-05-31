/*
  ==============================================================================

    MembraneResonator.h

    Simply-supported rectangular membrane (2D wave equation). Eigenfrequencies
    obey
        f_mn ~ sqrt( (m/a)^2 + (n/b)^2 )
    normalised so the (1,1) mode equals the tuned fundamental. Differs from the
    plate only in this frequency law; mode shapes are identical.

  ==============================================================================
*/

#pragma once

#include "ModalResonator.h"

class MembraneResonator : public ModalResonator
{
public:
    static constexpr const char* typeName = "Membrane";

protected:
    float modeFrequency (int m, int n, float fundamental, float aspectRatio) const override;
};

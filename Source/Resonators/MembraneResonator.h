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
    // The membrane's sqrt(m^2 + n^2) law packs modes densely in a narrow band;
    // at low damping the closely-spaced narrow biquads interfere across the sum
    // and lose perceived level. We compensate with a stronger curve: 2x larger
    // zetaRef (uniform +3 dB across the range) and a doubled clamp ceiling so
    // the boost can keep growing to ~+6 dB at the most resonant settings.
    float modeGainCompensation (float zeta) const noexcept override;
};

/*
  ==============================================================================

    WavetableLibrary.h

    Open registry of single-cycle wavetables. Each table is generated from a
    {name, generator} entry, so adding a waveform is a one-line change in the
    .cpp (no static data blobs, no hand-maintained selector). Tables are built
    once, at a fixed size, with wrap-around so interpolation is seamless.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace WavetableLibrary
{
    struct Table
    {
        juce::String name;
        std::vector<float> samples;   // one cycle, length tableSize()
    };

    int tableSize();
    const std::vector<Table>& tables();
    juce::StringArray names();

    // Linear interpolation with wrap-around. phase in [0,1).
    inline float read (const Table& t, float phase) noexcept
    {
        const int n = (int) t.samples.size();
        const float p = phase * (float) n;
        int i0 = (int) p;
        const float frac = p - (float) i0;
        i0 %= n;
        const int i1 = (i0 + 1) % n;
        return t.samples[(size_t) i0] + frac * (t.samples[(size_t) i1] - t.samples[(size_t) i0]);
    }
}

/*
  ==============================================================================

    WavetableLibrary.cpp

    To add a waveform: add one entry to `defs` below — {display name, function of
    phase in [0,1) returning a sample in [-1,1]}.

  ==============================================================================
*/

#include "WavetableLibrary.h"

namespace
{
    constexpr int kTableSize = 2048;

    struct WaveDef { juce::String name; std::function<float (float)> fn; };

    const std::vector<WaveDef>& defs()
    {
        static const float twoPi = juce::MathConstants<float>::twoPi;

        static const std::vector<WaveDef> d = {
            { "Sine",     [] (float p) { return std::sin (twoPi * p); } },
            { "Triangle", [] (float p) { return p < 0.5f ? (4.0f * p - 1.0f) : (3.0f - 4.0f * p); } },
            { "Saw",      [] (float p) { return 2.0f * p - 1.0f; } },
            { "Square",   [] (float p) { return p < 0.5f ? 1.0f : -1.0f; } },
            { "Pulse 25", [] (float p) { return p < 0.25f ? 1.0f : -1.0f; } },
            { "Half Sine",[] (float p) { return p < 0.5f ? std::sin (twoPi * p) : 0.0f; } },
            { "Harmonics",[] (float p) { return 0.6f * std::sin (twoPi * p)
                                              + 0.3f * std::sin (2.0f * twoPi * p)
                                              + 0.1f * std::sin (3.0f * twoPi * p); } },
            { "Noise Cyc",[] (float p) { juce::Random r ((juce::int64) (p * 100000.0f));
                                         return 2.0f * r.nextFloat() - 1.0f; } }
        };
        return d;
    }
}

int WavetableLibrary::tableSize() { return kTableSize; }

const std::vector<WavetableLibrary::Table>& WavetableLibrary::tables()
{
    static const std::vector<Table> built = []
    {
        std::vector<Table> t;
        for (const auto& def : defs())
        {
            Table table;
            table.name = def.name;
            table.samples.resize ((size_t) kTableSize);
            for (int i = 0; i < kTableSize; ++i)
                table.samples[(size_t) i] = def.fn ((float) i / (float) kTableSize);
            t.push_back (std::move (table));
        }
        return t;
    }();
    return built;
}

juce::StringArray WavetableLibrary::names()
{
    juce::StringArray n;
    for (const auto& t : tables())
        n.add (t.name);
    return n;
}

/*
  ==============================================================================

    Effect.h

    Uniform wrapper over the reused FxmeFX effect classes. Those classes share a
    common interface (process / assignParameters / checkParameters / static
    addParameters) but differ in their prepare() signature, so EffectAdapter<T>
    adapts each to one virtual interface. The prepare overload-set is resolved at
    compile time by priority tag dispatch.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>


class Effect
{
public:
    virtual ~Effect() = default;

    virtual void prepare (double sampleRate, int numChannels, int blockSize) = 0;
    virtual void process (juce::AudioBuffer<float>& buffer) = 0;
    virtual void addParametersToLayout (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix) = 0;
    virtual void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix) = 0;
    virtual void checkParameters() = 0;
};

namespace effectdetail
{
    template <int N> struct prio : prio<N - 1> {};
    template <>      struct prio<0> {};

    template <class T> auto callPrepare (prio<2>, T& t, double sr, int nch, int bs)
        -> decltype (t.prepare (sr, nch, bs)) { return t.prepare (sr, nch, bs); }

    template <class T> auto callPrepare (prio<1>, T& t, double sr, int nch, int bs)
        -> decltype (t.prepare (sr, nch)) { juce::ignoreUnused (bs); return t.prepare (sr, nch); }

    template <class T> auto callPrepare (prio<0>, T& t, double sr, int nch, int bs)
        -> decltype (t.prepare (sr)) { juce::ignoreUnused (nch, bs); return t.prepare (sr); }
}

template <class T>
class EffectAdapter : public Effect
{
public:
    T& get() { return impl; }

    void prepare (double sampleRate, int numChannels, int blockSize) override
    {
        effectdetail::callPrepare (effectdetail::prio<2>{}, impl, sampleRate, numChannels, blockSize);
    }

    void process (juce::AudioBuffer<float>& buffer) override { impl.process (buffer); }

    void addParametersToLayout (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix) override
    {
        T::addParameters (params, prefix);
    }

    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix) override { impl.assignParameters (apvts, prefix); }
    void checkParameters() override { impl.checkParameters(); }

private:
    T impl;
};

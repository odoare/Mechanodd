/*
  ==============================================================================

    InputSource.h

    Feeds the host's audio input into the engine. Two concrete types tap the
    left and right input channels respectively. Beyond reading the channel,
    they behave like any other source: the common ADSR / resonant LPF / level /
    velocity links shape how each voice is fed by the input.

  ==============================================================================
*/

#pragma once

#include "Source.h"

class PluginInputSource : public Source
{
public:
    explicit PluginInputSource (int channel) : inputChannel (channel) {}

    void addParametersToLayout (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                                const juce::String& prefix) override
    {
        addCommonParameters (params, prefix);
    }

    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix) override
    {
        assignCommonParameters (apvts, prefix);
    }

    void checkParameters() override
    {
        checkCommonParameters();
    }

    void setInputPointers (const float* l, const float* r) override
    {
        inL = l;
        inR = r;
    }

protected:
    void renderSource (juce::AudioBuffer<float>& outBuffer, int startSample, int numSamples) override
    {
        const float* src = (inputChannel == 0) ? inL : inR;
        auto* out = outBuffer.getWritePointer (0);

        if (src == nullptr)
        {
            juce::FloatVectorOperations::clear (out + startSample, numSamples);
            return;
        }

        for (int i = 0; i < numSamples; ++i)
            out[startSample + i] = src[i];
    }

private:
    int inputChannel { 0 };
    const float* inL { nullptr };
    const float* inR { nullptr };
};

class InputLeftSource : public PluginInputSource
{
public:
    static constexpr const char* typeName = "Input L";
    InputLeftSource() : PluginInputSource (0) {}
};

class InputRightSource : public PluginInputSource
{
public:
    static constexpr const char* typeName = "Input R";
    InputRightSource() : PluginInputSource (1) {}
};

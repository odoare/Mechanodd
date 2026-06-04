/*
  ==============================================================================

    EffectFactory.cpp

  ==============================================================================
*/

#include "EffectFactory.h"

#include "StereoDelay.h"
#include "StereoDelayComponent.h"
#include "Tube.h"
#include "TubeComponent.h"
#include "Equalizer.h"
#include "EqualizerComponent.h"
#include "Oct.h"
#include "OctComponent.h"
#include "Compressor.h"
#include "CompressorComponent.h"
#include "Transient.h"
#include "TransientComponent.h"
#include "Cab.h"
#include "CabComponent.h"
#include "ConvolReverb.h"
#include "ConvolReverbComponent.h"

namespace
{
    // Builds a registry entry for effect class FX with component class FXComponent.
    template <class FX, class FXComponent>
    EffectTypeInfo makeEntry (const juce::String& name)
    {
        return {
            name,
            []                                       { return std::unique_ptr<Effect> (new EffectAdapter<FX>()); },
            [] (Effect& e, juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
            {
                auto& impl = static_cast<EffectAdapter<FX>&> (e).get();
                return std::unique_ptr<juce::Component> (new FXComponent (impl, apvts, prefix));
            }
        };
    }

    // ── Cab adapter ───────────────────────────────────────────────────────────
    // Cab::prepare takes (sampleRate, samplesPerBlock) — different from the
    // standard (sampleRate, numChannels) — and addParameters needs the IR count
    // explicitly, so we use a hand-written adapter instead of EffectAdapter<Cab>.
    class CabAdapter : public Effect
    {
    public:
        CabAdapter()
        {
            // Map display names to the BinaryData symbol names generated from the
            // IR filenames in lib/FxmeFX/Source/Cab/IR/.
            static const std::pair<const char*, const char*> kIRs[] = {
                { "2off-pres5",                           "_2offpres5_wav"                      },
                { "Allure 67 Brit Greenback",              "Allure_67_Brit_Greenback_wav"         },
                { "Allure 90s Cali V30",                  "Allure_90s_Cali_V30_wav"              },
                { "Annihilator Feast",                    "AnnihilatorFeast_wav"                 },
                { "Brohymn Mesa 4x12 SM57 V30 1",         "Brohymn_Mesa_4x12_SM57_V30_1_wav"    },
                { "Brohymn-Mesa-4x12-SM57-V30-5",         "BrohymnMesa4x12SM57V305_wav"          },
                { "Cenzo Celestion V30 Mix",               "Cenzo_Celestion_V30_Mix_wav"          },
                { "Excalibur 1 - Bright1, a",              "Excalibur_1__Bright1_a_wav"           },
                { "Excalibur 2 - Dark3, b",                "Excalibur_2__Dark3_b_wav"             },
                { "GuitarHack JJ CENTRE45 0",             "GuitarHack_JJ_CENTRE45_0_wav"         },
                { "GuitarHack JJ FRED45 0",               "GuitarHack_JJ_FRED45_0_wav"           },
                { "KornKorn",                             "KornKorn_wav"                         },
                { "Marshall1960A-G12Ms-M160-CapEdge-3in", "Marshall1960AG12MsM160CapEdge3in_wav" },
                { "Marshall1960A-G12Ms-SM57-Cap-0in",     "Marshall1960AG12MsSM57Cap0in_wav"     },
                { "Neumann U87 0_dc",                     "NewmannU87_0_dc_wav"                  },
                { "OH 412 MES-ST V60 421-04",             "OH_412_MESST_V60_42104_wav"           },
                { "OH 412 MES-ST V60 57-00",              "OH_412_MESST_V60_5700_wav"            },
                { "Shure SM57 0_dc",                      "ShureSM57_0_dc_wav"                   },
                { "s-preshigh",                           "spreshigh_wav"                        },
            };

            juce::StringArray names, resources;
            for (const auto& [name, resource] : kIRs)
            {
                names.add (name);
                resources.add (resource);
            }
            impl.setImpulseList (names, resources);
        }

        Cab& get() { return impl; }

        void prepare (double sampleRate, int /*numChannels*/, int blockSize) override
        {
            impl.prepare (sampleRate, blockSize);
        }

        void process (juce::AudioBuffer<float>& buffer) override { impl.process (buffer); }

        void addParametersToLayout (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                                    const juce::String& prefix) override
        {
            Cab::addParameters (params, prefix, impl.getImpulseNames().size());
        }

        void assignParameters (juce::AudioProcessorValueTreeState& apvts,
                               const juce::String& prefix) override
        {
            impl.assignParameters (apvts, prefix);
        }

        void checkParameters() override { impl.checkParameters(); }

    private:
        Cab impl;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CabAdapter)
    };

    // ── ConvolReverb adapter ──────────────────────────────────────────────────
    // ConvolReverb::prepare takes (sampleRate, samplesPerBlock) and its
    // checkParameters() is private (called internally from process()). The
    // adapter provides a no-op checkParameters and routes prepare correctly.
    class ConvolReverbAdapter : public Effect
    {
    public:
        ConvolReverbAdapter()
        {
            // Map display names to BinaryData symbols from
            // lib/FxmeFX/Source/ConvolReverb/ir/.
            static const std::pair<const char*, const char*> kIRs[] = {
                { "Council Chamber",         "Council_Chamber_wav"         },
                { "Forest short",            "Forest_short_wav"            },
                { "Forest long",             "Forest_long_wav"             },
                { "Rectangular room small",  "Rectangular_room_small_wav"  },
                { "Rectangular room medium", "Rectangular_room_medium_wav" },
                { "Rectangular room large",  "Rectangular_room_large_wav"  },
            };

            juce::StringArray names, resources;
            for (const auto& [name, resource] : kIRs)
            {
                names.add (name);
                resources.add (resource);
            }
            impl.setImpulseList (names, resources);
        }

        ConvolReverb& get() { return impl; }

        void prepare (double sampleRate, int /*numChannels*/, int blockSize) override
        {
            impl.prepare (sampleRate, blockSize);
        }

        void process (juce::AudioBuffer<float>& buffer) override { impl.process (buffer); }

        void addParametersToLayout (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                                    const juce::String& prefix) override
        {
            ConvolReverb::addParameters (params, prefix, impl.getImpulseNames().size());
        }

        void assignParameters (juce::AudioProcessorValueTreeState& apvts,
                               const juce::String& prefix) override
        {
            impl.assignParameters (apvts, prefix);
        }

        // ConvolReverb calls its own checkParameters() from process(); no
        // external call is needed.
        void checkParameters() override {}

    private:
        ConvolReverb impl;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConvolReverbAdapter)
    };

    // Factory entry for the two IR-backed effects whose adapters are hand-written.
    template <class Adapter, class FXComponent>
    EffectTypeInfo makeIREntry (const juce::String& name)
    {
        return {
            name,
            [] { return std::unique_ptr<Effect> (new Adapter()); },
            [] (Effect& e, juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
            {
                auto& impl = static_cast<Adapter&> (e).get();
                return std::unique_ptr<juce::Component> (new FXComponent (impl, apvts, prefix));
            }
        };
    }
}

const std::vector<EffectTypeInfo>& EffectFactory::types()
{
    static const std::vector<EffectTypeInfo> registry = {
        makeEntry<StereoDelay,  StereoDelayComponent>  ("Delay"),
        makeEntry<Tube,         TubeComponent>          ("Tube"),
        makeEntry<Equalizer,    EqualizerComponent>     ("EQ"),
        makeEntry<Oct,          OctComponent>           ("Oct"),
        makeEntry<Compressor,   CompressorComponent>    ("Comp"),
        makeEntry<Transient,    TransientComponent>     ("Transient"),
        makeIREntry<CabAdapter,         CabComponent>         ("Cab"),
        makeIREntry<ConvolReverbAdapter, ConvolReverbComponent> ("Reverb"),
    };
    return registry;
}

juce::StringArray EffectFactory::typeChoices()
{
    juce::StringArray choices;
    choices.add ("Off");
    for (const auto& info : types())
        choices.add (info.name);
    return choices;
}

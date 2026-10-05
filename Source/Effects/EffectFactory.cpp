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
#include "Limiter.h"
#include "LimiterComponent.h"
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
    EffectTypeInfo makeEntry (const juce::String& name, const juce::String& moduleName, const juce::String& paramTag,
                              int preferredWidth, int preferredHeight)
    {
        return {
            name,
            moduleName,
            paramTag,
            []                                       { return std::unique_ptr<Effect> (new EffectAdapter<FX>()); },
            [] (EffectTypeInfo::ParameterList& params, const juce::String& prefix) { FX::addParameters (params, prefix); },
            [] (Effect& e, juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
            {
                auto& impl = static_cast<EffectAdapter<FX>&> (e).get();
                return std::unique_ptr<juce::Component> (new FXComponent (impl, apvts, prefix));
            },
            preferredWidth,
            preferredHeight
        };
    }

    // ── IR-backed effects: loading deferred until used ────────────────────────
    // Cab and ConvolReverb load an IR as soon as they get their IR list
    // (decode, resample, FFT partitioning) and start a polling thread in
    // prepare(). Every slot holds one of each, so that is put off until the
    // slot is actually set to them: ensureLoaded(), called by the processor's
    // loader thread (or from prepareToPlay, for a slot already set to them).
    // Until then the effect outputs silence; prepare() only records the
    // settings. Once loaded it stays loaded.
    //
    // The order inside ensureLoaded() matters: the IR list first, then
    // prepare(). The other way round, the effect's polling thread would find
    // an empty list, mark the IR parameter as handled, and never load the IR
    // it names.
    template <class Impl>
    class DeferredIRAdapter : public Effect
    {
    public:
        Impl& get() { return impl; }

        void prepare (double newSampleRate, int /*numChannels*/, int newBlockSize) override
        {
            const juce::ScopedLock sl (loadLock);
            sampleRate = newSampleRate;
            blockSize  = newBlockSize;
            prepared   = true;
            if (loaded)
            {
                ready = false;
                impl.prepare (sampleRate, blockSize);
                ready = true;
            }
        }

        void ensureLoaded() override
        {
            const juce::ScopedLock sl (loadLock);
            if (loaded)
                return;

            setImpulseList();
            loaded = true;

            // A GUI-side instance is never prepared: it only needs the list
            // (and the IR, for its display).
            if (prepared)
            {
                impl.prepare (sampleRate, blockSize);
                ready = true;
            }
        }

        void process (juce::AudioBuffer<float>& buffer) override
        {
            if (ready.load (std::memory_order_acquire))
                impl.process (buffer);
            else
                buffer.clear();   // still loading: silence rather than an unprocessed signal
        }

        void assignParameters (juce::AudioProcessorValueTreeState& apvts,
                               const juce::String& prefix) override
        {
            impl.assignParameters (apvts, prefix);
        }

        void checkParameters() override
        {
            if (ready.load (std::memory_order_acquire))
                checkImplParameters();
        }

    protected:
        virtual void setImpulseList() = 0;      // hands the IR list to impl (loads the selected IR)
        virtual void checkImplParameters() {}

        Impl impl;

    private:
        juce::CriticalSection loadLock;          // ensureLoaded vs prepare; never taken by audio
        bool loaded = false, prepared = false;
        double sampleRate = 44100.0;
        int blockSize = 512;
        std::atomic<bool> ready { false };       // loaded and prepared: process() may run
    };

    // ── Cab adapter ───────────────────────────────────────────────────────────
    // Cab::prepare takes (sampleRate, samplesPerBlock), and addParameters
    // needs the IR count explicitly.
    class CabAdapter : public DeferredIRAdapter<Cab>
    {
    public:
        // Map display names to the BinaryData symbol names generated from the
        // IR filenames in lib/FxmeFX/Source/Cab/IR/.
        static constexpr std::pair<const char*, const char*> kIRs[] = {
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
        static constexpr int numIRs = (int) std::size (kIRs);

        static void addParameters (EffectTypeInfo::ParameterList& params, const juce::String& prefix)
        {
            Cab::addParameters (params, prefix, numIRs);
        }

    protected:
        void setImpulseList() override
        {
            juce::StringArray names, resources;
            for (const auto& [name, resource] : kIRs)
            {
                names.add (name);
                resources.add (resource);
            }
            impl.setImpulseList (names, resources);
        }

        void checkImplParameters() override { impl.checkParameters(); }
    };

    // ── ConvolReverb adapter ──────────────────────────────────────────────────
    // ConvolReverb::prepare takes (sampleRate, samplesPerBlock) and its
    // checkParameters() is private (called internally from process()), so
    // there is nothing to check from here.
    class ConvolReverbAdapter : public DeferredIRAdapter<ConvolReverb>
    {
    public:
        // Map display names to BinaryData symbols from
        // lib/FxmeFX/Source/ConvolReverb/ir/. Same order as FxmeFX's own
        // ConvolReverb plugin: the IR parameter is an index, and the
        // effect's module presets (shared with that plugin) store it.
        // The first three are recorded as mid / side and are decoded to
        // left / right on load; played as left / right they come out loud
        // and lopsided to the left.
        struct IR { const char* name; const char* resource; bool midSide; };
        static constexpr IR kIRs[] = {
            { "Council Chamber",         "Council_Chamber_wav",         true  },
            { "Forest short",            "Forest_short_wav",            true  },
            { "Forest long",             "Forest_long_wav",             true  },
            { "Rectangular room small",  "Rectangular_room_small_wav",  false },
            { "Rectangular room medium", "Rectangular_room_medium_wav", false },
            { "Rectangular room large",  "Rectangular_room_large_wav",  false },
        };
        static constexpr int numIRs = (int) std::size (kIRs);

        static void addParameters (EffectTypeInfo::ParameterList& params, const juce::String& prefix)
        {
            ConvolReverb::addParameters (params, prefix, numIRs);
        }

    protected:
        void setImpulseList() override
        {
            juce::StringArray names, resources, midSide;
            for (const auto& ir : kIRs)
            {
                names.add (ir.name);
                resources.add (ir.resource);
                if (ir.midSide)
                    midSide.add (ir.resource);
            }
            impl.setImpulseList (names, resources, midSide);
        }
    };

    // Factory entry for the two IR-backed effects whose adapters are hand-written.
    template <class Adapter, class FXComponent>
    EffectTypeInfo makeIREntry (const juce::String& name, const juce::String& moduleName, const juce::String& paramTag,
                                int preferredWidth, int preferredHeight)
    {
        return {
            name,
            moduleName,
            paramTag,
            [] { return std::unique_ptr<Effect> (new Adapter()); },
            [] (EffectTypeInfo::ParameterList& params, const juce::String& prefix) { Adapter::addParameters (params, prefix); },
            [] (Effect& e, juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
            {
                auto& impl = static_cast<Adapter&> (e).get();
                return std::unique_ptr<juce::Component> (new FXComponent (impl, apvts, prefix));
            },
            preferredWidth,
            preferredHeight
        };
    }
}

const std::vector<EffectTypeInfo>& EffectFactory::types()
{
    static const std::vector<EffectTypeInfo> registry = {
        makeEntry<StereoDelay,  StereoDelayComponent>  ("Delay",     "StereoDelay", "Del",   600, 300),
        makeEntry<Tube,         TubeComponent>          ("Tube",      "Tube",        "Tube",  640, 300),
        makeEntry<Equalizer,    EqualizerComponent>     ("EQ",        "Equalizer",   "EQ",    600, 700),
        makeEntry<Oct,          OctComponent>           ("Oct",       "Oct",         "Oct",   520, 320),
        makeEntry<Compressor,   CompressorComponent>    ("Comp",      "Compressor",  "Comp",  600, 300),
        makeEntry<Limiter,      LimiterComponent>       ("Limit",     "Limiter",     "Lim",   520, 280),
        makeEntry<Transient,    TransientComponent>     ("Transient", "Transient",   "Trans", 480, 300),
        makeIREntry<CabAdapter,          CabComponent>          ("Cab",    "Cab",          "Cab", 600, 400),
        makeIREntry<ConvolReverbAdapter, ConvolReverbComponent>  ("Reverb", "ConvolReverb", "Rev", 600, 400),
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

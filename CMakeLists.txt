cmake_minimum_required(VERSION 3.22)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

project(MechanOdd VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Target architectures")
endif()

# JUCE and the FxmeJuceTools user-module are expected as siblings of this repo,
# mirroring the FxmeFX layout:
#   ../JUCE/                              (juce-framework/JUCE checkout)
#   ../JUCE/usermodules/FxmeJuceTools/    (symlink to FxmeJuceTools/module/FxmeJuceTools)
add_subdirectory(../JUCE JUCE)
juce_add_module(../JUCE/usermodules/FxmeJuceTools)

juce_add_plugin(MechanOdd
    COMPANY_NAME                "FX-Mechanics"
    COMPANY_WEBSITE             "www.fx-mechanics.com"
    PLUGIN_MANUFACTURER_CODE    FXME
    PLUGIN_CODE                 MECO
    FORMATS                     AU VST3 Standalone
    COPY_PLUGIN_AFTER_BUILD     TRUE
    PRODUCT_NAME                "MechanOdd"
    DESCRIPTION                 "FX-Mechanics modular physical-modelling synth"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             Instrument Synth
    BUNDLE_ID                   "com.FXMechanics.MechanOdd"
)

# Source files are discovered recursively so adding a new module/component
# does not require editing this file. Re-run CMake configure after adding files.
file(GLOB_RECURSE MECHANODD_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h
)

target_sources(MechanOdd PRIVATE ${MECHANODD_SOURCES})

# Make every subdirectory under Source/ an include root so headers can be
# included by name regardless of where a module lives.
file(GLOB MECHANODD_INCLUDE_DIRS LIST_DIRECTORIES true
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*
)
list(FILTER MECHANODD_INCLUDE_DIRS EXCLUDE REGEX "\\.(c|cc|cpp|cxx|h|hpp|hxx|mm)$")
target_include_directories(MechanOdd PRIVATE ${MECHANODD_INCLUDE_DIRS})

# ── Reused FxmeFX effects ─────────────────────────────────────────────────────
# Compile each effect's DSP class and its component (not the standalone plugin
# wrappers PluginProcessor/PluginEditor/PdMain) into MechanOdd.
set(FXMEFX_EFFECTS StereoDelay Tube Equalizer Oct VuMeter Compressor Limiter Transient Cab ConvolReverb)
set(FXMEFX_ASSETS "")
foreach(_fx IN LISTS FXMEFX_EFFECTS)
    set(_fxdir ${CMAKE_CURRENT_SOURCE_DIR}/lib/FxmeFX/Source/${_fx})
    file(GLOB _fxsrcs ${_fxdir}/*.cpp)
    list(FILTER _fxsrcs EXCLUDE REGEX "(PluginProcessor|PluginEditor|PdMain)\\.cpp$")
    target_sources(MechanOdd PRIVATE ${_fxsrcs})
    target_include_directories(MechanOdd PRIVATE ${_fxdir})

    # Collect image assets and IR WAV files (Cab uses IR/, ConvolReverb uses ir/).
    file(GLOB _fxassets
        ${_fxdir}/img/*.png ${_fxdir}/img/*.jpg ${_fxdir}/Assets/*.png
        ${_fxdir}/IR/*.wav  ${_fxdir}/ir/*.wav)
    list(APPEND FXMEFX_ASSETS ${_fxassets})
endforeach()

# Add MechanOdd's own image assets and factory preset XML files.
file(GLOB MECHANODD_ASSETS ${CMAKE_CURRENT_SOURCE_DIR}/Source/Assets/*.png
                            ${CMAKE_CURRENT_SOURCE_DIR}/Source/Assets/*.jpg
                            ${CMAKE_CURRENT_SOURCE_DIR}/Source/Assets/*.xml)
list(APPEND FXMEFX_ASSETS ${MECHANODD_ASSETS})

if(FXMEFX_ASSETS)
    juce_add_binary_data(MechanOddBinaryData SOURCES ${FXMEFX_ASSETS})
    # Embedded WAV byte arrays don't benefit from LTO and can exhaust RAM at
    # compile time — disable both for the binary-data target only.
    set_target_properties(MechanOddBinaryData PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION OFF)
    target_compile_options(MechanOddBinaryData PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-O0 -fno-lto>
        $<$<CXX_COMPILER_ID:MSVC>:/Od>)
    target_link_libraries(MechanOdd PRIVATE MechanOddBinaryData)
endif()

# ── WDL convolution engine (required by Cab and ConvolReverb) ─────────────────
set(_wdldir ${CMAKE_CURRENT_SOURCE_DIR}/lib/FxmeFX/WDL/WDL)
target_sources(MechanOdd PRIVATE
    ${_wdldir}/convoengine.cpp
    ${_wdldir}/fft.c
    ${_wdldir}/resample.cpp
)

# FxmeJuceTools DSP helpers not exposed by the module umbrella header.
set(FXME_DSP ${CMAKE_CURRENT_SOURCE_DIR}/../JUCE/usermodules/FxmeJuceTools/Dsp)
target_sources(MechanOdd PRIVATE ${FXME_DSP}/CracksGenerator.cpp)
target_include_directories(MechanOdd PRIVATE ${FXME_DSP})

juce_generate_juce_header(MechanOdd)

target_compile_definitions(MechanOdd
    PUBLIC
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(MechanOdd
    PRIVATE
        FxmeJuceTools
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

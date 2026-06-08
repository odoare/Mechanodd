/*
  ==============================================================================

    Theme.h

    Colour scheme. The backdrop is the StrinGO diagonal gradient, shared by every
    tab. Controls are tinted per *slot*: each source and each resonator has its
    own accent (applied to the slider colour IDs the FxmeLookAndFeel reads, after
    the FxmeFX pattern). The feedback matrix reuses the resonator accents so each
    row carries its destination resonator's colour.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace MechanOddTheme
{
    // ---- StrinGO-style background ------------------------------------------------
    inline void paintBackground (juce::Graphics& g, juce::Rectangle<float> b)
    {
        const auto base = juce::Colour::fromFloatRGBA (0.15f, 0.15f, 0.25f, 1.0f);
        juce::ColourGradient grad (base.darker().darker().darker(), b.getBottomLeft(),
                                   base, b.getTopRight(), false);
        g.setGradientFill (grad);
        g.fillRect (b);
    }

    // Uniform tab-button background; the gradient is the real backdrop.
    inline const juce::Colour tabButton { 0xff262633 };

    // Uniform accent for the (homogeneous) modulation page.
    inline const juce::Colour modulation { 0xff8aa0b8 };   // steel

    // ---- Per-slot accents --------------------------------------------------------
    inline juce::Colour sourceColour (int i) noexcept
    {
        static const juce::Colour c[] = { juce::Colour (0xffe8833a),   // S0 orange
                                          juce::Colour (0xffd9b13a),   // S1 gold
                                          juce::Colour (0xffe0586f),   // S2 rose
                                          juce::Colour (0xffd06fd0) }; // S3 magenta
        return c[(size_t) juce::jlimit (0, (int) (sizeof c / sizeof c[0]) - 1, i)];
    }

    // Accent for the send-bus feedback column in the matrix.
    inline juce::Colour busColour() noexcept { return juce::Colour (0xffb8a040); }   // amber

    inline juce::Colour resonatorColour (int i) noexcept
    {
        static const juce::Colour c[] = { juce::Colour (0xff35c0c0),   // R0 teal
                                          juce::Colour (0xff4a90e0),   // R1 azure
                                          juce::Colour (0xff8fc73e),   // R2 lime
                                          juce::Colour (0xffb164d6) }; // R3 violet
        return c[(size_t) juce::jlimit (0, (int) (sizeof c / sizeof c[0]) - 1, i)];
    }

    // ---- Applying an accent to controls ------------------------------------------
    // Knob body stays dark; the value arc, pointer and outline carry the accent.
    inline void accentSlider (juce::Slider& s, juce::Colour accent)
    {
        s.setColour (juce::Slider::rotarySliderFillColourId,    juce::Colour (0xff2b2b2b));
        s.setColour (juce::Slider::rotarySliderOutlineColourId, accent.darker (1.6f));
        s.setColour (juce::Slider::trackColourId,               accent);
        s.setColour (juce::Slider::thumbColourId,               accent.brighter (0.4f));
    }

    inline void accentComboBox (juce::ComboBox& c, juce::Colour accent)
    {
        c.setColour (juce::ComboBox::outlineColourId, accent.darker());
        c.setColour (juce::ComboBox::arrowColourId,   accent.brighter (0.3f));
    }

    inline void accentToggleButton (juce::ToggleButton& b, juce::Colour accent)
    {
        b.setColour (juce::ToggleButton::tickColourId,         accent);
        b.setColour (juce::ToggleButton::tickDisabledColourId, accent.withAlpha (0.4f));
    }

    // Tint every slider / combo box / toggle button in a component subtree.
    inline void applyAccent (juce::Component& root, juce::Colour accent)
    {
        for (auto* child : root.getChildren())
        {
            if      (auto* s = dynamic_cast<juce::Slider*>        (child)) accentSlider (*s, accent);
            else if (auto* c = dynamic_cast<juce::ComboBox*>       (child)) accentComboBox (*c, accent);
            else if (auto* b = dynamic_cast<juce::ToggleButton*>   (child)) accentToggleButton (*b, accent);

            applyAccent (*child, accent);
        }
    }
}

/*
  ==============================================================================

    AppSettings.h

    Small machine-wide settings shared by every MechanOdd instance (and the
    standalone app), persisted in a JUCE PropertiesFile under the user's
    application-data folder (FX-Mechanics/MechanOdd.settings: ~/.config on
    Linux, ~/Library/Application Support on macOS, %APPDATA% on Windows).
    Currently just whether the hover help is shown.

    Header-only: one shared PropertiesFile in a function-local static, so any
    component reaches it without plumbing the processor through. Message
    thread.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace mechanodd
{

/** Process-wide application settings (one shared file on disk). */
inline juce::ApplicationProperties& appProperties()
{
    static juce::ApplicationProperties props;
    static const bool inited = []
    {
        juce::PropertiesFile::Options o;
        o.applicationName     = "MechanOdd";
        o.filenameSuffix      = "settings";
       #if JUCE_LINUX || JUCE_BSD
        o.folderName          = ".config/FX-Mechanics";   // JUCE puts it under ~ itself on Linux
       #else
        o.folderName          = "FX-Mechanics";           // already under the application data folder
       #endif
        o.osxLibrarySubFolder = "Application Support";
        props.setStorageParameters (o);
        return true;
    }();
    juce::ignoreUnused (inited);
    return props;
}

/** Whether the hover help is shown (on by default). */
inline bool getUiTooltips()
{
    auto* s = appProperties().getUserSettings();
    return s == nullptr || s->getBoolValue ("uiTooltips", true);
}

inline void setUiTooltips (bool on)
{
    if (auto* s = appProperties().getUserSettings())
    {
        s->setValue ("uiTooltips", on);
        s->saveIfNeeded();
    }
}

} // namespace mechanodd

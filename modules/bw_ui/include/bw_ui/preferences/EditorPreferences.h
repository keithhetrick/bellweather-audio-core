// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include <memory>

namespace bws::ui
{

/**
 * Canonical per-plugin editor-preference store.
 *
 * On-disk contract - one XML property file per plugin, all editor
 * preferences (scale, tooltips, update-check, preset-directory override,
 * and any future persisted editor state) cohabit in it:
 *
 *   macOS:   ~/Library/Application Support/BellweatherStudios/<pluginName>.settings
 *   Windows: %APPDATA%\BellweatherStudios\<pluginName>.settings
 *   Linux:   ~/BellweatherStudios/<pluginName>.settings
 *
 * Writes persist synchronously. Every instance for the same plugin shares
 * one underlying file object, so independent owners (editor, preset
 * manager) cannot clobber each other's keys.
 *
 * Legacy migration: builds prior to this seam wrote the editor scale to a
 * shared WeatherPlugins.settings file (<appData>/Bellweather Studios/
 * Preferences/, key "<pluginName>_scale"). On first open of a per-plugin
 * store that has no kEditorScaleKey, that value is copied in once. The
 * legacy file is never written to and never deleted; other plugins' entries
 * in it are left for their own first-open migration.
 */
class EditorPreferences
{
public:
    static constexpr const char* kEditorScaleKey = "editorScale";

    /** Storage endpoints; the default constructor resolves the real
        per-platform locations. Injectable for tests. */
    struct Locations
    {
        juce::File canonicalFile;
        juce::File legacyScaleFile;
    };

    explicit EditorPreferences(const juce::String& pluginName);
    EditorPreferences(const juce::String& pluginName, const Locations& locations);

    bool getBool(const juce::String& key, bool defaultValue) const;
    int getInt(const juce::String& key, int defaultValue) const;
    double getDouble(const juce::String& key, double defaultValue) const;
    juce::String getString(const juce::String& key, const juce::String& defaultValue = {}) const;

    void setBool(const juce::String& key, bool value);
    void setInt(const juce::String& key, int value);
    void setDouble(const juce::String& key, double value);
    void setString(const juce::String& key, const juce::String& value);

    void removeValue(const juce::String& key);

private:
    std::shared_ptr<juce::PropertiesFile> store_;
};

} // namespace bws::ui

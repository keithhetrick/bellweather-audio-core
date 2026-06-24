// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * @file BarometerWeatherPresetManager.h
 * @brief Weather Design System Preset Manager for BwsBarometer
 *
 * Barometer's shared WeatherPresetManager integration.
 *
 * Factory Presets (legacy parity - exact 10):
 * - Unity
 * - +6 dB Boost
 * - +12 dB Boost
 * - -6 dB Trim
 * - -12 dB Trim
 * - lip
 * - lip L
 * - lip R
 * - Full Left
 * - Full Right
 */

#include <bw_ui/preset_system/WeatherPresetManager.h>
#include <bw_juce_adapters/JucePresetJsonCodec.h>

namespace bws::barometer
{

// Forward declaration
class BarometerProcessor;

/**
 * @class BarometerWeatherPresetManager
 * @brief WeatherPresetManager implementation for BwsBarometer plugin.
 *
 * Weather Design System preset integration for a simple utility plugin with
 * stereo processing features.
 */
class BarometerWeatherPresetManager : public bws::weather::WeatherPresetManager
{
public:
    explicit BarometerWeatherPresetManager(BarometerProcessor& processor);

    //==========================================================================
    // WeatherPresetManager Override
    //==========================================================================

    /**
     * Create factory presets for Barometer.
     * Called automatically during initialization.
     */
    void createFactoryPresets() override;

    //==========================================================================
    // Legacy compatibility shims (index-based CRUD for editor/test compat)
    //==========================================================================

    /** Find user preset index by name. Returns -1 if not found. */
    int findUserPresetByName(const juce::String& name) const;

    /** Overwrite user preset at index with current processor state. */
    bool updateUserPreset(int index);

    /** Delete user preset by index. */
    bool deleteUserPreset(int index);

    /** Rename user preset by index. */
    bool renameUserPreset(int index, const juce::String& newName);

    /** Directory compat: delegate to Weather system. */
    juce::File getPresetDirectory() const;
    juce::File getDefaultPresetDirectory() const;
    bool hasCustomPresetDirectory() const;
    juce::File getCustomPresetDirectory() const;
    void setCustomPresetDirectory(const juce::File& directory);
    void clearCustomPresetDirectory();
    void loadUserPresets();

    /** One-time migration from legacy UserPresets.json to Weather format. */
    void migrateLegacyUserPresets();

    /** Migrate from an explicit legacy source file. Returns true only after safe archive. */
    bool migrateLegacyUserPresetsFromFile(const juce::File& legacyFile);

private:
    BarometerProcessor& barometerProcessor_;
    bws::adapters::JucePresetJsonCodec presetJsonCodec_;

    void configurePresetRepository();

    /**
     * Create a factory preset ValueTree state with the given parameter values.
     */
    juce::ValueTree createPresetState(float gain, bool phaseInvertL, bool phaseInvertR, bool phaseLink, float balance,
                                      bool monoEnabled, float width, bool swapLR, bool soloL, bool soloR,
                                      float outputTrim);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BarometerWeatherPresetManager)
};

} // namespace bws::barometer

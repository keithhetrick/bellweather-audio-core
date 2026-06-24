// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * =============================================================================
 * PresetManager.h - Legacy Plugin-Agnostic Preset Management System
 * =============================================================================
 *
 * Bellweather Studios - Shared Preset Infrastructure
 *
 * LEGACY PATH:
 *   New shared plugin preset work should use bw_preset_core +
 *   bw_juce_adapters + bw_ui/preset_system/WeatherPresetManager. This manager
 *   remains for component-style/plugin integrations that still use the
 *   direct manager API.
 *
 * Features:
 *   - CRUD operations: Create, Read, Update, Delete, Rename
 *   - Custom preset directory support (external drives, cloud folders)
 *   - Platform-specific default locations
 *   - JSON persistence with versioning
 *   - Thread-safe for UI operations
 *
 * Usage:
 *   1. Create a subclass implementing getCurrentState() and applyState()
 *   2. Instantiate with plugin name in processor constructor
 *   3. Call loadUserPresets() after construction
 *   4. Delegate CRUD operations from processor to PresetManager
 *
 * Example:
 *   class MyPresetManager : public bws::PresetManager {
 *   public:
 * MyPresetManager(Processor& p): PresetManager("Plugin"), proc_(p) {}
 *   protected:
 *       juce::var getCurrentState() const override { ... }
 *       void applyState(const juce::var& state) override { ... }
 *   private:
 * Processor& proc_;
 *   };
 *
 * =============================================================================
 */

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <vector>
#include <functional>

namespace bws
{

/**
 * User preset data structure.
 * Stores name, timestamp, and plugin-specific parameter state.
 */
struct UserPreset
{
    juce::String name;
    juce::var state; // Plugin-specific parameter state (flexible JSON-like structure)
    juce::Time timestamp;

    bool isValid() const { return name.isNotEmpty(); }
};

/**
 * Abstract preset manager base class.
 *
 * Provides complete preset management infrastructure:
 * - Location management (default + custom directories)
 * - CRUD operations with JSON persistence
 * - Settings persistence for custom directory preference
 *
 * Plugins subclass this and implement two methods:
 * - getCurrentState(): Serialize current parameters to juce::var
 * - applyState(): Deserialize juce::var back to parameters
 */
class PresetManager
{
public:
    // =========================================================================
    // Construction
    // =========================================================================

    /**
     * Create a PresetManager for the specified plugin.
     *
     * @param pluginName Unique plugin identifier (e.g., "BwsBarometer", "")
     *                    Used for directory naming and JSON metadata.
     */
    explicit PresetManager(const juce::String& pluginName);

    virtual ~PresetManager() = default;

    // =========================================================================
    // Location Management
    // =========================================================================

    /**
     * Get the current preset directory.
     * Returns custom directory if set, otherwise platform-specific default.
     */
    juce::File getPresetDirectory() const;

    /**
     * Get the default preset directory (platform-specific).
     * - macOS: ~/Library/Application Support/BellweatherStudios/{pluginName}
     * - Windows: %APPDATA%/BellweatherStudios/{pluginName}
     * - Linux: ~/.config/BellweatherStudios/{pluginName}
     */
    juce::File getDefaultPresetDirectory() const;

    /**
     * Set a custom preset directory.
     * Automatically copies existing presets to the new location.
     *
     * @param directory  New preset directory (can be external drive, cloud folder, etc.)
     */
    void setCustomPresetDirectory(const juce::File& directory);

    /**
     * Reset to the default preset directory.
     * Clears custom directory setting but does NOT delete presets from custom location.
     */
    void clearCustomPresetDirectory();

    /**
     * Check if using a custom preset directory.
     */
    bool hasCustomPresetDirectory() const;

    /**
     * Get the custom preset directory (may be invalid if not set).
     */
    juce::File getCustomPresetDirectory() const;

    /**
     * Open the preset directory in Finder/Explorer.
     */
    void revealPresetDirectory() const;

    // =========================================================================
    // CRUD Operations
    // =========================================================================

    /**
     * Load all user presets from JSON file.
     * Call this in processor constructor after PresetManager is created.
     */
    void loadUserPresets();

    /**
     * Save a new preset with the current parameters.
     * If a preset with this name exists, it will be overwritten.
     *
     * @param name  Preset name (must not be empty)
     * @return true if saved successfully
     */
    bool saveUserPreset(const juce::String& name);

    /**
     * Load a preset by index.
     *
     * @param index  Zero-based index into user presets
     * @return true if loaded successfully
     */
    bool loadUserPreset(int index);

    /**
     * Update an existing preset with current parameters.
     *
     * @param index  Zero-based index into user presets
     * @return true if updated successfully
     */
    bool updateUserPreset(int index);

    /**
     * Delete a preset by index.
     *
     * @param index  Zero-based index into user presets
     * @return true if deleted successfully
     */
    bool deleteUserPreset(int index);

    /**
     * Rename a preset.
     *
     * @param index    Zero-based index into user presets
     * @param newName  New name for the preset
     * @return true if renamed successfully
     */
    bool renameUserPreset(int index, const juce::String& newName);

    /**
     * Find a preset by name (case-insensitive).
     *
     * @param name  Preset name to search for
     * @return Index of preset, or -1 if not found
     */
    int findUserPresetByName(const juce::String& name) const;

    // =========================================================================
    // Accessors
    // =========================================================================

    /**
     * Get all user presets.
     */
    const std::vector<UserPreset>& getUserPresets() const { return userPresets_; }

    /**
     * Get the number of user presets.
     */
    int getUserPresetCount() const { return static_cast<int>(userPresets_.size()); }

    /**
     * Get preset names as a StringArray (for UI dropdowns).
     */
    juce::StringArray getUserPresetNames() const;

    /**
     * Get the plugin name this manager was created with.
     */
    const juce::String& getPluginName() const { return pluginName_; }

    // =========================================================================
    // Preset Info (for PresetBrowser compatibility)
    // =========================================================================

    /**
     * Preset info structure for PresetBrowser UI component.
     */
    struct PresetInfo
    {
        juce::String name;
        juce::String category; // "Factory" or "User"
        juce::String path;     // For user presets: index as string; for factory: index as string
    };

    /**
     * Get combined list of factory + user presets for browser UI.
     * Factory presets should be added via registerFactoryPresets().
     */
    std::vector<PresetInfo> getPresetList() const;

    /**
     * Load a preset by path (used by PresetBrowser).
     * Path format: "factory:INDEX" or "user:INDEX"
     */
    bool loadPreset(const juce::String& path);

    // =========================================================================
    // Factory Presets Support
    // =========================================================================

    /**
     * Register factory presets for this plugin.
     * Call this from the processor constructor before loadUserPresets().
     *
     * @param presets  Vector of preset info with names and states
     */
    void registerFactoryPresets(std::vector<std::pair<juce::String, juce::var>> presets);

    /**
     * Load a factory preset by index.
     */
    bool loadFactoryPreset(int index);

    /**
     * Get factory preset names.
     */
    juce::StringArray getFactoryPresetNames() const;

    /**
     * Get the number of factory presets.
     */
    int getFactoryPresetCount() const { return static_cast<int>(factoryPresets_.size()); }

protected:
    // =========================================================================
    // Plugin-Specific Overrides (REQUIRED)
    // =========================================================================

    /**
     * Get the current plugin state as a juce::var.
     *
     * Implementation should serialize all relevant parameters.
     * The returned var should be a DynamicObject with named properties.
     *
     * Example:
     *   juce::DynamicObject::Ptr obj = new juce::DynamicObject();
     *   obj->setProperty("gain", getGain());
     *   obj->setProperty("phase", getPhaseInvert());
     *   return juce::var(obj.get());
     */
    virtual juce::var getCurrentState() const = 0;

    /**
     * Apply a preset state to the plugin.
     *
     * Implementation should deserialize and apply parameters.
     *
     * Example:
     *   if (state.isObject()) {
     *       auto* obj = state.getDynamicObject();
     *       setGain(obj->getProperty("gain"));
     *       setPhaseInvert(obj->getProperty("phase"));
     *   }
     */
    virtual void applyState(const juce::var& state) = 0;

private:
    // =========================================================================
    // Internal Implementation
    // =========================================================================

    juce::File getSettingsFile() const;
    void loadCustomPresetDirectorySetting();
    void saveCustomPresetDirectorySetting();
    void saveUserPresetsToFile();

    // =========================================================================
    // Members
    // =========================================================================

    juce::String pluginName_;
    std::vector<UserPreset> userPresets_;
    juce::File customPresetDirectory_;

    // Factory presets (optional)
    std::vector<std::pair<juce::String, juce::var>> factoryPresets_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};

} // namespace bws

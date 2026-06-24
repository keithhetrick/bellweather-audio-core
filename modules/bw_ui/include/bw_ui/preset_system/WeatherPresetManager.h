// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * @file WeatherPresetManager.h
 * @brief FabFilter-quality Preset Management System - Weather Design System
 *
 * Bellweather Studios - Reusable Preset Infrastructure
 *
 * Features:
 *   - CRUD operations with category support
 *   - Factory presets from authored assets or plugin-specific declarations
 *   - Repository-backed user presets with metadata (author, description)
 *   - Navigation (next/previous cycling)
 *   - Unsaved changes tracking
 *   - Multi-listener state change notifications
 *   - Platform-specific preset directories through adapter-backed storage
 *
 * Usage:
 *   class MyPresetManager : public bws::weather::WeatherPresetManager {
 *   public:
 * MyPresetManager(juce::Processor& p)
 *WeatherPresetManager(p, "Plugin") {}
 *       void createFactoryPresets() override { ... }
 *   };
 */

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>
#include <bw_ports/IPresetStatePersistence.h>
#include <bw_preset_core/IPresetStateBridge.h>
#include <bw_preset_core/PresetLibraryKey.h>
#include <bw_preset_core/PresetLibraryState.h>
#include <bw_preset_core/PresetNavigation.h>
#include <bw_preset_core/PresetRepository.h>
#include <vector>
#include <set>
#include <memory>
#include <limits>

namespace bws::weather
{

/**
 * @class WeatherPresetManager
 * @brief Professional preset management with category support and change tracking.
 *
 * Subclass this for each plugin and implement createFactoryPresets().
 */
class WeatherPresetManager : public bws::IPresetStatePersistence
{
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void weatherPresetChanged() {}
        virtual void weatherPresetUnsavedChangesChanged(bool) {}
        virtual void weatherPresetListChanged() {}
    };

    //==========================================================================
    // Preset Data Structure
    //==========================================================================

    /**
     * @struct Preset
     * @brief Complete preset metadata and state.
     */
    struct Preset
    {
        enum class Format
        {
            Json,
            LegacyXml,
            InMemoryFactory
        };

        enum class Intensity
        {
            Unknown,
            Subtle,
            Balanced,
            Assertive,
            Extreme
        };

        juce::String name;
        juce::String category;
        juce::ValueTree state;
        bool isFactory = false;
        juce::String presetId;
        juce::String engine;
        juce::String useCase;
        Intensity intensity = Intensity::Unknown;
        juce::StringArray tags;
        bool isCleanStarter = false;
        juce::String author;
        juce::String description;
        juce::Time timestamp;
        juce::File file;
        int sortOrder = std::numeric_limits<int>::max();
        Format format = Format::InMemoryFactory;
        bool isFavorite = false;
        bool isStartupDefault = false;
        int recentRank = -1;

        /** Get the file associated with this preset (if saved). */
        juce::File getFile() const;

        /** Check if this is a valid preset. */
        bool isValid() const { return name.isNotEmpty() && state.isValid(); }

        /** True when this preset has an authored stable factory ID. */
        bool hasStableFactoryId() const { return presetId.isNotEmpty(); }
    };

    struct FilterCriteria
    {
        enum class Source
        {
            All,
            Factory,
            User
        };

        enum class Curation
        {
            All,
            Favorites,
            CleanStarters
        };

        juce::String searchText;
        Source source = Source::All;
        Curation curation = Curation::All;
        juce::String engine;
        juce::String useCase;
        Preset::Intensity intensity = Preset::Intensity::Unknown;
    };

    using PresetIdentity = bws::preset::PresetLibraryIdentity;

    //==========================================================================
    // Construction
    //==========================================================================

    /**
     * Create a WeatherPresetManager for the specified plugin.
     *
     * @param processor Reference to the host audio processor, for state access.
     * @param pluginName Unique plugin identifier used to scope preset storage.
     */
    WeatherPresetManager(juce::AudioProcessor& processor, const juce::String& pluginName);

    ~WeatherPresetManager() override;

    //==========================================================================
    // Core Operations
    //==========================================================================

    /** Override to return false when preset saving should be blocked (e.g. trial mode).
     *  Default returns true (saving allowed). */
    virtual bool canSavePreset() const { return true; }

    /**
     * Save current processor state as a new preset.
     * Returns false immediately if canSavePreset() returns false.
     *
     * @param name        Preset name (must not be empty)
     * @param category    Category for organization (e.g., "Mix Bus", "Vocals")
     * @param author      Optional author name
     * @param description Optional description
     * @return true if saved successfully
     */
    bool savePreset(const juce::String& name, const juce::String& category, const juce::String& author = "",
                    const juce::String& description = "");

    /**
     * Load a preset by name.
     *
     * @param name  Preset name to load
     * @return true if loaded successfully
     */
    bool loadPreset(const juce::String& name);

    /**
     * Load a preset by index.
     *
     * @param index  Zero-based index into presets
     * @return true if loaded successfully
     */
    virtual bool loadPresetByIndex(int index);

    /**
     * Delete a user preset by name.
     * Factory presets cannot be deleted.
     *
     * @param name  Preset name to delete
     * @return true if deleted successfully
     */
    bool deletePreset(const juce::String& name);

    /**
     * Rename a user preset.
     * Factory presets cannot be renamed.
     *
     * @param oldName  Current preset name
     * @param newName  New preset name
     * @return true if renamed successfully
     */
    bool renamePreset(const juce::String& oldName, const juce::String& newName);

    //==========================================================================
    // Navigation
    //==========================================================================

    /** Load the next preset in the list (wraps around). */
    void loadNextPreset();

    /** Load the previous preset in the list (wraps around). */
    void loadPreviousPreset();

    /** Get the index of the currently loaded preset (-1 if none). */
    int getCurrentPresetIndex() const;

    //==========================================================================
    // Query
    //==========================================================================

    /** Get all presets (factory + user). */
    const std::vector<Preset>& getAllPresets() const { return presets_; }

    /** Get all unique categories. */
    std::vector<juce::String> getCategories() const;

    /** Get all presets in a specific category. */
    std::vector<Preset> getPresetsInCategory(const juce::String& category) const;

    /** Get the currently loaded preset. */
    const Preset& getCurrentPreset() const { return currentPreset_; }

    /** Check if the current state has unsaved changes. */
    bool hasUnsavedChanges() const { return hasUnsavedChanges_ || hasPendingDirtyTrackingChange(); }

    /** True while a reversible preset-browser session is active. */
    bool isPreviewSessionActive() const { return previewSession_.active; }

    /** Get the number of presets. */
    int getPresetCount() const { return static_cast<int>(presets_.size()); }

    /** Get the plugin name. */
    const juce::String& getPluginName() const { return pluginName_; }

    /** Query preset indices using the shared discovery/filter semantics. */
    std::vector<int> queryPresetIndices(const FilterCriteria& criteria) const;

    /** Shared logical identity for preset-manager and browser state. */
    PresetIdentity getPresetIdentity(const Preset& preset) const;
    juce::String getPresetIdentityKey(const Preset& preset) const;

    /** Available discovery metadata values from the current library. */
    juce::StringArray getAvailableEngines() const;
    juce::StringArray getAvailableUseCases() const;
    juce::StringArray getAvailableIntensityLabels() const;

    /** Favorites / library state helpers. */
    bool isPresetFavorite(int presetIndex) const;
    bool setPresetFavorite(int presetIndex, bool shouldBeFavorite);
    bool togglePresetFavorite(int presetIndex);
    const std::vector<juce::String>& getRecentPresetKeys() const { return recentPresetKeys_; }

    /** Startup default helpers. */
    bool hasStartupDefaultPreset() const;
    bool isCurrentPresetStartupDefault() const;
    bool setStartupDefaultPreset(int presetIndex);
    bool clearStartupDefaultPreset();

    //==========================================================================
    // Backward Compatibility (Legacy PresetManager API)
    //==========================================================================

    /** Get factory preset names (for dropdown UI). */
    juce::StringArray getFactoryPresetNames() const;

    /** Get user preset names (for dropdown UI). */
    juce::StringArray getUserPresetNames() const;

    /** Get the number of factory presets. */
    int getFactoryPresetCount() const;

    /** Get the number of user presets. */
    int getUserPresetCount() const;

    /** Load a factory preset by index. */
    bool loadFactoryPreset(int index);

    /** Load a user preset by index. */
    bool loadUserPreset(int index);

    /** Save a user preset (simplified - uses "User" category). */
    bool saveUserPreset(const juce::String& name);

    //==========================================================================
    // Reversible Preview Session
    //==========================================================================

    /** Capture a reversible browsing snapshot for preset preview/cancel. */
    bool beginPreviewSession();

    /** Preview a preset by global preset index without committing the session. */
    bool previewPresetByIndex(int index);

    /** Finalize the current preview session, keeping the current live preset. */
    bool commitPreviewSession();

    /** Restore the captured state from the current preview session. */
    bool cancelPreviewSession();

    //==========================================================================
    // Factory Presets (Override per plugin)
    //==========================================================================

    /**
     * Create factory presets for this plugin.
     * Called from initialize() after the subclass has finished construction.
     * Override in derived classes.
     */
    virtual void createFactoryPresets() = 0;

    //==========================================================================
    // State Change Notifications
    //==========================================================================

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    //==========================================================================
    // Directories
    //==========================================================================

    /** Get the user preset directory. */
    juce::File getUserPresetDirectory() const;

    /** Get the default user preset directory (without user override). */
    juce::File getDefaultUserPresetDirectory() const;

    /** Get the factory preset directory. */
    juce::File getFactoryPresetDirectory() const;

    /** Set a custom user preset directory override (persisted). */
    bool setUserPresetDirectoryOverride(const juce::File& directory);

    /** Clear the custom user preset directory override (revert to default). */
    void clearUserPresetDirectoryOverride();

    /** True if a custom user preset directory override is currently active. */
    bool isUsingCustomUserPresetDirectory() const;

    /** Open the preset folder in system file browser. */
    void openPresetFolder() const;

    //==========================================================================
    // State Management
    //==========================================================================

    /** Mark the current state as modified (e.g., when a parameter changes). */
    void markAsModified();

    /** Attach a state bridge after construction so subclasses keep safe member ordering. */
    void setPresetStateBridge(std::unique_ptr<bws::preset::IPresetStateBridge> bridge);

    /** Detach the currently attached state bridge. */
    void clearPresetStateBridge();

    /** Attach the core preset repository used for storage/discovery. */
    void setPresetRepository(std::unique_ptr<bws::preset::IPresetRepository> repository);

    /** Deterministically flush pending dirty tracking work for tests/headless callers. */
    void flushPendingDirtyTrackingForTesting();

    /** Refresh the preset list by rescanning directories. */
    void refreshPresetList();

    /** Initialize - call after construction to set up factory presets. */
    void initialize();

    //==========================================================================
    // Centralized State Management Helpers
    //==========================================================================

    /** Return current preset metadata for state persistence.
     * Called by::getStateInformation.*/
    [[nodiscard]] bws::domain::BwResult<bws::domain::PresetMetadata> getPresetMetadata() const override;

    /** Restore preset metadata from serialized state blob.
     * Called by::setStateInformation.*/
    [[nodiscard]] bws::domain::BwResult<void> restorePresetState(bws::domain::BwStateBlob blob) override;

    void beginPresetStateRestore() override;
    void endPresetStateRestore() override;

    /**
     * Set current preset selection without loading state.
     * Used when restoring plugin state where parameters are already set.
     *
     * @param name        Preset name to select
     * @param category    Preset category (optional, for disambiguation)
     * @param wasModified Whether the state was modified from the preset
     * @param presetId    Stable preset identifier (optional)
     * @return true if preset was found in the list
     */
    bool selectPresetForRestore(const juce::String& name, const juce::String& category = "", bool wasModified = false,
                                const juce::String& presetId = "");

protected:
    juce::AudioProcessor& processor_;
    juce::String pluginName_;

    Preset currentPreset_;
    std::vector<Preset> presets_;
    bool hasUnsavedChanges_ = false;
    bool isInitialized_ = false;

    /** Scan directories for preset files. */
    void scanForPresets();

    /** Sort presets using shared catalog policy: source, sort order, metadata, recency. */
    void sortPresets();

    /**
     * Helper to add a factory preset.
     * Call from createFactoryPresets() implementation.
     */
    void addFactoryPreset(const juce::String& name, const juce::String& category, const juce::ValueTree& state,
                          const juce::String& description = "");

    /**
     * Helper to create a ValueTree state from current processor state.
     * Override if you need custom state serialization.
     */
    virtual juce::ValueTree captureProcessorState() const;

    /**
     * Helper to apply a ValueTree state to the processor.
     * Override if you need custom state deserialization.
     */
    virtual void applyProcessorState(const juce::ValueTree& state);

    /** Resolve bundled factory asset roots for runtime discovery. */
    virtual std::vector<juce::File> getBundledFactoryAssetRoots() const;
    virtual bool readLibraryStateDocument(juce::var& outDocument) const;
    virtual bool writeLibraryStateDocument(const juce::var& document) const;

private:
    enum class LoadReason
    {
        DirectCommit,
        Preview,
        StartupDefault
    };

    struct PreviewSessionState
    {
        bool active = false;
        juce::ValueTree snapshot;
        Preset committedPreset;
        juce::File committedPresetFile;
        bool committedHadUnsavedChanges = false;
    };

    /** Internal file path tracking for saved presets. */
    juce::File currentPresetFile_;

    bool suppressCallbacks_ = false;
    juce::ListenerList<Listener> listeners_;
    mutable std::unique_ptr<juce::PropertiesFile> propertiesFile_;
    mutable juce::String cachedPresetName_;
    mutable juce::String cachedPresetCategory_;
    mutable juce::String cachedPresetId_;
    PreviewSessionState previewSession_;
    std::vector<Preset> registeredFactoryPresets_;
    bws::preset::PresetLibraryState libraryState_;
    std::vector<juce::String> recentPresetKeys_;
    std::unique_ptr<bws::preset::IPresetStateBridge> presetStateBridge_;
    std::unique_ptr<bws::preset::IPresetRepository> presetRepository_;
    bws::preset::PresetStateSubscription presetStateSubscription_;
    std::vector<bws::preset::ScopedPresetStateSuppression> activeRestoreSuppressions_;

    class ScopedDirtyTrackingSuppression;
    class ScopedCallbackSuppression;

    juce::PropertiesFile* getPropertiesFile() const;
    juce::File getLibraryStateFile() const;
    void loadLibraryState();
    bool saveLibraryState() const;
    bool saveLibraryState(const bws::preset::PresetLibraryState& state) const;
    void refreshRecentPresetKeyCache();
    void applyLibraryStateToPresets();
    juce::String makePresetLibraryKey(const Preset& preset) const;
    void updateRecentsForPreset(const Preset& preset);
    void clearInvalidLibraryStateReferences();
    void notifyPresetChanged();
    void notifyUnsavedChangesChanged(bool hasChanges);
    void notifyPresetListChanged();
    bool hasPendingDirtyTrackingChange() const;
    void resetDirtyTrackingState(bool eligible);
    bool loadPresetByDirection(bws::preset::PresetNavigationDirection direction);
    bool loadPresetByIndexInternal(int index, LoadReason reason);
    bws::preset::PresetStateBytes capturePresetStateBytes() const;
    bool decodeBase64State(const juce::String& base64, juce::MemoryOutputStream& stream) const;
    int findUniquePresetIndexByName(const juce::String& name) const;
    int findUniqueUserPresetIndexByName(const juce::String& name) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WeatherPresetManager)
};

} // namespace bws::weather

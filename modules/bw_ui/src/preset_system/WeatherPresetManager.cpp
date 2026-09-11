// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/preset_system/WeatherPresetManager.h"

#include "WeatherPresetLibraryStateStore.h"
#include "WeatherPresetRecordAdapter.h"

#include <bw_preset_core/PresetCatalog.h>
#include <bw_preset_core/PresetDiscoveryPolicy.h>
#include <bw_preset_core/PresetFactoryPolicy.h>
#include <bw_preset_core/PresetIdentity.h>
#include <bw_preset_core/PresetLibraryKey.h>
#include <bw_preset_core/PresetLibraryState.h>
#include <bw_preset_core/PresetMetadataPolicy.h>
#include <bw_preset_core/PresetNavigation.h>
#include <bw_preset_core/PresetUuid.h>

#include <juce_cryptography/juce_cryptography.h>

#include <cstdlib>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <tuple>

#if JUCE_WINDOWS
#include <windows.h>
#elif JUCE_MAC || JUCE_LINUX
#include <dlfcn.h>
#endif

namespace bws::weather
{
namespace
{
bws::preset::PresetCatalogSourceFilter toCoreCatalogSourceFilter(WeatherPresetManager::FilterCriteria::Source source)
{
    using Source = WeatherPresetManager::FilterCriteria::Source;
    switch (source)
    {
    case Source::Factory:
        return bws::preset::PresetCatalogSourceFilter::Factory;
    case Source::User:
        return bws::preset::PresetCatalogSourceFilter::User;
    case Source::All:
        break;
    }

    return bws::preset::PresetCatalogSourceFilter::All;
}

bws::preset::PresetCatalogCurationFilter toCoreCatalogCurationFilter(
    WeatherPresetManager::FilterCriteria::Curation curation)
{
    using Curation = WeatherPresetManager::FilterCriteria::Curation;
    switch (curation)
    {
    case Curation::Favorites:
        return bws::preset::PresetCatalogCurationFilter::Favorites;
    case Curation::CleanStarters:
        return bws::preset::PresetCatalogCurationFilter::CleanStarters;
    case Curation::All:
        break;
    }

    return bws::preset::PresetCatalogCurationFilter::All;
}

bws::preset::PresetCatalogQuery toCoreCatalogQuery(const WeatherPresetManager::FilterCriteria& criteria)
{
    return {criteria.searchText.toStdString(),
            toCoreCatalogSourceFilter(criteria.source),
            toCoreCatalogCurationFilter(criteria.curation),
            criteria.engine.toStdString(),
            criteria.useCase.toStdString(),
            toCorePresetIntensity(criteria.intensity)};
}

std::vector<bws::preset::PresetCatalogEntry> toCoreCatalogEntries(
    const juce::String& pluginName, const std::vector<WeatherPresetManager::Preset>& presets)
{
    std::vector<bws::preset::PresetCatalogEntry> entries;
    entries.reserve(presets.size());
    for (int i = 0; i < static_cast<int>(presets.size()); ++i)
    {
        const auto& preset = presets[static_cast<size_t>(i)];
        entries.push_back({static_cast<std::size_t>(i), toCorePresetMetadata(pluginName, preset), preset.isFavorite,
                           preset.recentRank});
    }
    return entries;
}

void appendUniqueDirectory(std::vector<juce::File>& directories, const juce::File& directory)
{
    if (!directory.isDirectory())
        return;

    const auto path = directory.getFullPathName();
    const auto it = std::find_if(directories.begin(), directories.end(),
                                 [&path](const juce::File& candidate) { return candidate.getFullPathName() == path; });
    if (it == directories.end())
        directories.push_back(directory);
}

void appendFactoryDirectoryCandidates(std::vector<juce::File>& directories, const juce::File& base,
                                      const juce::String& pluginName)
{
    if (base == juce::File())
        return;

    const auto directFactory = base.getFileName().equalsIgnoreCase("Factory") ? base : juce::File();

    if (directFactory != juce::File())
        appendUniqueDirectory(directories, directFactory);

    appendUniqueDirectory(directories, base.getChildFile("Factory"));
    appendUniqueDirectory(directories, base.getChildFile("Presets").getChildFile("Factory"));
    appendUniqueDirectory(directories, base.getChildFile("assets").getChildFile("Presets").getChildFile("Factory"));
    appendUniqueDirectory(directories, base.getChildFile("Resources").getChildFile("Presets").getChildFile("Factory"));
    appendUniqueDirectory(
        directories,
        base.getChildFile("Contents").getChildFile("Resources").getChildFile("Presets").getChildFile("Factory"));
    appendUniqueDirectory(directories, base.getChildFile(pluginName).getChildFile("Presets").getChildFile("Factory"));
}

bws::preset::PresetStateBytes withEmbeddedPresetMetadata(const bws::preset::PresetStateBytes& stateBytes,
                                                         const juce::String& presetName,
                                                         const juce::String& presetCategory,
                                                         const juce::String& presetId, bool wasModified)
{
    if (stateBytes.empty())
        return stateBytes;

    constexpr std::size_t cap = 3u * 1024u * 1024u;
    constexpr std::size_t frameOverhead = 9u;
    if (stateBytes.size() < frameOverhead || stateBytes.size() > cap)
        return {};
    const auto* framed = reinterpret_cast<const std::uint8_t*>(stateBytes.data());
    if (framed[0] != 'V' || framed[1] != 'C' || framed[2] != '2' || framed[3] != '!')
        return {};
    std::uint32_t payloadSize = 0;
    for (std::size_t byte = 0; byte < 4; ++byte)
        payloadSize |= static_cast<std::uint32_t>(framed[4 + byte]) << (byte * 8u);
    if (payloadSize == 0u || payloadSize > cap - frameOverhead ||
        static_cast<std::size_t>(payloadSize) + frameOverhead != stateBytes.size() || framed[8u + payloadSize] != 0u ||
        std::memchr(framed + 8u, 0, payloadSize) != nullptr)
        return {};

    auto xml = juce::AudioProcessor::getXmlFromBinary(stateBytes.data(), static_cast<int>(stateBytes.size()));
    if (xml == nullptr)
        return {};

    xml->setAttribute("currentPresetName", presetName);
    xml->setAttribute("currentPresetCategory", presetCategory);
    xml->setAttribute("presetWasModified", wasModified);

    if (presetId.isNotEmpty())
        xml->setAttribute("currentPresetId", presetId);
    else
        xml->removeAttribute("currentPresetId");

    juce::MemoryBlock normalizedState(cap, true);
    juce::MemoryOutputStream output(normalizedState.getData(), normalizedState.getSize());
    constexpr std::array<std::byte, 8> header {std::byte {'V'}, std::byte {'C'}, std::byte {'2'}, std::byte {'!'},
                                               std::byte {0},   std::byte {0},   std::byte {0},   std::byte {0}};
    if (!output.write(header.data(), header.size()))
        return {};
    xml->writeTo(output, juce::XmlElement::TextFormat().singleLine());
    if (!output.writeByte(0))
        return {};
    const auto totalSize = output.getDataSize();
    if (totalSize < frameOverhead || totalSize > cap)
        return {};
    const auto rewrittenPayloadSize = totalSize - frameOverhead;
    if (rewrittenPayloadSize == 0u || rewrittenPayloadSize > UINT32_MAX)
        return {};
    auto* rewritten = static_cast<std::byte*>(normalizedState.getData());
    const auto payload32 = static_cast<std::uint32_t>(rewrittenPayloadSize);
    for (std::size_t byte = 0; byte < 4; ++byte)
        rewritten[4 + byte] = static_cast<std::byte>((payload32 >> (byte * 8u)) & 0xffu);
    normalizedState.setSize(totalSize, false);
    return bws::preset::PresetStateBytes(normalizedState.getData(), normalizedState.getSize());
}

juce::File getCurrentModuleFile()
{
#if JUCE_WINDOWS
    HMODULE moduleHandle = nullptr;
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCWSTR>(&getCurrentModuleFile), &moduleHandle) != 0)
    {
        wchar_t path[MAX_PATH] = {};
        if (GetModuleFileNameW(moduleHandle, path, static_cast<DWORD>(std::size(path))) > 0)
            return juce::File(juce::String(path));
    }
#elif JUCE_MAC || JUCE_LINUX
    Dl_info info {};
    if (dladdr(reinterpret_cast<const void*>(&getCurrentModuleFile), &info) != 0 && info.dli_fname != nullptr)
    {
        return juce::File(juce::String::fromUTF8(info.dli_fname));
    }
#endif

    return {};
}
} // namespace

//==============================================================================
// Preset Helper Methods
//==============================================================================

juce::File WeatherPresetManager::Preset::getFile() const
{
    return file;
}

//==============================================================================
// Construction & Initialization
//==============================================================================

void WeatherPresetManager::addListener(Listener* listener)
{
    listeners_.add(listener);
}

void WeatherPresetManager::removeListener(Listener* listener)
{
    listeners_.remove(listener);
}

void WeatherPresetManager::notifyPresetChanged()
{
    if (suppressCallbacks_)
        return;

    listeners_.call([](Listener& listener) { listener.weatherPresetChanged(); });
}

void WeatherPresetManager::notifyUnsavedChangesChanged(bool hasChanges)
{
    if (suppressCallbacks_)
        return;

    listeners_.call([hasChanges](Listener& listener) { listener.weatherPresetUnsavedChangesChanged(hasChanges); });
}

void WeatherPresetManager::notifyPresetListChanged()
{
    if (suppressCallbacks_)
        return;

    listeners_.call([](Listener& listener) { listener.weatherPresetListChanged(); });
}

WeatherPresetManager::WeatherPresetManager(juce::AudioProcessor& processor, const juce::String& pluginName)
    : processor_(processor)
    , pluginName_(pluginName)
{}

WeatherPresetManager::~WeatherPresetManager()
{
    clearPresetStateBridge();
}

class WeatherPresetManager::ScopedDirtyTrackingSuppression
{
public:
    explicit ScopedDirtyTrackingSuppression(WeatherPresetManager& manager)
        : manager_(manager)
        , suppression_(manager_.presetStateBridge_ != nullptr
                           ? manager_.presetStateBridge_->suppressDirtyNotifications()
                           : bws::preset::ScopedPresetStateSuppression())
    {}

    ~ScopedDirtyTrackingSuppression() {}

private:
    WeatherPresetManager& manager_;
    bws::preset::ScopedPresetStateSuppression suppression_;
};

class WeatherPresetManager::ScopedCallbackSuppression
{
public:
    explicit ScopedCallbackSuppression(WeatherPresetManager& manager)
        : manager_(manager)
        , previous_(manager_.suppressCallbacks_)
    {
        manager_.suppressCallbacks_ = true;
    }

    ~ScopedCallbackSuppression() { manager_.suppressCallbacks_ = previous_; }

private:
    WeatherPresetManager& manager_;
    bool previous_ = false;
};

void WeatherPresetManager::initialize()
{
    if (isInitialized_)
        return;

    getUserPresetDirectory().createDirectory();
    getLibraryStateFile().getParentDirectory().createDirectory();

    registeredFactoryPresets_.clear();
    createFactoryPresets();
    loadLibraryState();
    scanForPresets();

    if (!presets_.empty() && shouldLoadStartupPreset())
    {
        int initialIndex = 0;
        if (!libraryState_.startupPresetKey.empty())
        {
            for (int i = 0; i < static_cast<int>(presets_.size()); ++i)
            {
                if (makePresetLibraryKey(presets_[static_cast<size_t>(i)]).toStdString() ==
                    libraryState_.startupPresetKey)
                {
                    initialIndex = i;
                    break;
                }
            }
        }
        loadPresetByIndexInternal(initialIndex, LoadReason::StartupDefault);
    }

    if (shouldActivateStateBridgeDuringInitialize() && !activatePresetStateBridge())
        return;

    isInitialized_ = true;
}

//==============================================================================
// Directory Management
//==============================================================================

juce::File WeatherPresetManager::getUserPresetDirectory() const
{
    if (auto* props = getPropertiesFile())
    {
        const auto overridePath = props->getValue("userPresetDirectoryOverride");
        if (overridePath.isNotEmpty())
            return juce::File(overridePath);
    }

    return getDefaultUserPresetDirectory();
}

juce::File WeatherPresetManager::getDefaultUserPresetDirectory() const
{
    auto appData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);

#if JUCE_MAC
    return appData.getChildFile("BellweatherStudios")
        .getChildFile(pluginName_)
        .getChildFile("Presets")
        .getChildFile("User");
#elif JUCE_WINDOWS
    return appData.getChildFile("BellweatherStudios")
        .getChildFile(pluginName_)
        .getChildFile("Presets")
        .getChildFile("User");
#else
    auto configDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile(".config");
    return configDir.getChildFile("BellweatherStudios")
        .getChildFile(pluginName_)
        .getChildFile("Presets")
        .getChildFile("User");
#endif
}

std::vector<juce::File> WeatherPresetManager::getBundledFactoryAssetRoots() const
{
    std::vector<juce::File> roots;

    if (const char* env = std::getenv("BWS_PRESET_ASSET_ROOT"))
        appendFactoryDirectoryCandidates(roots, juce::File(juce::String::fromUTF8(env)), pluginName_);

    const auto moduleFile = getCurrentModuleFile();
    auto moduleCursor = moduleFile;
    for (int depth = 0; depth < 8 && moduleCursor != juce::File(); ++depth)
    {
        appendFactoryDirectoryCandidates(roots, moduleCursor, pluginName_);
        moduleCursor = moduleCursor.getParentDirectory();
    }

    const auto currentExecutable = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    auto executableCursor = currentExecutable;
    for (int depth = 0; depth < 8 && executableCursor != juce::File(); ++depth)
    {
        appendFactoryDirectoryCandidates(roots, executableCursor, pluginName_);
        executableCursor = executableCursor.getParentDirectory();
    }

    const auto currentApp = juce::File::getSpecialLocation(juce::File::currentApplicationFile);
    auto cursor = currentApp;
    for (int depth = 0; depth < 8 && cursor != juce::File(); ++depth)
    {
        appendFactoryDirectoryCandidates(roots, cursor, pluginName_);
        cursor = cursor.getParentDirectory();
    }

    const auto workingDirectory = juce::File::getCurrentWorkingDirectory();
    appendFactoryDirectoryCandidates(
        roots, workingDirectory.getChildFile("plugins").getChildFile("production").getChildFile(pluginName_),
        pluginName_);

    return roots;
}

juce::File WeatherPresetManager::getFactoryPresetDirectory() const
{
    const auto bundledRoots = getBundledFactoryAssetRoots();
    if (!bundledRoots.empty())
        return bundledRoots.front();

    auto appData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);

#if JUCE_MAC
    return appData.getChildFile("BellweatherStudios")
        .getChildFile(pluginName_)
        .getChildFile("Presets")
        .getChildFile("Factory");
#elif JUCE_WINDOWS
    return appData.getChildFile("BellweatherStudios")
        .getChildFile(pluginName_)
        .getChildFile("Presets")
        .getChildFile("Factory");
#else
    auto configDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile(".config");
    return configDir.getChildFile("BellweatherStudios")
        .getChildFile(pluginName_)
        .getChildFile("Presets")
        .getChildFile("Factory");
#endif
}

bool WeatherPresetManager::setUserPresetDirectoryOverride(const juce::File& directory)
{
    if (directory == juce::File())
        return false;

    if (!directory.exists() && !directory.createDirectory())
        return false;

    if (auto* props = getPropertiesFile())
    {
        props->setValue("userPresetDirectoryOverride", directory.getFullPathName());
        props->saveIfNeeded();
    }

    refreshPresetList();
    return true;
}

void WeatherPresetManager::clearUserPresetDirectoryOverride()
{
    if (auto* props = getPropertiesFile())
    {
        props->removeValue("userPresetDirectoryOverride");
        props->saveIfNeeded();
    }

    refreshPresetList();
}

bool WeatherPresetManager::isUsingCustomUserPresetDirectory() const
{
    if (auto* props = getPropertiesFile())
        return props->getValue("userPresetDirectoryOverride").isNotEmpty();
    return false;
}

void WeatherPresetManager::openPresetFolder() const
{
    getUserPresetDirectory().revealToUser();
}

//==============================================================================
// Core Operations - Save
//==============================================================================

bool WeatherPresetManager::savePreset(const juce::String& name, const juce::String& category,
                                      const juce::String& author, const juce::String& description)
{
    if (!canSavePreset() || name.isEmpty())
        return false;

    // Capture before allocating identity/timestamp metadata or touching the
    // repository. Empty is never a valid processor state.
    const auto capturedStateBytes = capturePresetStateBytes();
    if (capturedStateBytes.empty())
        return false;

    Preset preset;
    preset.name = name;
    preset.category = presetCategoryOrDefault(category, bws::preset::kDefaultUserPresetCategory);
    preset.author = presetAuthorOrDefault(author, bws::preset::PresetSource::User);
    preset.description = description;
    preset.timestamp = juce::Time::getCurrentTime();
    preset.isFactory = false;
    juce::String previousLibraryKey;
    std::vector<bws::preset::PresetMetadata> exactOverwriteMetadata;
    const Preset* inheritSource = nullptr;
    for (const auto& existing : presets_)
    {
        if (!existing.isFactory && existing.name == preset.name && existing.category == preset.category)
        {
            previousLibraryKey = makePresetLibraryKey(existing);
            exactOverwriteMetadata.push_back(toCorePresetMetadata(pluginName_, existing));
            inheritSource = &existing; // overwrite → preserve target's metadata
            break;
        }
    }

    // Display metadata inheritance (contract §B2): preserve engine / useCase /
    // intensity / tags / isCleanStarter from the overwrite target (if any) or
    // fall back to the currently-loaded preset. Without this, user saves lose
    // the subtitle fields the browser renders (engine • intensity).
    if (inheritSource == nullptr && currentPreset_.isValid())
        inheritSource = &currentPreset_; // create-new → inherit loaded preset

    if (inheritSource != nullptr)
    {
        preset.engine = inheritSource->engine;
        preset.useCase = inheritSource->useCase;
        preset.intensity = inheritSource->intensity;
        preset.tags = inheritSource->tags;
        preset.isCleanStarter = inheritSource->isCleanStarter;
    }

    preset.presetId = juce::String(bws::preset::stableIdForUserPresetSave(
        bws::preset::PresetIdentity::legacy(pluginName_.toStdString(), preset.name.toStdString(),
                                            preset.category.toStdString()),
        exactOverwriteMetadata, bws::preset::generateUuidV4()));
    bws::preset::PresetStateBytes presetStateBytes;
    if (presetStateBridge_ != nullptr)
    {
        const auto metadataResult = presetStateBridge_->embedPresetMetadataResult(
            capturedStateBytes.view(),
            {std::string_view(preset.name.toRawUTF8(), static_cast<std::size_t>(preset.name.length())),
             std::string_view(preset.category.toRawUTF8(), static_cast<std::size_t>(preset.category.length())),
             std::string_view(preset.presetId.toRawUTF8(), static_cast<std::size_t>(preset.presetId.length())), false});
        lastStateCaptureStatus_ = metadataResult.status;
        if (metadataResult.verified())
            presetStateBytes = metadataResult.bytes;
        else if (metadataResult.status == bws::preset::StateCaptureStatus::unsupported)
            presetStateBytes =
                withEmbeddedPresetMetadata(capturedStateBytes, preset.name, preset.category, preset.presetId, false);
        else
            return false;
    }
    else
        presetStateBytes =
            withEmbeddedPresetMetadata(capturedStateBytes, preset.name, preset.category, preset.presetId, false);
    if (presetStateBytes.empty())
        return false;
    preset.format = Preset::Format::Json;
    preset.state =
        makePresetStateTree(pluginName_, preset.name, preset.category, preset.author, preset.description,
                            preset.timestamp, base64FromStateBytes(presetStateBytes), false, preset.sortOrder);

    if (presetRepository_ == nullptr)
        return false;

    auto saved =
        presetRepository_->savePreset(toCorePresetRecord(pluginName_, preset, presetStateBytes),
                                      previousLibraryKey.isNotEmpty() ? bws::preset::PresetSaveMode::OverwriteExisting
                                                                      : bws::preset::PresetSaveMode::CreateNew);
    if (!saved)
        return false;

    scanForPresets();

    for (const auto& candidate : presets_)
    {
        if (!candidate.isFactory &&
            bws::preset::matchesPresetLookupIdentity(toCorePresetMetadata(pluginName_, candidate).identity,
                                                     toCorePresetMetadata(pluginName_, preset).identity))
        {
            currentPreset_ = candidate;
            break;
        }
    }
    if (!currentPreset_.isValid() ||
        !bws::preset::matchesPresetLookupIdentity(toCorePresetMetadata(pluginName_, currentPreset_).identity,
                                                  toCorePresetMetadata(pluginName_, preset).identity))
    {
        currentPreset_ = preset;
    }
    currentPresetFile_ = currentPreset_.file;

    const auto currentLibraryKey = makePresetLibraryKey(currentPreset_);
    if (previousLibraryKey.isNotEmpty() && currentLibraryKey.isNotEmpty() && previousLibraryKey != currentLibraryKey)
    {
        auto nextState = bws::preset::migrateLibraryStateKey(libraryState_, previousLibraryKey.toStdString(),
                                                             currentLibraryKey.toStdString());
        if (saveLibraryState(nextState))
        {
            libraryState_ = nextState;
            refreshRecentPresetKeyCache();
            applyLibraryStateToPresets();
        }
    }

    updateRecentsForPreset(currentPreset_);

    hasUnsavedChanges_ = false;
    resetDirtyTrackingState(currentPreset_.name.isNotEmpty());
    notifyUnsavedChangesChanged(false);
    notifyPresetListChanged();

    return true;
}

//==============================================================================
// Core Operations - Load
//==============================================================================

bool WeatherPresetManager::loadPreset(const juce::String& name)
{
    const int index = findUniquePresetIndexByName(name);
    return index >= 0 && loadPresetByIndexInternal(index, LoadReason::DirectCommit);
}

bool WeatherPresetManager::loadPresetByIndex(int index)
{
    return loadPresetByIndexInternal(index, LoadReason::DirectCommit);
}

bool WeatherPresetManager::loadPresetByIndexInternal(int index, LoadReason reason)
{
    if (index < 0 || index >= static_cast<int>(presets_.size()))
        return false;

    const auto& preset = presets_[static_cast<size_t>(index)];
    const auto intent = reason == LoadReason::Preview ? bws::preset::ColdStateIntent::previewApply
                                                      : bws::preset::ColdStateIntent::presetFile;
    // applyProcessorState performs the one strict decode before entering any
    // bridge suppression or mutating processor/catalog metadata.
    if (!applyProcessorState(preset.state, intent, &preset))
        return false;

    currentPreset_ = preset;
    currentPresetFile_ = preset.file;
    hasUnsavedChanges_ = false;
    resetDirtyTrackingState(true);

    if (reason == LoadReason::DirectCommit)
        updateRecentsForPreset(currentPreset_);

    notifyPresetChanged();
    notifyUnsavedChangesChanged(false);

    return true;
}

//==============================================================================
// Core Operations - Delete
//==============================================================================

bool WeatherPresetManager::deletePreset(const juce::String& name)
{
    const int presetIndex = findUniqueUserPresetIndexByName(name);
    if (presetIndex < 0)
        return false;

    const auto preset = presets_[static_cast<size_t>(presetIndex)];
    if (preset.isFactory || presetRepository_ == nullptr)
        return false;

    const auto deleted = presetRepository_->deletePreset(toCorePresetMetadata(pluginName_, preset).identity);
    if (!deleted)
        return false;

    const auto key = makePresetLibraryKey(preset);
    auto nextState =
        key.isNotEmpty() ? bws::preset::removeLibraryStateKey(libraryState_, key.toStdString()) : libraryState_;

    const bool stateSaved = saveLibraryState(nextState);
    if (stateSaved)
    {
        libraryState_ = nextState;
        refreshRecentPresetKeyCache();
    }

    const auto currentKey = currentPreset_.isValid() ? makePresetLibraryKey(currentPreset_) : juce::String();
    const bool deletedCurrent = currentKey.isNotEmpty() && currentKey == key;
    scanForPresets();

    if (deletedCurrent)
    {
        currentPreset_ = Preset();
        currentPresetFile_ = juce::File();
    }

    // Rebuild row models before reloading so listeners reconcile against current rows.
    notifyPresetListChanged();

    if (deletedCurrent && !presets_.empty())
        loadPresetByIndex(0);

    return stateSaved;
}

//==============================================================================
// Core Operations - Rename
//==============================================================================

bool WeatherPresetManager::renamePreset(const juce::String& oldName, const juce::String& newName)
{
    if (newName.isEmpty() || oldName == newName)
        return false;

    const int existingIndex = findUniqueUserPresetIndexByName(oldName);
    if (existingIndex < 0)
        return false;

    const auto existing = presets_[static_cast<size_t>(existingIndex)];
    if (existing.isFactory || presetRepository_ == nullptr)
        return false;

    Preset renamed = existing;
    renamed.name = newName;
    renamed.state.setProperty("name", newName, nullptr);

    const auto currentKey = currentPreset_.isValid() ? makePresetLibraryKey(currentPreset_) : juce::String();
    const auto existingKey = makePresetLibraryKey(existing);
    const bool renamedPresetWasCurrent = currentKey.isNotEmpty() && currentKey == existingKey;

    auto result = presetRepository_->renamePreset(toCorePresetMetadata(pluginName_, existing).identity,
                                                  newName.toStdString(), existing.category.toStdString());
    if (!result)
        return false;
    if (auto converted = fromCorePresetRecord(pluginName_, result.value))
        renamed = *converted;

    auto nextState = libraryState_;
    const auto oldKey = existingKey;
    const auto newKey = makePresetLibraryKey(renamed);
    if (oldKey.isNotEmpty() && newKey.isNotEmpty() && oldKey != newKey)
        nextState =
            bws::preset::migrateLibraryStateKey(std::move(nextState), oldKey.toStdString(), newKey.toStdString());

    if (!saveLibraryState(nextState))
        return false;

    libraryState_ = nextState;
    refreshRecentPresetKeyCache();
    scanForPresets();

    if (renamedPresetWasCurrent)
    {
        bool resolvedFromScan = false;
        for (const auto& preset : presets_)
        {
            if (makePresetLibraryKey(preset) == newKey)
            {
                currentPreset_ = preset;
                currentPresetFile_ = preset.file;
                resolvedFromScan = true;
                break;
            }
        }

        if (!resolvedFromScan)
        {
            currentPreset_ = renamed;
            currentPresetFile_ = renamed.file;
        }
    }

    notifyPresetListChanged();
    return true;
}

//==============================================================================
// Navigation
//==============================================================================

void WeatherPresetManager::loadNextPreset()
{
    static_cast<void>(loadPresetByDirection(bws::preset::PresetNavigationDirection::Next));
}

void WeatherPresetManager::loadPreviousPreset()
{
    static_cast<void>(loadPresetByDirection(bws::preset::PresetNavigationDirection::Previous));
}

bool WeatherPresetManager::loadPresetByDirection(bws::preset::PresetNavigationDirection direction)
{
    if (presets_.empty())
        return false;

    const auto currentKey =
        currentPreset_.isValid() ? makePresetLibraryKey(currentPreset_).toStdString() : std::string {};
    const auto attempts = bws::preset::presetCatalogNavigationAttemptIndices(
        toCoreCatalogEntries(pluginName_, presets_), toCoreCatalogQuery(FilterCriteria {}), currentKey, direction,
        [](const auto& entry) { return bws::preset::makePresetLibraryKey(entry.metadata); });

    for (const auto presetIndex : attempts)
    {
        if (loadPresetByIndex(static_cast<int>(presetIndex)))
            return true;
    }

    return false;
}

int WeatherPresetManager::getCurrentPresetIndex() const
{
    const auto currentKey = currentPreset_.isValid() ? makePresetLibraryKey(currentPreset_) : juce::String();
    if (currentKey.isEmpty())
        return -1;

    for (size_t i = 0; i < presets_.size(); ++i)
    {
        if (makePresetLibraryKey(presets_[i]) == currentKey)
            return static_cast<int>(i);
    }

    return -1;
}

//==============================================================================
// Query
//==============================================================================

std::vector<juce::String> WeatherPresetManager::getCategories() const
{
    std::set<juce::String> categorySet;
    for (const auto& preset : presets_)
        categorySet.insert(preset.category);
    return std::vector<juce::String>(categorySet.begin(), categorySet.end());
}

std::vector<WeatherPresetManager::Preset> WeatherPresetManager::getPresetsInCategory(const juce::String& category) const
{
    std::vector<Preset> result;
    for (const auto& preset : presets_)
    {
        if (preset.category == category)
            result.push_back(preset);
    }
    return result;
}

std::vector<int> WeatherPresetManager::queryPresetIndices(const FilterCriteria& criteria) const
{
    const auto queriedIndices = bws::preset::queryPresetCatalogEntries(toCoreCatalogEntries(pluginName_, presets_),
                                                                       toCoreCatalogQuery(criteria));

    std::vector<int> indices;
    indices.reserve(queriedIndices.size());
    for (const auto index : queriedIndices)
        indices.push_back(static_cast<int>(index));
    return indices;
}

WeatherPresetManager::PresetIdentity WeatherPresetManager::getPresetIdentity(const Preset& preset) const
{
    return bws::preset::makePresetLibraryIdentity(toCorePresetMetadata(pluginName_, preset));
}

juce::String WeatherPresetManager::getPresetIdentityKey(const Preset& preset) const
{
    return makePresetLibraryKey(preset);
}

juce::StringArray WeatherPresetManager::getAvailableEngines() const
{
    juce::StringArray values;
    for (const auto& preset : presets_)
    {
        if (preset.engine.isNotEmpty())
            values.addIfNotAlreadyThere(preset.engine);
    }
    values.sort(true);
    return values;
}

juce::StringArray WeatherPresetManager::getAvailableUseCases() const
{
    juce::StringArray values;
    for (const auto& preset : presets_)
    {
        if (preset.useCase.isNotEmpty())
            values.addIfNotAlreadyThere(preset.useCase);
    }
    values.sort(true);
    return values;
}

juce::StringArray WeatherPresetManager::getAvailableIntensityLabels() const
{
    juce::StringArray values;
    for (const auto& preset : presets_)
    {
        const auto label = intensityToString(preset.intensity);
        if (label.isNotEmpty())
            values.addIfNotAlreadyThere(label);
    }
    return values;
}

bool WeatherPresetManager::isPresetFavorite(int presetIndex) const
{
    return juce::isPositiveAndBelow(presetIndex, static_cast<int>(presets_.size())) &&
           presets_[static_cast<size_t>(presetIndex)].isFavorite;
}

bool WeatherPresetManager::setPresetFavorite(int presetIndex, bool shouldBeFavorite)
{
    if (!juce::isPositiveAndBelow(presetIndex, static_cast<int>(presets_.size())))
        return false;

    const auto& preset = presets_[static_cast<size_t>(presetIndex)];
    const auto key = makePresetLibraryKey(preset);
    if (key.isEmpty())
        return false;

    auto nextState = libraryState_;
    const auto keyString = key.toStdString();
    if (shouldBeFavorite)
        nextState.favoriteKeys.insert(keyString);
    else
        nextState.favoriteKeys.erase(keyString);

    const bool ok = saveLibraryState(nextState);
    if (!ok)
        return false;

    libraryState_ = nextState;
    applyLibraryStateToPresets();
    if (ok)
        notifyPresetListChanged();
    return ok;
}

bool WeatherPresetManager::togglePresetFavorite(int presetIndex)
{
    if (!juce::isPositiveAndBelow(presetIndex, static_cast<int>(presets_.size())))
        return false;
    return setPresetFavorite(presetIndex, !presets_[static_cast<size_t>(presetIndex)].isFavorite);
}

bool WeatherPresetManager::hasStartupDefaultPreset() const
{
    return !libraryState_.startupPresetKey.empty();
}

bool WeatherPresetManager::isCurrentPresetStartupDefault() const
{
    return currentPreset_.isValid() &&
           makePresetLibraryKey(currentPreset_).toStdString() == libraryState_.startupPresetKey;
}

bool WeatherPresetManager::setStartupDefaultPreset(int presetIndex)
{
    if (!juce::isPositiveAndBelow(presetIndex, static_cast<int>(presets_.size())))
        return false;

    const auto key = makePresetLibraryKey(presets_[static_cast<size_t>(presetIndex)]);
    if (key.isEmpty())
        return false;

    auto nextState = libraryState_;
    nextState.startupPresetKey = key.toStdString();
    const bool ok = saveLibraryState(nextState);
    if (!ok)
        return false;

    libraryState_ = nextState;
    applyLibraryStateToPresets();
    if (ok)
        notifyPresetListChanged();
    return ok;
}

bool WeatherPresetManager::clearStartupDefaultPreset()
{
    if (libraryState_.startupPresetKey.empty())
        return true;

    auto nextState = libraryState_;
    nextState.startupPresetKey.clear();
    const bool ok = saveLibraryState(nextState);
    if (!ok)
        return false;

    libraryState_ = nextState;
    applyLibraryStateToPresets();
    if (ok)
        notifyPresetListChanged();
    return ok;
}

//==============================================================================
// State Management
//==============================================================================

void WeatherPresetManager::markAsModified()
{
    if (!hasUnsavedChanges_)
    {
        hasUnsavedChanges_ = true;
        notifyUnsavedChangesChanged(true);
    }
}

void WeatherPresetManager::setPresetStateBridge(std::unique_ptr<bws::preset::IPresetStateBridge> bridge)
{
    clearPresetStateBridge();
    presetStateBridge_ = std::move(bridge);
    if (presetStateBridge_ == nullptr)
        return;

    presetStateSubscription_ = presetStateBridge_->subscribeToDirtyChanges([this] { markAsModified(); });
    if (isInitialized_ && !presetStateBridge_->activate())
        return;
    resetDirtyTrackingState(currentPreset_.name.isNotEmpty());
}

void WeatherPresetManager::clearPresetStateBridge()
{
    activeRestoreSuppressions_.clear();
    presetStateSubscription_.reset();
    if (presetStateBridge_ != nullptr)
        presetStateBridge_->deactivate();
    presetStateBridge_.reset();
}

bool WeatherPresetManager::activatePresetStateBridge()
{
    if (presetStateBridge_ == nullptr)
        return true;
    if (!presetStateBridge_->activate())
        return false;
    resetDirtyTrackingState(currentPreset_.name.isNotEmpty());
    return true;
}

void WeatherPresetManager::setPresetRepository(std::unique_ptr<bws::preset::IPresetRepository> repository)
{
    presetRepository_ = std::move(repository);
}

void WeatherPresetManager::flushPendingDirtyTrackingForTesting()
{
    if (presetStateBridge_ != nullptr)
        presetStateBridge_->flushPendingDirtyChangeForTesting();
}

bool WeatherPresetManager::hasPendingDirtyTrackingChange() const
{
    return presetStateBridge_ != nullptr && presetStateBridge_->hasPendingDirtyChange();
}

void WeatherPresetManager::resetDirtyTrackingState(bool eligible)
{
    if (presetStateBridge_ != nullptr)
        presetStateBridge_->resetDirtyTracking(eligible);
}

void WeatherPresetManager::refreshPresetList()
{
    scanForPresets();
    notifyPresetListChanged();
}

bool WeatherPresetManager::selectPresetForRestore(const juce::String& name, const juce::String& category,
                                                  bool wasModified, const juce::String& presetId)
{
    std::vector<bws::preset::PresetMetadata> presetMetadata;
    presetMetadata.reserve(presets_.size());
    for (const auto& preset : presets_)
        presetMetadata.push_back(toCorePresetMetadata(pluginName_, preset));

    const auto restoreIndex = bws::preset::findPresetRestoreIndex(
        presetMetadata, pluginName_.toStdString(), name.toStdString(), category.toStdString(), presetId.toStdString());

    if (restoreIndex.has_value())
    {
        const auto& preset = presets_[*restoreIndex];
        currentPreset_ = preset;
        currentPresetFile_ = preset.file;
        hasUnsavedChanges_ = wasModified;
        resetDirtyTrackingState(true);

        notifyPresetChanged();
        notifyUnsavedChangesChanged(wasModified);
        return true;
    }

    currentPreset_ = Preset();
    currentPreset_.name = name;
    currentPreset_.category = category;
    currentPreset_.presetId = presetId;
    hasUnsavedChanges_ = name.isNotEmpty() || wasModified;
    resetDirtyTrackingState(currentPreset_.name.isNotEmpty());

    notifyPresetChanged();
    notifyUnsavedChangesChanged(hasUnsavedChanges_);

    return false;
}

//==============================================================================
// Centralized State Management Helpers
//==============================================================================

bws::domain::BwResult<bws::domain::PresetMetadata> WeatherPresetManager::getPresetMetadata() const
{
    const auto& currentPreset = getCurrentPreset();
    cachedPresetName_ = currentPreset.name;
    cachedPresetCategory_ = currentPreset.category;
    cachedPresetId_ = currentPreset.presetId;
    return bws::domain::BwSuccess(bws::domain::PresetMetadata {
        std::string_view(cachedPresetName_.toRawUTF8(), static_cast<size_t>(cachedPresetName_.length())),
        std::string_view(cachedPresetCategory_.toRawUTF8(), static_cast<size_t>(cachedPresetCategory_.length())),
        hasUnsavedChanges(),
        std::string_view(cachedPresetId_.toRawUTF8(), static_cast<size_t>(cachedPresetId_.length()))});
}

bws::domain::BwResult<void> WeatherPresetManager::restorePresetState(bws::domain::BwStateBlob blob)
{
    auto xml = juce::AudioProcessor::getXmlFromBinary(blob.data.data(), static_cast<int>(blob.data.size_bytes()));
    if (!xml)
        return bws::domain::BwFailureVoid("restorePresetState: invalid blob");

    ScopedDirtyTrackingSuppression dirtyGuard(*this);

    if (xml->hasAttribute("currentPresetName"))
    {
        {
            ScopedCallbackSuppression callbackGuard(*this);
            selectPresetForRestore(
                xml->getStringAttribute("currentPresetName"), xml->getStringAttribute("currentPresetCategory"),
                xml->getBoolAttribute("presetWasModified", false), xml->getStringAttribute("currentPresetId"));
        }

        notifyPresetChanged();
        notifyUnsavedChangesChanged(hasUnsavedChanges());
    }
    else
    {
        currentPreset_ = Preset();
        currentPresetFile_ = juce::File();
        hasUnsavedChanges_ = false;
        resetDirtyTrackingState(false);
        notifyPresetChanged();
        notifyUnsavedChangesChanged(false);
    }

    return bws::domain::BwSuccessVoid();
}

void WeatherPresetManager::beginPresetStateRestore()
{
    if (presetStateBridge_ != nullptr)
        activeRestoreSuppressions_.push_back(presetStateBridge_->suppressDirtyNotifications());
}

void WeatherPresetManager::endPresetStateRestore()
{
    if (!activeRestoreSuppressions_.empty())
        activeRestoreSuppressions_.pop_back();

    if (activeRestoreSuppressions_.empty())
        resetDirtyTrackingState(currentPreset_.name.isNotEmpty());
}

//==============================================================================
// Scanning & Sorting
//==============================================================================

void WeatherPresetManager::scanForPresets()
{
    presets_.clear();

    for (const auto& factoryPreset : registeredFactoryPresets_)
        presets_.push_back(factoryPreset);

    if (presetRepository_ == nullptr)
    {
        sortPresets();
        clearInvalidLibraryStateReferences();
        applyLibraryStateToPresets();
        return;
    }

    std::set<std::string> seenKeys;
    for (const auto& preset : presets_)
    {
        seenKeys.insert(bws::preset::makePresetDiscoveryKey(toCorePresetMetadata(pluginName_, preset)));
    }

    const auto listed = presetRepository_->listPresets({pluginName_.toStdString(), true, true});
    if (listed)
    {
        for (const auto& metadata : listed.value)
        {
            auto loaded = presetRepository_->loadPreset(metadata.identity);
            if (!loaded)
                continue;

            auto converted = fromCorePresetRecord(pluginName_, loaded.value);
            if (!converted.has_value())
                continue;

            const auto key = bws::preset::makePresetDiscoveryKey(toCorePresetMetadata(pluginName_, *converted));
            if (!seenKeys.insert(key).second)
                continue;

            presets_.push_back(std::move(*converted));
        }
    }

    sortPresets();
    clearInvalidLibraryStateReferences();
    applyLibraryStateToPresets();

    if (currentPreset_.isValid())
    {
        const auto currentKey = makePresetLibraryKey(currentPreset_);
        for (const auto& preset : presets_)
        {
            if (makePresetLibraryKey(preset) == currentKey)
            {
                currentPreset_ = preset;
                currentPresetFile_ = preset.file;
                break;
            }
        }
    }
}

void WeatherPresetManager::sortPresets()
{
    std::sort(presets_.begin(), presets_.end(), [this](const Preset& a, const Preset& b) {
        return bws::preset::presetCatalogEntryCanonicalLess(
            {0, toCorePresetMetadata(pluginName_, a), a.isFavorite, a.recentRank},
            {0, toCorePresetMetadata(pluginName_, b), b.isFavorite, b.recentRank});
    });
}

//==============================================================================
// Factory Preset Helper
//==============================================================================

void WeatherPresetManager::addFactoryPreset(const juce::String& name, const juce::String& category,
                                            const juce::ValueTree& state, const juce::String& description)
{
    Preset preset;
    preset.name = name;
    preset.category = category;
    preset.author = juceStringFromStdString(std::string(bws::preset::kDefaultFactoryPresetAuthor));
    preset.description = description;
    preset.timestamp = juce::Time::getCurrentTime();
    preset.isFactory = true;
    preset.format = Preset::Format::InMemoryFactory;
    preset.sortOrder = bws::preset::implicitFactorySortOrderForDeclarationIndex(registeredFactoryPresets_.size());

    auto existingBase64 = state.getProperty("data").toString();
    if (existingBase64.isEmpty())
    {
        juce::MemoryBlock stateData;
        juce::MemoryOutputStream stream(stateData, false);
        state.writeToStream(stream);
        existingBase64 = juce::Base64::toBase64(stateData.getData(), stateData.getSize());
    }

    preset.state = makePresetStateTree(pluginName_, preset.name, preset.category, preset.author, preset.description,
                                       preset.timestamp, existingBase64, true, preset.sortOrder);
    registeredFactoryPresets_.push_back(preset);
}

//==============================================================================
// Backward Compatibility (Legacy PresetManager API)
//==============================================================================

juce::StringArray WeatherPresetManager::getFactoryPresetNames() const
{
    juce::StringArray names;
    for (const auto& preset : presets_)
    {
        if (preset.isFactory)
            names.add(preset.name);
    }
    return names;
}

juce::StringArray WeatherPresetManager::getUserPresetNames() const
{
    juce::StringArray names;
    for (const auto& preset : presets_)
    {
        if (!preset.isFactory)
            names.add(preset.name);
    }
    return names;
}

int WeatherPresetManager::getFactoryPresetCount() const
{
    int count = 0;
    for (const auto& preset : presets_)
    {
        if (preset.isFactory)
            ++count;
    }
    return count;
}

int WeatherPresetManager::getUserPresetCount() const
{
    int count = 0;
    for (const auto& preset : presets_)
    {
        if (!preset.isFactory)
            ++count;
    }
    return count;
}

bool WeatherPresetManager::loadFactoryPreset(int index)
{
    int factoryIndex = 0;
    for (size_t i = 0; i < presets_.size(); ++i)
    {
        if (!presets_[i].isFactory)
            continue;
        if (factoryIndex == index)
            return loadPresetByIndex(static_cast<int>(i));
        ++factoryIndex;
    }
    return false;
}

bool WeatherPresetManager::loadUserPreset(int index)
{
    int userIndex = 0;
    for (size_t i = 0; i < presets_.size(); ++i)
    {
        if (presets_[i].isFactory)
            continue;
        if (userIndex == index)
            return loadPresetByIndex(static_cast<int>(i));
        ++userIndex;
    }
    return false;
}

bool WeatherPresetManager::saveUserPreset(const juce::String& name)
{
    return savePreset(name, "User", "", "");
}

bool WeatherPresetManager::beginPreviewSession()
{
    if (previewSession_.active)
        return true;

    previewSession_.snapshot = captureProcessorState();
    previewSession_.committedPreset = currentPreset_;
    previewSession_.committedPresetFile = currentPresetFile_;
    previewSession_.committedHadUnsavedChanges = hasUnsavedChanges();
    previewSession_.active = previewSession_.snapshot.isValid();
    return previewSession_.active;
}

bool WeatherPresetManager::previewPresetByIndex(int index)
{
    if (!previewSession_.active)
        return false;

    return loadPresetByIndexInternal(index, LoadReason::Preview);
}

bool WeatherPresetManager::commitPreviewSession()
{
    if (!previewSession_.active)
        return false;

    if (currentPreset_.isValid())
        updateRecentsForPreset(currentPreset_);

    previewSession_ = {};
    return true;
}

bool WeatherPresetManager::cancelPreviewSession()
{
    if (!previewSession_.active || !previewSession_.snapshot.isValid())
        return false;

    const auto snapshot = previewSession_.snapshot.createCopy();
    const auto committedPreset = previewSession_.committedPreset;
    const auto committedPresetFile = previewSession_.committedPresetFile;
    const bool committedHadUnsavedChanges = previewSession_.committedHadUnsavedChanges;
    {
        ScopedDirtyTrackingSuppression dirtyGuard(*this);
        if (!applyProcessorState(snapshot, bws::preset::ColdStateIntent::previewCancel))
            return false;
    }
    previewSession_ = {};
    currentPreset_ = committedPreset;
    currentPresetFile_ = committedPresetFile;
    hasUnsavedChanges_ = committedHadUnsavedChanges;
    resetDirtyTrackingState(currentPreset_.name.isNotEmpty());
    notifyPresetChanged();
    notifyUnsavedChangesChanged(hasUnsavedChanges_);
    return true;
}

juce::File WeatherPresetManager::getLibraryStateFile() const
{
    auto presetRoot = getUserPresetDirectory().getParentDirectory();
    if (presetRoot == juce::File())
        presetRoot = getDefaultUserPresetDirectory().getParentDirectory();
    return presetRoot.getChildFile(juceStringFromStdString(std::string(bws::preset::kPresetLibraryStateFileName)));
}

bool WeatherPresetManager::readLibraryStateDocument(juce::var& outDocument) const
{
    const auto file = getLibraryStateFile();
    if (!file.existsAsFile())
        return false;

    outDocument = juce::JSON::parse(file);
    return true;
}

bool WeatherPresetManager::writeLibraryStateDocument(const juce::var& document) const
{
    const auto file = getLibraryStateFile();
    file.getParentDirectory().createDirectory();
    return file.replaceWithText(juce::JSON::toString(document, true));
}

void WeatherPresetManager::loadLibraryState()
{
    libraryState_ = {};
    recentPresetKeys_.clear();

    juce::var parsed;
    if (!readLibraryStateDocument(parsed))
        return;
    auto parsedState = parsePresetLibraryStateDocument(parsed);
    if (!parsedState.has_value())
        return;

    libraryState_ = std::move(*parsedState);
    refreshRecentPresetKeyCache();
}

bool WeatherPresetManager::saveLibraryState() const
{
    return saveLibraryState(libraryState_);
}

bool WeatherPresetManager::saveLibraryState(const bws::preset::PresetLibraryState& state) const
{
    return writeLibraryStateDocument(makePresetLibraryStateDocument(state));
}

void WeatherPresetManager::refreshRecentPresetKeyCache()
{
    recentPresetKeys_.clear();
    recentPresetKeys_.reserve(libraryState_.recentPresetKeys.size());
    for (const auto& recentKey : libraryState_.recentPresetKeys)
        recentPresetKeys_.push_back(juce::String(recentKey));
}

void WeatherPresetManager::applyLibraryStateToPresets()
{
    refreshRecentPresetKeyCache();

    const auto currentKey = currentPreset_.isValid() ? makePresetLibraryKey(currentPreset_) : juce::String();

    for (auto& preset : presets_)
    {
        const auto key = makePresetLibraryKey(preset);
        const auto keyString = key.toStdString();
        preset.isFavorite = libraryState_.favoriteKeys.contains(keyString);
        preset.isStartupDefault = (keyString == libraryState_.startupPresetKey);
        preset.recentRank = bws::preset::recentPresetRank(libraryState_, keyString);

        if (currentKey.isNotEmpty() && key == currentKey)
            currentPreset_ = preset;
    }
}

juce::String WeatherPresetManager::makePresetLibraryKey(const Preset& preset) const
{
    return juce::String(bws::preset::makePresetLibraryKey(toCorePresetMetadata(pluginName_, preset)));
}

void WeatherPresetManager::updateRecentsForPreset(const Preset& preset)
{
    const auto key = makePresetLibraryKey(preset);
    if (key.isEmpty())
        return;

    auto nextState = bws::preset::promoteRecentPreset(libraryState_, key.toStdString());

    if (!saveLibraryState(nextState))
        return;

    libraryState_ = nextState;
    applyLibraryStateToPresets();
}

void WeatherPresetManager::clearInvalidLibraryStateReferences()
{
    std::set<std::string> validKeys;
    for (const auto& preset : presets_)
    {
        const auto key = makePresetLibraryKey(preset);
        if (key.isNotEmpty())
            validKeys.insert(key.toStdString());
    }

    libraryState_ = bws::preset::clearInvalidLibraryStateReferences(libraryState_, validKeys);
    refreshRecentPresetKeyCache();
}

juce::PropertiesFile* WeatherPresetManager::getPropertiesFile() const
{
    juce::PropertiesFile::Options options;
    options.applicationName = pluginName_;
    options.filenameSuffix = ".settings";
    options.folderName = "BellweatherStudios";
    options.osxLibrarySubFolder = "Application Support";

    if (!propertiesFile_)
        propertiesFile_ = std::make_unique<juce::PropertiesFile>(options);

    return propertiesFile_.get();
}

bool WeatherPresetManager::decodeBase64State(const juce::String& base64, juce::MemoryBlock& decoded) const
{
    decoded.reset();
    if (base64.isEmpty())
        return false;

    const auto encodedSize = static_cast<std::size_t>(base64.length());
    if ((encodedSize % 4u) != 0u)
        return false;

    const auto* raw = base64.toRawUTF8();
    if (std::strlen(raw) != encodedSize)
        return false;

    const auto cap = maximumDecodedPresetStateBytes();
    if (cap != std::numeric_limits<std::size_t>::max())
    {
        const auto maxEncoded = 4u * ((cap + 2u) / 3u);
        if (encodedSize > maxEncoded)
            return false;
    }

    const auto sextet = [](juce::juce_wchar c) -> int {
        if (c >= 'A' && c <= 'Z')
            return static_cast<int>(c - 'A');
        if (c >= 'a' && c <= 'z')
            return static_cast<int>(c - 'a' + 26);
        if (c >= '0' && c <= '9')
            return static_cast<int>(c - '0' + 52);
        if (c == '+')
            return 62;
        if (c == '/')
            return 63;
        return -1;
    };

    std::size_t padding = 0;
    if (raw[encodedSize - 1u] == '=')
        ++padding;
    if (encodedSize >= 2u && raw[encodedSize - 2u] == '=')
        ++padding;
    for (std::size_t i = 0; i < encodedSize; ++i)
    {
        const auto c = static_cast<juce::juce_wchar>(raw[i]);
        const bool paddingPosition = i >= encodedSize - padding;
        if ((c == '=') != paddingPosition || (!paddingPosition && sextet(c) < 0))
            return false;
    }
    if (padding == 2u && (sextet(raw[encodedSize - 3u]) & 0x0f) != 0)
        return false;
    if (padding == 1u && (sextet(raw[encodedSize - 2u]) & 0x03) != 0)
        return false;

    const auto decodedSize = (encodedSize / 4u) * 3u - padding;
    if (decodedSize == 0u || decodedSize > cap)
        return false;
    decoded.setSize(decodedSize, true);
    juce::MemoryOutputStream stream(decoded.getData(), decoded.getSize());
    if (!juce::Base64::convertFromBase64(stream, base64))
    {
        decoded.reset();
        return false;
    }
    if (stream.getDataSize() != decodedSize)
    {
        decoded.reset();
        return false;
    }
    return true;
}

int WeatherPresetManager::findUniquePresetIndexByName(const juce::String& name) const
{
    int match = -1;
    for (int i = 0; i < static_cast<int>(presets_.size()); ++i)
    {
        if (presets_[static_cast<size_t>(i)].name != name)
            continue;
        if (match >= 0)
            return -1;
        match = i;
    }
    return match;
}

int WeatherPresetManager::findUniqueUserPresetIndexByName(const juce::String& name) const
{
    int match = -1;
    for (int i = 0; i < static_cast<int>(presets_.size()); ++i)
    {
        const auto& preset = presets_[static_cast<size_t>(i)];
        if (preset.isFactory || preset.name != name)
            continue;
        if (match >= 0)
            return -1;
        match = i;
    }
    return match;
}

//==============================================================================
// Processor State Helpers (Virtual - Override for custom serialization)
//==============================================================================

bws::preset::PresetStateBytes WeatherPresetManager::capturePresetStateBytes() const
{
    if (presetStateBridge_ != nullptr)
    {
        auto result = presetStateBridge_->captureStateResult();
        lastStateCaptureStatus_ = result.status;
        if (result.verified() || result.status == bws::preset::StateCaptureStatus::capturedUnverified)
            return std::move(result.bytes);
        return {};
    }

    auto bytes = stateBytesFromProcessorState(processor_);
    lastStateCaptureStatus_ =
        bytes.empty() ? bws::preset::StateCaptureStatus::failed : bws::preset::StateCaptureStatus::capturedUnverified;
    return bytes;
}

juce::ValueTree WeatherPresetManager::captureProcessorState() const
{
    const auto bytes = capturePresetStateBytes();
    if (bytes.empty())
        return {};
    juce::ValueTree state("ProcessorState");
    state.setProperty("data", base64FromStateBytes(bytes), nullptr);
    return state;
}

bool WeatherPresetManager::applyProcessorState(const juce::ValueTree& state, bws::preset::ColdStateIntent intent,
                                               const Preset* trustedPreset)
{
    auto base64 = state.getProperty("data").toString();
    if (base64.isEmpty())
        base64 = state.getProperty("state").toString();

    juce::MemoryBlock decoded;
    if (!decodeBase64State(base64, decoded))
        return false;

    if (presetStateBridge_ != nullptr)
    {
        bws::preset::ColdStateIdentityContext identity;
        std::string stateSha256;
        if (trustedPreset != nullptr)
        {
            stateSha256 = juce::SHA256(decoded.getData(), decoded.getSize()).toHexString().toStdString();
            identity = {std::string_view(pluginName_.toRawUTF8(), static_cast<std::size_t>(pluginName_.length())),
                        std::string_view(trustedPreset->presetId.toRawUTF8(),
                                         static_cast<std::size_t>(trustedPreset->presetId.length())),
                        stateSha256, trustedPreset->isFactory};
        }
        const auto result = presetStateBridge_->applyStateResultWithContext(
            bws::domain::BwStateBlob {
                std::span<const std::byte>(static_cast<const std::byte*>(decoded.getData()), decoded.getSize())},
            intent, identity);
        lastStateApplyStatus_ = result.status;
        return result.verifiedCommit() || result.status == bws::preset::StateApplyStatus::appliedUnverified;
    }
    else
    {
        ScopedDirtyTrackingSuppression dirtyGuard(*this);
        ScopedCallbackSuppression callbackGuard(*this);
        processor_.setStateInformation(decoded.getData(), static_cast<int>(decoded.getSize()));
        lastStateApplyStatus_ = bws::preset::StateApplyStatus::appliedUnverified;
        return true; // Legacy void JUCE path is applied but unverified.
    }
}

} // namespace bws::weather

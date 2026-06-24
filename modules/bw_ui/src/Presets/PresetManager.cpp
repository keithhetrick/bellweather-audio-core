// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/Presets/PresetManager.h"

namespace bws
{

// =============================================================================
// Construction
// =============================================================================

PresetManager::PresetManager(const juce::String& pluginName)
    : pluginName_(pluginName)
{
    loadCustomPresetDirectorySetting();
}

// =============================================================================
// Location Management
// =============================================================================

juce::File PresetManager::getDefaultPresetDirectory() const
{
    auto appData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);

#if JUCE_MAC
    return appData.getChildFile("Application Support").getChildFile("BellweatherStudios").getChildFile(pluginName_);
#elif JUCE_WINDOWS
    return appData.getChildFile("BellweatherStudios").getChildFile(pluginName_);
#else
    return appData.getChildFile(".config").getChildFile("BellweatherStudios").getChildFile(pluginName_);
#endif
}

juce::File PresetManager::getPresetDirectory() const
{
    if (hasCustomPresetDirectory())
        return customPresetDirectory_;

    return getDefaultPresetDirectory();
}

juce::File PresetManager::getSettingsFile() const
{
    return getDefaultPresetDirectory().getChildFile("Settings.json");
}

bool PresetManager::hasCustomPresetDirectory() const
{
    return customPresetDirectory_.exists() && customPresetDirectory_.isDirectory();
}

juce::File PresetManager::getCustomPresetDirectory() const
{
    return customPresetDirectory_;
}

void PresetManager::setCustomPresetDirectory(const juce::File& directory)
{
    if (!directory.exists())
        directory.createDirectory();

    // Copy existing presets to new location if different
    const auto oldDir = getPresetDirectory();
    if (oldDir.exists() && oldDir != directory)
    {
        auto oldPresetFile = oldDir.getChildFile("UserPresets.json");
        if (oldPresetFile.existsAsFile())
        {
            oldPresetFile.copyFileTo(directory.getChildFile("UserPresets.json"));
            DBG("[PresetManager] Copied presets from " << oldDir.getFullPathName() << " to "
                                                       << directory.getFullPathName());
        }
    }

    customPresetDirectory_ = directory;
    saveCustomPresetDirectorySetting();

    // Reload presets from new location
    loadUserPresets();
}

void PresetManager::clearCustomPresetDirectory()
{
    customPresetDirectory_ = juce::File();
    saveCustomPresetDirectorySetting();
    loadUserPresets();
}

void PresetManager::revealPresetDirectory() const
{
    auto presetDir = getPresetDirectory();

    if (!presetDir.exists())
        presetDir.createDirectory();

    presetDir.revealToUser();
}

void PresetManager::loadCustomPresetDirectorySetting()
{
    auto settingsFile = getSettingsFile();
    if (!settingsFile.existsAsFile())
        return;

    auto jsonString = settingsFile.loadFileAsString();
    auto json = juce::JSON::parse(jsonString);

    if (json.isObject())
    {
        auto* obj = json.getDynamicObject();
        if (obj != nullptr)
        {
            auto customPath = obj->getProperty("customPresetDirectory").toString();
            if (customPath.isNotEmpty())
            {
                juce::File customDir(customPath);
                if (customDir.exists() && customDir.isDirectory())
                {
                    customPresetDirectory_ = customDir;
                    DBG("[PresetManager] Loaded custom preset directory: " << customPath);
                }
            }
        }
    }
}

void PresetManager::saveCustomPresetDirectorySetting()
{
    auto settingsDir = getDefaultPresetDirectory();
    if (!settingsDir.exists())
        settingsDir.createDirectory();

    auto settingsFile = getSettingsFile();

    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("customPresetDirectory",
                     customPresetDirectory_.exists() ? customPresetDirectory_.getFullPathName() : "");

    auto jsonString = juce::JSON::toString(juce::var(obj.get()));
    settingsFile.replaceWithText(jsonString);

    DBG("[PresetManager] Saved settings to: " << settingsFile.getFullPathName());
}

// =============================================================================
// CRUD Operations
// =============================================================================

void PresetManager::loadUserPresets()
{
    userPresets_.clear();

    auto presetFile = getPresetDirectory().getChildFile("UserPresets.json");
    if (!presetFile.existsAsFile())
        return;

    auto jsonString = presetFile.loadFileAsString();
    auto json = juce::JSON::parse(jsonString);

    if (!json.isObject())
        return;

    auto* obj = json.getDynamicObject();
    if (obj == nullptr)
        return;

    auto presetsArray = obj->getProperty("presets");
    if (!presetsArray.isArray())
        return;

    for (int i = 0; i < presetsArray.getArray()->size(); ++i)
    {
        auto presetVar = presetsArray.getArray()->getReference(i);
        if (!presetVar.isObject())
            continue;

        auto* presetObj = presetVar.getDynamicObject();
        if (presetObj == nullptr)
            continue;

        UserPreset preset;
        preset.name = presetObj->getProperty("name").toString();
        preset.timestamp =
            juce::Time(static_cast<juce::int64>(presetObj->getProperty("timestamp").toString().getLargeIntValue()));
        preset.state = presetObj->getProperty("parameters");

        if (preset.isValid())
            userPresets_.push_back(preset);
    }

    DBG("[PresetManager] Loaded " << userPresets_.size() << " user presets for " << pluginName_);
}

void PresetManager::saveUserPresetsToFile()
{
    auto presetDir = getPresetDirectory();
    presetDir.createDirectory();

    auto json = std::make_unique<juce::DynamicObject>();
    json->setProperty("version", "1.0");
    json->setProperty("plugin", pluginName_);

    juce::Array<juce::var> presetsArray;
    for (const auto& preset : userPresets_)
    {
        auto presetObj = std::make_unique<juce::DynamicObject>();
        presetObj->setProperty("name", preset.name);
        presetObj->setProperty("timestamp", juce::String(preset.timestamp.toMilliseconds()));
        presetObj->setProperty("parameters", preset.state);

        presetsArray.add(juce::var(presetObj.release()));
    }

    json->setProperty("presets", presetsArray);

    auto jsonString = juce::JSON::toString(juce::var(json.release()), true);
    auto presetFile = presetDir.getChildFile("UserPresets.json");
    presetFile.replaceWithText(jsonString);

    DBG("[PresetManager] Saved " << userPresets_.size() << " user presets for " << pluginName_);
}

bool PresetManager::saveUserPreset(const juce::String& name)
{
    if (name.isEmpty())
        return false;

    // Check for duplicate name - update existing preset
    for (size_t i = 0; i < userPresets_.size(); ++i)
    {
        if (userPresets_[i].name.compareIgnoreCase(name) == 0)
        {
            userPresets_[i].state = getCurrentState();
            userPresets_[i].timestamp = juce::Time::getCurrentTime();
            saveUserPresetsToFile();
            return true;
        }
    }

    // Add new preset
    UserPreset preset;
    preset.name = name;
    preset.state = getCurrentState();
    preset.timestamp = juce::Time::getCurrentTime();
    userPresets_.push_back(preset);

    saveUserPresetsToFile();
    return true;
}

bool PresetManager::loadUserPreset(int index)
{
    if (index < 0 || index >= static_cast<int>(userPresets_.size()))
        return false;

    applyState(userPresets_[static_cast<size_t>(index)].state);
    return true;
}

bool PresetManager::updateUserPreset(int index)
{
    if (index < 0 || index >= static_cast<int>(userPresets_.size()))
        return false;

    userPresets_[static_cast<size_t>(index)].state = getCurrentState();
    userPresets_[static_cast<size_t>(index)].timestamp = juce::Time::getCurrentTime();

    saveUserPresetsToFile();

    DBG("[PresetManager] Updated user preset " << index << ": " << userPresets_[static_cast<size_t>(index)].name);
    return true;
}

bool PresetManager::deleteUserPreset(int index)
{
    if (index < 0 || index >= static_cast<int>(userPresets_.size()))
        return false;

    userPresets_.erase(userPresets_.begin() + index);
    saveUserPresetsToFile();
    return true;
}

bool PresetManager::renameUserPreset(int index, const juce::String& newName)
{
    if (index < 0 || index >= static_cast<int>(userPresets_.size()))
        return false;

    if (newName.isEmpty())
        return false;

    userPresets_[static_cast<size_t>(index)].name = newName;
    saveUserPresetsToFile();

    DBG("[PresetManager] Renamed user preset " << index << " to: " << newName);
    return true;
}

int PresetManager::findUserPresetByName(const juce::String& name) const
{
    for (int i = 0; i < static_cast<int>(userPresets_.size()); ++i)
    {
        if (userPresets_[static_cast<size_t>(i)].name.compareIgnoreCase(name) == 0)
            return i;
    }
    return -1;
}

// =============================================================================
// Accessors
// =============================================================================

juce::StringArray PresetManager::getUserPresetNames() const
{
    juce::StringArray names;
    for (const auto& preset : userPresets_)
        names.add(preset.name);
    return names;
}

// =============================================================================
// PresetBrowser Compatibility
// =============================================================================

std::vector<PresetManager::PresetInfo> PresetManager::getPresetList() const
{
    std::vector<PresetInfo> list;

    // Add factory presets
    for (int i = 0; i < static_cast<int>(factoryPresets_.size()); ++i)
    {
        PresetInfo info;
        info.name = factoryPresets_[static_cast<size_t>(i)].first;
        info.category = "Factory";
        info.path = "factory:" + juce::String(i);
        list.push_back(info);
    }

    // Add user presets
    for (int i = 0; i < static_cast<int>(userPresets_.size()); ++i)
    {
        PresetInfo info;
        info.name = userPresets_[static_cast<size_t>(i)].name;
        info.category = "User";
        info.path = "user:" + juce::String(i);
        list.push_back(info);
    }

    return list;
}

bool PresetManager::loadPreset(const juce::String& path)
{
    if (path.startsWith("factory:"))
    {
        int index = path.substring(8).getIntValue();
        return loadFactoryPreset(index);
    }
    else if (path.startsWith("user:"))
    {
        int index = path.substring(5).getIntValue();
        return loadUserPreset(index);
    }
    return false;
}

// =============================================================================
// Factory Presets
// =============================================================================

void PresetManager::registerFactoryPresets(std::vector<std::pair<juce::String, juce::var>> presets)
{
    factoryPresets_ = std::move(presets);
    DBG("[PresetManager] Registered " << factoryPresets_.size() << " factory presets for " << pluginName_);
}

bool PresetManager::loadFactoryPreset(int index)
{
    if (index < 0 || index >= static_cast<int>(factoryPresets_.size()))
        return false;

    applyState(factoryPresets_[static_cast<size_t>(index)].second);
    return true;
}

juce::StringArray PresetManager::getFactoryPresetNames() const
{
    juce::StringArray names;
    for (const auto& preset : factoryPresets_)
        names.add(preset.first);
    return names;
}

} // namespace bws

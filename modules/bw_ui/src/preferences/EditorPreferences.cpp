// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#include <bw_ui/preferences/EditorPreferences.h>

#include <map>
#include <mutex>

namespace bws::ui
{
namespace
{

juce::PropertiesFile::Options makeCanonicalOptions(const juce::String& pluginName)
{
    juce::PropertiesFile::Options options;
    options.applicationName = pluginName;
    options.filenameSuffix = ".settings";
    options.folderName = "BellweatherStudios";
    options.osxLibrarySubFolder = "Application Support";
    options.storageFormat = juce::PropertiesFile::storeAsXML;
    options.millisecondsBeforeSaving = 0;
    return options;
}

juce::File defaultLegacyScaleFile()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Bellweather Studios")
        .getChildFile("Preferences")
        .getChildFile("WeatherPlugins.settings");
}

void migrateLegacyScale(juce::PropertiesFile& store, const juce::String& pluginName, const juce::File& legacyScaleFile)
{
    if (store.containsKey(EditorPreferences::kEditorScaleKey))
        return;

    if (!legacyScaleFile.existsAsFile())
        return;

    juce::PropertiesFile::Options legacyOptions;
    legacyOptions.applicationName = "WeatherPlugins";
    legacyOptions.filenameSuffix = ".settings";
    legacyOptions.osxLibrarySubFolder = "";
    legacyOptions.ignoreCaseOfKeyNames = true;
    legacyOptions.storageFormat = juce::PropertiesFile::storeAsXML;
    legacyOptions.doNotSave = true;

    juce::PropertiesFile legacy(legacyScaleFile, legacyOptions);
    const juce::String legacyKey = pluginName + "_scale";

    if (legacy.containsKey(legacyKey))
    {
        store.setValue(EditorPreferences::kEditorScaleKey, legacy.getDoubleValue(legacyKey));
        store.saveIfNeeded();
    }
}

std::shared_ptr<juce::PropertiesFile> acquireStore(const juce::String& pluginName,
                                                   const EditorPreferences::Locations& locations)
{
    static std::mutex registryMutex;
    static std::map<juce::String, std::weak_ptr<juce::PropertiesFile>> registry;

    const std::scoped_lock lock(registryMutex);

    const auto key = locations.canonicalFile.getFullPathName();
    if (auto existing = registry[key].lock())
        return existing;

    locations.canonicalFile.getParentDirectory().createDirectory();

    auto store = std::make_shared<juce::PropertiesFile>(locations.canonicalFile, makeCanonicalOptions(pluginName));
    migrateLegacyScale(*store, pluginName, locations.legacyScaleFile);

    registry[key] = store;
    return store;
}

} // namespace

EditorPreferences::EditorPreferences(const juce::String& pluginName)
    : EditorPreferences(pluginName,
                        Locations {makeCanonicalOptions(pluginName).getDefaultFile(), defaultLegacyScaleFile()})
{}

EditorPreferences::EditorPreferences(const juce::String& pluginName, const Locations& locations)
    : store_(acquireStore(pluginName, locations))
{}

bool EditorPreferences::getBool(const juce::String& key, bool defaultValue) const
{
    return store_->getBoolValue(key, defaultValue);
}

int EditorPreferences::getInt(const juce::String& key, int defaultValue) const
{
    return store_->getIntValue(key, defaultValue);
}

double EditorPreferences::getDouble(const juce::String& key, double defaultValue) const
{
    return store_->getDoubleValue(key, defaultValue);
}

juce::String EditorPreferences::getString(const juce::String& key, const juce::String& defaultValue) const
{
    return store_->getValue(key, defaultValue);
}

void EditorPreferences::setBool(const juce::String& key, bool value)
{
    store_->setValue(key, value);
    store_->saveIfNeeded();
}

void EditorPreferences::setInt(const juce::String& key, int value)
{
    store_->setValue(key, value);
    store_->saveIfNeeded();
}

void EditorPreferences::setDouble(const juce::String& key, double value)
{
    store_->setValue(key, value);
    store_->saveIfNeeded();
}

void EditorPreferences::setString(const juce::String& key, const juce::String& value)
{
    store_->setValue(key, value);
    store_->saveIfNeeded();
}

void EditorPreferences::removeValue(const juce::String& key)
{
    store_->removeValue(key);
    store_->saveIfNeeded();
}

} // namespace bws::ui

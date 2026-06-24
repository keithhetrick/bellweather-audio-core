// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "BarometerWeatherPresetManager.h"
#include "BarometerProcessor.h"

#include <bw_juce_adapters/JucePresetFileRepository.h>
#include <bw_juce_adapters/JucePresetStateBridge.h>

#include <set>
#include <vector>

namespace bws::barometer
{

namespace
{
struct LegacyUserPresetRecord
{
    juce::String name;
    juce::var parameters;
};

const juce::Array<juce::var>* legacyPresetArrayFromJson(const juce::var& parsedJson)
{
    if (parsedJson.isArray())
        return parsedJson.getArray();

    if (auto* obj = parsedJson.getDynamicObject())
    {
        auto presets = obj->getProperty("presets");
        if (presets.isArray())
            return presets.getArray();
    }

    return nullptr;
}

juce::var legacyParametersFromObject(juce::DynamicObject& obj)
{
    auto parameters = obj.getProperty("parameters");
    if (parameters.isObject())
        return parameters;

    auto state = obj.getProperty("state");
    if (state.isObject())
        return state;

    return {};
}

juce::File uniqueMigrationArchiveFile(const juce::File& legacyFile)
{
    const auto timestamp = juce::Time::getCurrentTime().formatted("%Y%m%d-%H%M%S");
    const auto parent = legacyFile.getParentDirectory();
    const auto baseName = "UserPresets.json.migrated-" + timestamp;

    auto candidate = parent.getChildFile(baseName);
    int suffix = 2;
    while (candidate.exists())
    {
        candidate = parent.getChildFile(baseName + "-" + juce::String(suffix));
        ++suffix;
    }

    return candidate;
}
} // namespace

BarometerWeatherPresetManager::BarometerWeatherPresetManager(BarometerProcessor& processor)
    : bws::weather::WeatherPresetManager(processor, "BwsBarometer")
    , barometerProcessor_(processor)
{
    configurePresetRepository();
    // Initialize the base class (scans for presets, creates factory presets)
    initialize();
    setPresetStateBridge(
        std::make_unique<bws::adapters::JucePresetStateBridge>(barometerProcessor_, barometerProcessor_.getApvts()));
}

void BarometerWeatherPresetManager::configurePresetRepository()
{
    setPresetRepository(std::make_unique<bws::adapters::JucePresetFileRepository>(
        "BwsBarometer", getUserPresetDirectory(), getBundledFactoryAssetRoots(), presetJsonCodec_));
}

void BarometerWeatherPresetManager::createFactoryPresets()
{
    // =========================================================================
    // Legacy parity: exact match of the original 10 factory presets.
    // Same names, same order, same parameter values.
    // =========================================================================

    // All presets share: width=1.0, swap=off, mono=off, solo=off, trim=0.0
    // Format: gain, phaseL, phaseR, phaseLink, balance, mono, width, swap, soloL, soloR, trim

    // Keep the legacy single-category display while relying on the shared
    // preset system's in-memory declaration-order sort.
    const auto cat = juce::String("Factory");

    addFactoryPreset("Unity", cat,
                     createPresetState(0.0f, false, false, true, 0.0f, false, 1.0f, false, false, false, 0.0f),
                     "Default unity gain with no processing");

    addFactoryPreset("+6 dB Boost", cat,
                     createPresetState(6.0f, false, false, true, 0.0f, false, 1.0f, false, false, false, 0.0f),
                     "Boost level by 6dB");

    addFactoryPreset("+12 dB Boost", cat,
                     createPresetState(12.0f, false, false, true, 0.0f, false, 1.0f, false, false, false, 0.0f),
                     "Boost level by 12dB");

    addFactoryPreset("-6 dB Trim", cat,
                     createPresetState(-6.0f, false, false, true, 0.0f, false, 1.0f, false, false, false, 0.0f),
                     "Reduce level by 6dB");

    addFactoryPreset("-12 dB Trim", cat,
                     createPresetState(-12.0f, false, false, true, 0.0f, false, 1.0f, false, false, false, 0.0f),
                     "Reduce level by 12dB");

    addFactoryPreset("Phase Flip", cat,
                     createPresetState(0.0f, true, true, true, 0.0f, false, 1.0f, false, false, false, 0.0f),
                     "Invert polarity of both channels");

    addFactoryPreset("Phase Flip L", cat,
                     createPresetState(0.0f, true, false, false, 0.0f, false, 1.0f, false, false, false, 0.0f),
                     "Invert polarity of left channel only");

    addFactoryPreset("Phase Flip R", cat,
                     createPresetState(0.0f, false, true, false, 0.0f, false, 1.0f, false, false, false, 0.0f),
                     "Invert polarity of right channel only");

    addFactoryPreset("Full Left", cat,
                     createPresetState(0.0f, false, false, true, -100.0f, false, 1.0f, false, false, false, 0.0f),
                     "Pan balance fully left");

    addFactoryPreset("Full Right", cat,
                     createPresetState(0.0f, false, false, true, 100.0f, false, 1.0f, false, false, false, 0.0f),
                     "Pan balance fully right");
}

juce::ValueTree BarometerWeatherPresetManager::createPresetState(float gain, bool phaseInvertL, bool phaseInvertR,
                                                                 bool phaseLink, float balance, bool monoEnabled,
                                                                 float width, bool swapLR, bool soloL, bool soloR,
                                                                 float outputTrim)
{
    // Temporarily set all parameters on the processor
    auto& apvts = barometerProcessor_.getApvts();

    // Helper to set a parameter
    auto setParam = [&](const char* id, float value) {
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1(value));
    };

    auto setBoolParam = [&](const char* id, bool value) {
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(value ? 1.0f : 0.0f);
    };

    // Set all parameters using Barometer's parameter IDs
    setParam(BarometerProcessor::kGainParamId, gain);
    setBoolParam(BarometerProcessor::kPhaseInvertLParamId, phaseInvertL);
    setBoolParam(BarometerProcessor::kPhaseInvertRParamId, phaseInvertR);
    setBoolParam(BarometerProcessor::kPhaseLinkParamId, phaseLink);
    setParam(BarometerProcessor::kBalanceParamId, balance);
    setBoolParam(BarometerProcessor::kMonoParamId, monoEnabled);
    setParam(BarometerProcessor::kWidthParamId, width);
    setBoolParam(BarometerProcessor::kSwapLRParamId, swapLR);
    setBoolParam(BarometerProcessor::kSoloLParamId, soloL);
    setBoolParam(BarometerProcessor::kSoloRParamId, soloR);
    setParam(BarometerProcessor::kOutputTrimParamId, outputTrim);

    // Capture the state
    juce::MemoryBlock stateData;
    barometerProcessor_.getStateInformation(stateData);

    // Create a ValueTree to hold the state
    juce::ValueTree state("ProcessorState");
    auto base64 = juce::Base64::toBase64(stateData.getData(), stateData.getSize());
    state.setProperty("data", base64, nullptr);

    return state;
}

//==============================================================================
// Legacy compatibility shims
//==============================================================================

int BarometerWeatherPresetManager::findUserPresetByName(const juce::String& name) const
{
    const auto& presets = getAllPresets();
    int userIndex = 0;
    for (const auto& p : presets)
    {
        if (!p.isFactory)
        {
            if (p.name == name)
                return userIndex;
            ++userIndex;
        }
    }
    return -1;
}

bool BarometerWeatherPresetManager::updateUserPreset(int index)
{
    const auto& presets = getAllPresets();
    int userIndex = 0;
    for (const auto& p : presets)
    {
        if (!p.isFactory)
        {
            if (userIndex == index)
                return savePreset(p.name, p.category);
            ++userIndex;
        }
    }
    return false;
}

bool BarometerWeatherPresetManager::deleteUserPreset(int index)
{
    const auto& presets = getAllPresets();
    int userIndex = 0;
    for (const auto& p : presets)
    {
        if (!p.isFactory)
        {
            if (userIndex == index)
                return deletePreset(p.name);
            ++userIndex;
        }
    }
    return false;
}

bool BarometerWeatherPresetManager::renameUserPreset(int index, const juce::String& newName)
{
    const auto& presets = getAllPresets();
    int userIndex = 0;
    for (const auto& p : presets)
    {
        if (!p.isFactory)
        {
            if (userIndex == index)
                return renamePreset(p.name, newName);
            ++userIndex;
        }
    }
    return false;
}

juce::File BarometerWeatherPresetManager::getPresetDirectory() const
{
    return getUserPresetDirectory();
}

juce::File BarometerWeatherPresetManager::getDefaultPresetDirectory() const
{
    return getDefaultUserPresetDirectory();
}

bool BarometerWeatherPresetManager::hasCustomPresetDirectory() const
{
    return isUsingCustomUserPresetDirectory();
}

juce::File BarometerWeatherPresetManager::getCustomPresetDirectory() const
{
    return getUserPresetDirectory();
}

void BarometerWeatherPresetManager::setCustomPresetDirectory(const juce::File& directory)
{
    if (setUserPresetDirectoryOverride(directory))
    {
        configurePresetRepository();
        loadUserPresets();
    }
}

void BarometerWeatherPresetManager::clearCustomPresetDirectory()
{
    clearUserPresetDirectoryOverride();
    configurePresetRepository();
    loadUserPresets();
}

void BarometerWeatherPresetManager::loadUserPresets()
{
    scanForPresets();
}

//==============================================================================
// One-time migration from legacy UserPresets.json
//==============================================================================

void BarometerWeatherPresetManager::migrateLegacyUserPresets()
{
    // Legacy path: ~/Library/Application Support/BellweatherStudios/BwsBarometer/UserPresets.json
    auto appData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
#if JUCE_MAC
    auto legacyDir = appData.getChildFile("BellweatherStudios").getChildFile("BwsBarometer");
#elif JUCE_WINDOWS
    auto legacyDir = appData.getChildFile("BellweatherStudios").getChildFile("BwsBarometer");
#else
    auto legacyDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                         .getChildFile(".config")
                         .getChildFile("BellweatherStudios")
                         .getChildFile("BwsBarometer");
#endif

    auto legacyFile = legacyDir.getChildFile("UserPresets.json");
    migrateLegacyUserPresetsFromFile(legacyFile);
}

bool BarometerWeatherPresetManager::migrateLegacyUserPresetsFromFile(const juce::File& legacyFile)
{
    if (!legacyFile.existsAsFile())
        return false;

    juce::MemoryBlock originalProcessorState;
    barometerProcessor_.getStateInformation(originalProcessorState);

    auto restoreOriginalProcessorState = [&]() {
        if (originalProcessorState.getSize() > 0)
        {
            barometerProcessor_.setStateInformation(originalProcessorState.getData(),
                                                    static_cast<int>(originalProcessorState.getSize()));
        }
    };

    // Parse the legacy JSON
    auto jsonText = legacyFile.loadFileAsString();
    auto parsedJson = juce::JSON::parse(jsonText);
    const auto* presetArray = legacyPresetArrayFromJson(parsedJson);
    if (presetArray == nullptr || presetArray->isEmpty())
        return false;

    std::vector<LegacyUserPresetRecord> legacyPresets;
    std::set<juce::String> seenNames;

    for (const auto& presetVar : *presetArray)
    {
        if (auto* obj = presetVar.getDynamicObject())
        {
            auto name = obj->getProperty("name").toString();
            if (name.isEmpty())
                return false;

            if (!seenNames.insert(name).second)
                return false;

            auto parameters = legacyParametersFromObject(*obj);
            if (!parameters.isObject())
                return false;

            legacyPresets.push_back({name, parameters});
        }
        else
        {
            return false;
        }
    }

    if (legacyPresets.empty())
        return false;

    int migrated = 0;
    for (const auto& legacyPreset : legacyPresets)
    {
        // Apply legacy param values to APVTS, then save as Weather preset.
        // Legacy state is a juce::var with named float properties.
        auto* stateObject = legacyPreset.parameters.getDynamicObject();
        if (stateObject == nullptr)
        {
            restoreOriginalProcessorState();
            return false;
        }

        auto& apvts = barometerProcessor_.getApvts();
        for (const auto& prop : stateObject->getProperties())
        {
            if (auto* param = apvts.getParameter(prop.name.toString()))
                param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(prop.value)));
        }

        if (saveUserPreset(legacyPreset.name))
            ++migrated;
    }

    // Verify every named legacy preset is discoverable before archiving source data.
    scanForPresets();
    const bool allMigrated = migrated == static_cast<int>(legacyPresets.size());
    bool allDiscoverable = allMigrated;
    for (const auto& legacyPreset : legacyPresets)
    {
        if (findUserPresetByName(legacyPreset.name) < 0)
        {
            allDiscoverable = false;
            break;
        }
    }

    restoreOriginalProcessorState();

    if (!allDiscoverable)
        return false;

    return legacyFile.moveFileTo(uniqueMigrationArchiveFile(legacyFile));
}

} // namespace bws::barometer

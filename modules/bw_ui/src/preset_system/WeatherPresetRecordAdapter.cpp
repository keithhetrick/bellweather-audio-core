// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "WeatherPresetRecordAdapter.h"

#include <bw_preset_core/PresetIdentity.h>
#include <bw_preset_core/PresetMetadataPolicy.h>

namespace bws::weather
{
namespace
{
[[nodiscard]] juce::StringArray stringArrayFromCoreStrings(const std::vector<std::string>& values)
{
    juce::StringArray result;
    for (const auto& value : values)
        result.addIfNotAlreadyThere(juce::String::fromUTF8(value.c_str()));
    return result;
}
} // namespace

juce::String juceStringFromStdString(const std::string& value)
{
    return juce::String::fromUTF8(value.c_str());
}

juce::String presetCategoryOrDefault(const juce::String& category, std::string_view fallback)
{
    return juceStringFromStdString(bws::preset::presetCategoryOrDefault(category.toStdString(), fallback));
}

juce::String presetAuthorOrDefault(const juce::String& author, bws::preset::PresetSource source)
{
    return juceStringFromStdString(bws::preset::presetAuthorOrDefault(author.toStdString(), source));
}

juce::String intensityToString(WeatherPresetManager::Preset::Intensity intensity)
{
    return juce::String(bws::preset::presetIntensityLabel(toCorePresetIntensity(intensity)));
}

bws::preset::PresetFormat toCorePresetFormat(WeatherPresetManager::Preset::Format format)
{
    using Format = WeatherPresetManager::Preset::Format;
    switch (format)
    {
    case Format::Json:
        return bws::preset::PresetFormat::Json;
    case Format::LegacyXml:
        return bws::preset::PresetFormat::LegacyXml;
    case Format::InMemoryFactory:
        break;
    }

    return bws::preset::PresetFormat::Unknown;
}

bws::preset::PresetIntensity toCorePresetIntensity(WeatherPresetManager::Preset::Intensity intensity)
{
    using Intensity = WeatherPresetManager::Preset::Intensity;
    switch (intensity)
    {
    case Intensity::Subtle:
        return bws::preset::PresetIntensity::Subtle;
    case Intensity::Balanced:
        return bws::preset::PresetIntensity::Balanced;
    case Intensity::Assertive:
        return bws::preset::PresetIntensity::Assertive;
    case Intensity::Extreme:
        return bws::preset::PresetIntensity::Extreme;
    case Intensity::Unknown:
        break;
    }

    return bws::preset::PresetIntensity::Unknown;
}

WeatherPresetManager::Preset::Format fromCorePresetFormat(bws::preset::PresetFormat format)
{
    using Format = bws::preset::PresetFormat;
    switch (format)
    {
    case Format::Json:
        return WeatherPresetManager::Preset::Format::Json;
    case Format::LegacyXml:
        return WeatherPresetManager::Preset::Format::LegacyXml;
    case Format::Unknown:
        break;
    }

    return WeatherPresetManager::Preset::Format::Json;
}

WeatherPresetManager::Preset::Intensity fromCorePresetIntensity(bws::preset::PresetIntensity intensity)
{
    using Intensity = bws::preset::PresetIntensity;
    switch (intensity)
    {
    case Intensity::Subtle:
        return WeatherPresetManager::Preset::Intensity::Subtle;
    case Intensity::Balanced:
        return WeatherPresetManager::Preset::Intensity::Balanced;
    case Intensity::Assertive:
        return WeatherPresetManager::Preset::Intensity::Assertive;
    case Intensity::Extreme:
        return WeatherPresetManager::Preset::Intensity::Extreme;
    case Intensity::Unknown:
        break;
    }

    return WeatherPresetManager::Preset::Intensity::Unknown;
}

bws::preset::PresetStateBytes stateBytesFromProcessorState(juce::AudioProcessor& processor)
{
    juce::MemoryBlock stateData;
    processor.getStateInformation(stateData);
    return bws::preset::PresetStateBytes(stateData.getData(), stateData.getSize());
}

juce::String base64FromStateBytes(const bws::preset::PresetStateBytes& state)
{
    if (state.empty())
        return {};

    return juce::Base64::toBase64(reinterpret_cast<const void*>(state.data()), state.size());
}

juce::ValueTree makePresetStateTree(const juce::String& pluginName, const juce::String& name,
                                    const juce::String& category, const juce::String& author,
                                    const juce::String& description, const juce::Time& timestamp,
                                    const juce::String& stateBlob, bool isFactory, int sortOrder)
{
    juce::ValueTree presetTree("PresetState");
    presetTree.setProperty("plugin", pluginName, nullptr);
    presetTree.setProperty("name", name, nullptr);
    presetTree.setProperty("category", category, nullptr);
    presetTree.setProperty("author", author, nullptr);
    presetTree.setProperty("description", description, nullptr);
    presetTree.setProperty("date", timestamp.toISO8601(true), nullptr);
    presetTree.setProperty("state", stateBlob, nullptr);
    presetTree.setProperty("factory", isFactory, nullptr);
    if (sortOrder != std::numeric_limits<int>::max())
        presetTree.setProperty("sort_order", sortOrder, nullptr);
    return presetTree;
}

bws::preset::PresetMetadata toCorePresetMetadata(const juce::String& pluginName,
                                                 const WeatherPresetManager::Preset& preset)
{
    bws::preset::PresetMetadata metadata;
    metadata.identity =
        preset.presetId.isNotEmpty()
            ? bws::preset::PresetIdentity::modern(preset.presetId.toStdString(), pluginName.toStdString(),
                                                  preset.name.toStdString(), preset.category.toStdString())
            : bws::preset::PresetIdentity::legacy(pluginName.toStdString(), preset.name.toStdString(),
                                                  preset.category.toStdString());
    metadata.source = preset.isFactory ? bws::preset::PresetSource::Factory : bws::preset::PresetSource::User;
    metadata.format = toCorePresetFormat(preset.format);
    metadata.author = preset.author.toStdString();
    metadata.description = preset.description.toStdString();
    metadata.timestampIso8601 = preset.timestamp.toISO8601(true).toStdString();
    metadata.engine = preset.engine.toStdString();
    metadata.useCase = preset.useCase.toStdString();
    metadata.intensity = toCorePresetIntensity(preset.intensity);
    for (const auto& tag : preset.tags)
        metadata.tags.push_back(tag.toStdString());
    metadata.cleanStarter = preset.isCleanStarter;
    metadata.sortOrder = preset.sortOrder;
    return metadata;
}

bws::preset::PresetRecord toCorePresetRecord(const juce::String& pluginName, const WeatherPresetManager::Preset& preset,
                                             bws::preset::PresetStateBytes state)
{
    bws::preset::PresetRecord record;
    record.metadata = toCorePresetMetadata(pluginName, preset);
    record.state = std::move(state);
    return record;
}

std::optional<WeatherPresetManager::Preset> fromCorePresetRecord(const juce::String& pluginName,
                                                                 const bws::preset::PresetRecord& record)
{
    if (!record.isLoadable())
        return std::nullopt;

    const auto stateBlob = base64FromStateBytes(record.state);
    if (stateBlob.isEmpty())
        return std::nullopt;

    WeatherPresetManager::Preset preset;
    preset.name = juce::String::fromUTF8(record.metadata.identity.name.c_str()).trim();
    preset.category = presetCategoryOrDefault(juce::String::fromUTF8(record.metadata.identity.category.c_str()).trim());
    preset.presetId = juce::String::fromUTF8(record.metadata.identity.stableId.c_str()).trim();
    preset.author = juce::String::fromUTF8(record.metadata.author.c_str());
    preset.description = juce::String::fromUTF8(record.metadata.description.c_str());
    preset.timestamp = juce::Time::fromISO8601(juce::String::fromUTF8(record.metadata.timestampIso8601.c_str()));
    if (preset.timestamp == juce::Time())
        preset.timestamp = juce::Time::getCurrentTime();
    preset.isFactory = record.metadata.source == bws::preset::PresetSource::Factory;
    preset.format = fromCorePresetFormat(record.metadata.format);
    preset.engine = juce::String::fromUTF8(record.metadata.engine.c_str()).trim();
    preset.useCase = juce::String::fromUTF8(record.metadata.useCase.c_str()).trim();
    preset.intensity = fromCorePresetIntensity(record.metadata.intensity);
    preset.tags = stringArrayFromCoreStrings(record.metadata.tags);
    preset.isCleanStarter = record.metadata.cleanStarter;
    preset.sortOrder = record.metadata.sortOrder;
    preset.author = presetAuthorOrDefault(preset.author, record.metadata.source);

    preset.state = makePresetStateTree(pluginName, preset.name, preset.category, preset.author, preset.description,
                                       preset.timestamp, stateBlob, preset.isFactory, preset.sortOrder);
    return preset;
}

} // namespace bws::weather

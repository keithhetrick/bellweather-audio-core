// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_juce_adapters/JucePresetJsonCodec.h"

#include <bw_preset_core/PresetMetadataPolicy.h>

#include <cstring>
#include <optional>

namespace bws::adapters
{
namespace
{

constexpr int kJsonSchemaVersion = 1;

juce::String stringFromBytes(const std::vector<std::byte>& bytes)
{
    if (bytes.empty())
        return {};

    std::string text(bytes.size(), '\0');
    std::memcpy(text.data(), bytes.data(), bytes.size());
    return juce::String::fromUTF8(text.data(), static_cast<int>(text.size()));
}

std::vector<std::byte> bytesFromString(const juce::String& text)
{
    const auto raw = text.toRawUTF8();
    const auto size = std::strlen(raw);
    std::vector<std::byte> bytes(size);
    std::memcpy(bytes.data(), raw, size);
    return bytes;
}

juce::String toJuceString(const std::string& value)
{
    return juce::String::fromUTF8(value.c_str());
}

std::string toStdString(const juce::String& value)
{
    return value.toStdString();
}

juce::var tagsToJsonArray(const std::vector<std::string>& tags)
{
    juce::Array<juce::var> values;
    for (const auto& tag : tags)
        values.add(toJuceString(tag));
    return juce::var(values);
}

std::vector<std::string> tagsFromJsonArray(const juce::var& value)
{
    std::vector<std::string> tags;
    if (!value.isArray())
        return tags;

    if (const auto* array = value.getArray())
    {
        tags.reserve(static_cast<size_t>(array->size()));
        for (const auto& tag : *array)
            tags.push_back(toStdString(tag.toString()));
    }

    return bws::preset::normalizedPresetTags(std::move(tags));
}

juce::String base64FromState(const bws::preset::PresetStateBytes& state)
{
    return juce::Base64::toBase64(state.data(), state.size());
}

std::optional<bws::preset::PresetStateBytes> stateFromBase64(const juce::String& encoded)
{
    juce::MemoryOutputStream decoded;
    if (!juce::Base64::convertFromBase64(decoded, encoded) || decoded.getDataSize() == 0)
        return std::nullopt;

    return bws::preset::PresetStateBytes(decoded.getData(), decoded.getDataSize());
}

bws::preset::PresetSource sourceFromFactoryFlag(bool factory)
{
    return factory ? bws::preset::PresetSource::Factory : bws::preset::PresetSource::User;
}

} // namespace

bws::preset::PresetFormat JucePresetJsonCodec::format() const
{
    return bws::preset::PresetFormat::Json;
}

bool JucePresetJsonCodec::canDecode(const bws::preset::PresetEncodedDocument& document) const
{
    return document.format == format() && bws::preset::encodedDocumentHasBytes(document);
}

bws::preset::PresetStorageValueResult<bws::preset::PresetRecord> JucePresetJsonCodec::decode(
    const bws::preset::PresetEncodedDocument& document) const
{
    if (!canDecode(document))
        return bws::preset::PresetStorageValueResult<bws::preset::PresetRecord>::failure(
            bws::preset::PresetStorageStatus::UnsupportedFormat);

    const auto parsed = juce::JSON::parse(stringFromBytes(document.bytes));
    if (!parsed.isObject())
        return bws::preset::PresetStorageValueResult<bws::preset::PresetRecord>::failure(
            bws::preset::PresetStorageStatus::ReadFailed, "preset JSON is not an object");

    const auto* object = parsed.getDynamicObject();
    if (object == nullptr)
        return bws::preset::PresetStorageValueResult<bws::preset::PresetRecord>::failure(
            bws::preset::PresetStorageStatus::ReadFailed, "preset JSON object is unavailable");

    auto pluginId = object->getProperty("plugin_id").toString().trim();
    if (pluginId.isEmpty())
        pluginId = object->getProperty("plugin_name").toString().trim();
    const auto name = object->getProperty("name").toString().trim();
    const auto category = toJuceString(
        bws::preset::presetCategoryOrDefault(toStdString(object->getProperty("category").toString().trim())));

    const auto state = stateFromBase64(object->getProperty("state_blob_b64").toString());
    if (pluginId.isEmpty() || name.isEmpty() || !state.has_value())
        return bws::preset::PresetStorageValueResult<bws::preset::PresetRecord>::failure(
            bws::preset::PresetStorageStatus::InvalidArgument, "preset JSON is missing required identity or state");

    bws::preset::PresetMetadata metadata;
    metadata.identity =
        bws::preset::PresetIdentity::legacy(toStdString(pluginId), toStdString(name), toStdString(category));
    metadata.source = sourceFromFactoryFlag(static_cast<bool>(object->getProperty("factory")));
    metadata.format = format();
    metadata.author =
        bws::preset::presetAuthorOrDefault(toStdString(object->getProperty("author").toString()), metadata.source);
    metadata.description = toStdString(object->getProperty("description").toString());
    metadata.timestampIso8601 = toStdString(object->getProperty("timestamp").toString());
    if (object->hasProperty("sort_order"))
        metadata.sortOrder = static_cast<int>(object->getProperty("sort_order"));

    if (const auto metadataValue = object->getProperty("metadata"); metadataValue.isObject())
    {
        if (const auto* metadataObject = metadataValue.getDynamicObject())
        {
            const auto stableId = metadataObject->getProperty("preset_id").toString().trim();
            if (stableId.isNotEmpty())
            {
                metadata.identity = bws::preset::PresetIdentity::modern(toStdString(stableId), toStdString(pluginId),
                                                                        toStdString(name), toStdString(category));
            }

            metadata.engine = toStdString(metadataObject->getProperty("engine").toString().trim());
            metadata.useCase = toStdString(metadataObject->getProperty("use_case").toString().trim());
            metadata.intensity =
                bws::preset::presetIntensityFromLabel(toStdString(metadataObject->getProperty("intensity").toString()));
            metadata.tags = tagsFromJsonArray(metadataObject->getProperty("tags"));
            metadata.cleanStarter = static_cast<bool>(metadataObject->getProperty("is_clean_starter"));
        }
    }

    return bws::preset::PresetStorageValueResult<bws::preset::PresetRecord>::success(
        bws::preset::PresetRecord {std::move(metadata), *state});
}

bws::preset::PresetStorageValueResult<bws::preset::PresetEncodedDocument> JucePresetJsonCodec::encode(
    const bws::preset::PresetRecord& record) const
{
    if (!record.isLoadable())
        return bws::preset::PresetStorageValueResult<bws::preset::PresetEncodedDocument>::failure(
            bws::preset::PresetStorageStatus::InvalidArgument, "preset record is not loadable");

    auto* object = new juce::DynamicObject();
    object->setProperty("schema_version", kJsonSchemaVersion);
    object->setProperty("plugin_id", toJuceString(record.metadata.identity.pluginId));
    object->setProperty("plugin_name", toJuceString(record.metadata.identity.pluginId));
    object->setProperty("name", toJuceString(record.metadata.identity.name));
    object->setProperty("category", toJuceString(record.metadata.identity.category));
    object->setProperty("author", toJuceString(record.metadata.author));
    object->setProperty("description", toJuceString(record.metadata.description));
    object->setProperty("timestamp", toJuceString(record.metadata.timestampIso8601));
    object->setProperty("factory", record.metadata.isFactory());
    if (record.metadata.hasAuthoredSortOrder())
        object->setProperty("sort_order", record.metadata.sortOrder);
    object->setProperty("state_blob_b64", base64FromState(record.state));

    auto* metadata = new juce::DynamicObject();
    if (record.metadata.identity.hasStableId())
        metadata->setProperty("preset_id", toJuceString(record.metadata.identity.stableId));
    if (!record.metadata.engine.empty())
        metadata->setProperty("engine", toJuceString(record.metadata.engine));
    if (!record.metadata.useCase.empty())
        metadata->setProperty("use_case", toJuceString(record.metadata.useCase));
    if (const auto intensity = bws::preset::presetIntensityLabel(record.metadata.intensity); !intensity.empty())
    {
        metadata->setProperty("intensity", toJuceString(intensity));
    }
    if (!record.metadata.tags.empty())
        metadata->setProperty("tags", tagsToJsonArray(record.metadata.tags));
    if (record.metadata.cleanStarter)
        metadata->setProperty("is_clean_starter", true);
    object->setProperty("metadata", metadata);

    const auto json = juce::JSON::toString(juce::var(object), true);
    return bws::preset::PresetStorageValueResult<bws::preset::PresetEncodedDocument>::success(
        bws::preset::makeEncodedDocument(format(), bytesFromString(json)));
}

} // namespace bws::adapters

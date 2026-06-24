// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_preset_core/PresetRecord.h>

#include <string>

namespace bws::preset
{

[[nodiscard]] inline std::string makeFactoryPresetLibraryKey(std::string pluginId, std::string stableId)
{
    const auto normalizedPluginId = normalizedIdentityText(std::move(pluginId));
    const auto normalizedStableId = normalizedIdentityText(std::move(stableId));
    if (normalizedPluginId.empty() || normalizedStableId.empty())
        return {};

    return normalizedPluginId + "::factory::" + normalizedStableId;
}

[[nodiscard]] inline std::string makeFactoryLegacyPresetLibraryKey(std::string pluginId, std::string category,
                                                                   std::string name)
{
    const auto normalizedPluginId = normalizedIdentityText(std::move(pluginId));
    const auto normalizedCategory = normalizedIdentityText(std::move(category));
    const auto normalizedName = normalizedIdentityText(std::move(name));
    if (normalizedPluginId.empty() || normalizedName.empty())
        return {};

    return normalizedPluginId + "::factory-legacy::" + normalizedCategory + "::" + normalizedName;
}

[[nodiscard]] inline std::string makeUserStablePresetLibraryKey(std::string pluginId, std::string stableId)
{
    const auto normalizedPluginId = normalizedIdentityText(std::move(pluginId));
    const auto normalizedStableId = normalizedIdentityText(std::move(stableId));
    if (normalizedPluginId.empty() || normalizedStableId.empty())
        return {};

    return normalizedPluginId + "::user-id::" + normalizedStableId;
}

[[nodiscard]] inline std::string makeUserLegacyPresetLibraryKey(std::string pluginId, std::string category,
                                                                std::string name)
{
    const auto normalizedPluginId = normalizedIdentityText(std::move(pluginId));
    const auto normalizedCategory = normalizedIdentityText(std::move(category));
    const auto normalizedName = normalizedIdentityText(std::move(name));
    if (normalizedPluginId.empty() || normalizedName.empty())
        return {};

    return normalizedPluginId + "::user::" + normalizedCategory + "::" + normalizedName;
}

[[nodiscard]] inline std::string makePresetLibraryKey(const PresetMetadata& metadata)
{
    if (metadata.source == PresetSource::Factory && metadata.identity.hasStableId())
        return makeFactoryPresetLibraryKey(metadata.identity.pluginId, metadata.identity.stableId);

    if (metadata.source == PresetSource::Factory)
    {
        return makeFactoryLegacyPresetLibraryKey(metadata.identity.pluginId, metadata.identity.category,
                                                 metadata.identity.name);
    }

    if (metadata.source == PresetSource::User && metadata.identity.hasStableId())
        return makeUserStablePresetLibraryKey(metadata.identity.pluginId, metadata.identity.stableId);

    if (metadata.source == PresetSource::User || metadata.source == PresetSource::Unknown)
    {
        return makeUserLegacyPresetLibraryKey(metadata.identity.pluginId, metadata.identity.category,
                                              metadata.identity.name);
    }

    return {};
}

struct PresetLibraryIdentity
{
    PresetIdentity identity;
    PresetSource source = PresetSource::Unknown;
    std::string key;

    [[nodiscard]] bool isValid() const { return !key.empty() || identity.isValid(); }

    bool operator==(const PresetLibraryIdentity& other) const
    {
        if (!key.empty() && !other.key.empty())
            return normalizedIdentityText(key) == normalizedIdentityText(other.key);

        return identity == other.identity;
    }

    bool operator!=(const PresetLibraryIdentity& other) const { return !(*this == other); }
};

[[nodiscard]] inline PresetLibraryIdentity makePresetLibraryIdentity(const PresetMetadata& metadata)
{
    return PresetLibraryIdentity {metadata.identity, metadata.source, makePresetLibraryKey(metadata)};
}

} // namespace bws::preset

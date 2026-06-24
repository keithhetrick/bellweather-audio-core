// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_preset_core/PresetIdentity.h>
#include <bw_preset_core/PresetState.h>

#include <algorithm>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace bws::preset
{

inline constexpr int kUnspecifiedPresetSortOrder = std::numeric_limits<int>::max();
inline constexpr int kCurrentJsonPresetSchemaVersion = 1;

enum class PresetSource
{
    Unknown,
    Factory,
    User
};

enum class PresetIntensity
{
    Unknown,
    Subtle,
    Balanced,
    Assertive,
    Extreme
};

[[nodiscard]] inline std::string presetIntensityLabel(PresetIntensity intensity)
{
    switch (intensity)
    {
    case PresetIntensity::Subtle:
        return "subtle";
    case PresetIntensity::Balanced:
        return "balanced";
    case PresetIntensity::Assertive:
        return "assertive";
    case PresetIntensity::Extreme:
        return "extreme";
    case PresetIntensity::Unknown:
        break;
    }

    return {};
}

[[nodiscard]] inline PresetIntensity presetIntensityFromLabel(std::string label)
{
    label = normalizedIdentityText(std::move(label));

    if (label == "subtle")
        return PresetIntensity::Subtle;
    if (label == "balanced")
        return PresetIntensity::Balanced;
    if (label == "assertive")
        return PresetIntensity::Assertive;
    if (label == "extreme")
        return PresetIntensity::Extreme;

    return PresetIntensity::Unknown;
}

struct PresetMetadata
{
    PresetIdentity identity;
    PresetSource source = PresetSource::Unknown;
    PresetFormat format = PresetFormat::Unknown;
    std::string author;
    std::string description;
    std::string timestampIso8601;
    std::string engine;
    std::string useCase;
    PresetIntensity intensity = PresetIntensity::Unknown;
    std::vector<std::string> tags;
    bool cleanStarter = false;
    int sortOrder = kUnspecifiedPresetSortOrder;

    [[nodiscard]] bool isFactory() const { return source == PresetSource::Factory; }
    [[nodiscard]] bool isUser() const { return source == PresetSource::User; }

    [[nodiscard]] bool hasStableIdentity() const { return identity.hasStableId(); }

    [[nodiscard]] bool hasAuthoredSortOrder() const { return sortOrder != kUnspecifiedPresetSortOrder; }

    [[nodiscard]] bool hasRequiredDisplayFields() const { return !identity.name.empty() && !identity.category.empty(); }

    [[nodiscard]] PresetMetadata renamed(std::string nextName, std::string nextCategory = {}) const
    {
        auto copy = *this;
        copy.identity = copy.identity.renamed(std::move(nextName), std::move(nextCategory));
        return copy;
    }
};

struct PresetRecord
{
    PresetMetadata metadata;
    PresetStateBytes state;

    [[nodiscard]] bool hasState() const { return !state.empty(); }

    [[nodiscard]] bool isLoadable() const
    {
        return metadata.identity.isValid() && metadata.hasRequiredDisplayFields() && hasState();
    }

    [[nodiscard]] PresetRecord renamed(std::string nextName, std::string nextCategory = {}) const
    {
        auto copy = *this;
        copy.metadata = copy.metadata.renamed(std::move(nextName), std::move(nextCategory));
        return copy;
    }
};

[[nodiscard]] inline bool isKnownPresetFileExtension(const std::string& extension)
{
    return normalizedIdentityText(extension) == extensionForPresetFormat(PresetFormat::Json) ||
           normalizedIdentityText(extension) == extensionForPresetFormat(PresetFormat::LegacyXml);
}

[[nodiscard]] inline std::vector<std::string> normalizedPresetTags(std::vector<std::string> tags)
{
    for (auto& tag : tags)
        tag = normalizedIdentityText(std::move(tag));

    tags.erase(std::remove_if(tags.begin(), tags.end(), [](const std::string& tag) { return tag.empty(); }),
               tags.end());
    std::sort(tags.begin(), tags.end());
    tags.erase(std::unique(tags.begin(), tags.end()), tags.end());
    return tags;
}

[[nodiscard]] inline std::string stableIdForUserPresetSave(const PresetIdentity& requestedIdentity,
                                                           const std::vector<PresetMetadata>& existingMetadata,
                                                           std::string generatedStableId)
{
    for (const auto& metadata : existingMetadata)
    {
        if (!metadata.isUser() || !metadata.identity.hasStableId())
            continue;

        if (matchesPresetLookupIdentity(metadata.identity, requestedIdentity))
            return metadata.identity.stableId;
    }

    return generatedStableId;
}

} // namespace bws::preset

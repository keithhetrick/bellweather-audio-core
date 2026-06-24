// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>
#include <vector>

namespace bws::preset
{

enum class PresetFormat
{
    Unknown,
    Json,
    LegacyXml
};

[[nodiscard]] inline std::string extensionForPresetFormat(PresetFormat format)
{
    switch (format)
    {
    case PresetFormat::Json:
        return ".bwpreset.json";
    case PresetFormat::LegacyXml:
        return ".bwspreset";
    case PresetFormat::Unknown:
        break;
    }

    return {};
}

[[nodiscard]] inline std::string normalizedIdentityText(std::string value)
{
    const auto first =
        std::find_if_not(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c) != 0; });
    const auto last =
        std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) { return std::isspace(c) != 0; }).base();

    if (first >= last)
        return {};

    std::string normalized(first, last);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return normalized;
}

struct PresetIdentity
{
    std::string stableId;
    std::string pluginId;
    std::string name;
    std::string category;

    [[nodiscard]] static PresetIdentity modern(std::string stableId, std::string pluginId, std::string name,
                                               std::string category)
    {
        return PresetIdentity {std::move(stableId), std::move(pluginId), std::move(name), std::move(category)};
    }

    [[nodiscard]] static PresetIdentity legacy(std::string pluginId, std::string name, std::string category)
    {
        return PresetIdentity {{}, std::move(pluginId), std::move(name), std::move(category)};
    }

    [[nodiscard]] bool hasStableId() const { return !stableId.empty(); }

    [[nodiscard]] bool isValid() const { return hasStableId() || (!pluginId.empty() && !name.empty()); }

    [[nodiscard]] std::string legacyKey() const
    {
        return "legacy:" + normalizedIdentityText(pluginId) + "::" + normalizedIdentityText(category) +
               "::" + normalizedIdentityText(name);
    }

    [[nodiscard]] std::string canonicalKey() const
    {
        if (hasStableId())
            return "id:" + normalizedIdentityText(stableId);

        return legacyKey();
    }

    [[nodiscard]] bool isSameLogicalPresetAs(const PresetIdentity& other) const
    {
        if (!isValid() || !other.isValid())
            return false;

        if (hasStableId() || other.hasStableId())
            return hasStableId() && other.hasStableId() &&
                   normalizedIdentityText(stableId) == normalizedIdentityText(other.stableId);

        return legacyKey() == other.legacyKey();
    }

    [[nodiscard]] bool isLegacyCompatibleWith(const PresetIdentity& other) const
    {
        return isValid() && other.isValid() && legacyKey() == other.legacyKey();
    }

    [[nodiscard]] PresetIdentity renamed(std::string nextName, std::string nextCategory = {}) const
    {
        auto copy = *this;
        copy.name = std::move(nextName);
        if (!nextCategory.empty())
            copy.category = std::move(nextCategory);
        return copy;
    }

    bool operator==(const PresetIdentity& other) const { return isSameLogicalPresetAs(other); }
};

[[nodiscard]] inline bool matchesPresetLookupIdentity(const PresetIdentity& stored, const PresetIdentity& lookup)
{
    if (!stored.isValid() || !lookup.isValid())
        return false;

    if (stored.hasStableId() && lookup.hasStableId())
        return normalizedIdentityText(stored.stableId) == normalizedIdentityText(lookup.stableId);

    if (lookup.hasStableId())
        return false;

    if (stored.hasStableId())
        return stored.isLegacyCompatibleWith(lookup);

    return stored.isSameLogicalPresetAs(lookup);
}

[[nodiscard]] inline std::vector<PresetIdentity> presetRestoreLookupIdentities(std::string pluginId, std::string name,
                                                                               std::string category,
                                                                               std::string stableId)
{
    std::vector<PresetIdentity> identities;
    if (!stableId.empty())
    {
        identities.push_back(PresetIdentity::modern(stableId, pluginId, name, category));
    }

    if (!name.empty())
    {
        identities.push_back(PresetIdentity::legacy(std::move(pluginId), std::move(name), std::move(category)));
    }

    return identities;
}

} // namespace bws::preset

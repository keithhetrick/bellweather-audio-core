// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_preset_core/PresetNavigation.h>
#include <bw_preset_core/PresetRecord.h>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace bws::preset
{

struct PresetCatalogSortOptions
{
    bool factoryFirst = true;
    bool authoredSortOrder = true;
    bool categoryThenName = true;
};

enum class PresetCatalogSourceFilter
{
    All,
    Factory,
    User
};

enum class PresetCatalogCurationFilter
{
    All,
    Favorites,
    CleanStarters
};

struct PresetCatalogQuery
{
    std::string searchText;
    PresetCatalogSourceFilter source = PresetCatalogSourceFilter::All;
    PresetCatalogCurationFilter curation = PresetCatalogCurationFilter::All;
    std::string engine;
    std::string useCase;
    PresetIntensity intensity = PresetIntensity::Unknown;
};

struct PresetCatalogEntry
{
    std::size_t index = 0;
    PresetMetadata metadata;
    bool favorite = false;
    int recentRank = -1;
};

[[nodiscard]] inline std::vector<PresetMetadata> orderedPresetMetadata(std::vector<PresetMetadata> presets,
                                                                       PresetCatalogSortOptions options = {})
{
    std::stable_sort(presets.begin(), presets.end(), [&](const auto& a, const auto& b) {
        if (options.factoryFirst && a.source != b.source)
        {
            if (a.source == PresetSource::Factory)
                return true;
            if (b.source == PresetSource::Factory)
                return false;
        }

        if (options.authoredSortOrder && a.source == PresetSource::Factory && b.source == PresetSource::Factory &&
            a.sortOrder != b.sortOrder)
            return a.sortOrder < b.sortOrder;

        if (options.categoryThenName)
        {
            const auto aCategory = normalizedIdentityText(a.identity.category);
            const auto bCategory = normalizedIdentityText(b.identity.category);
            if (aCategory != bCategory)
                return aCategory < bCategory;

            const auto aName = normalizedIdentityText(a.identity.name);
            const auto bName = normalizedIdentityText(b.identity.name);
            if (aName != bName)
                return aName < bName;
        }

        return false;
    });

    return presets;
}

[[nodiscard]] inline bool presetCatalogContainsNormalized(std::string value, const std::string& normalizedSearch)
{
    value = normalizedIdentityText(std::move(value));
    return !normalizedSearch.empty() && value.find(normalizedSearch) != std::string::npos;
}

[[nodiscard]] inline int searchRankForPresetMetadata(const PresetMetadata& metadata, std::string searchText)
{
    const auto normalizedSearch = normalizedIdentityText(std::move(searchText));
    if (normalizedSearch.empty())
        return 0;

    const auto name = normalizedIdentityText(metadata.identity.name);
    if (name == normalizedSearch)
        return 1;
    if (name.find(normalizedSearch) != std::string::npos)
        return 2;

    if (presetCatalogContainsNormalized(metadata.engine, normalizedSearch) ||
        presetCatalogContainsNormalized(metadata.useCase, normalizedSearch))
    {
        return 3;
    }

    for (const auto& tag : metadata.tags)
    {
        if (presetCatalogContainsNormalized(tag, normalizedSearch))
            return 4;
    }

    if (presetCatalogContainsNormalized(metadata.description, normalizedSearch))
        return 5;

    return std::numeric_limits<int>::max();
}

[[nodiscard]] inline bool presetCatalogEntryMatchesQuery(const PresetCatalogEntry& entry,
                                                         const PresetCatalogQuery& query)
{
    const auto& metadata = entry.metadata;

    if (query.source == PresetCatalogSourceFilter::Factory && !metadata.isFactory())
        return false;
    if (query.source == PresetCatalogSourceFilter::User && !metadata.isUser())
        return false;

    if (query.curation == PresetCatalogCurationFilter::Favorites && !entry.favorite)
        return false;
    if (query.curation == PresetCatalogCurationFilter::CleanStarters && !metadata.cleanStarter)
        return false;

    if (!query.engine.empty() && normalizedIdentityText(metadata.engine) != normalizedIdentityText(query.engine))
        return false;
    if (!query.useCase.empty() && normalizedIdentityText(metadata.useCase) != normalizedIdentityText(query.useCase))
        return false;
    if (query.intensity != PresetIntensity::Unknown && metadata.intensity != query.intensity)
        return false;

    const auto rank = searchRankForPresetMetadata(metadata, query.searchText);
    return normalizedIdentityText(query.searchText).empty() || rank != std::numeric_limits<int>::max();
}

[[nodiscard]] inline bool presetCatalogEntryCanonicalLess(const PresetCatalogEntry& a, const PresetCatalogEntry& b)
{
    if (a.metadata.source != b.metadata.source)
        return a.metadata.isFactory();

    if (a.metadata.isFactory() && a.metadata.sortOrder != b.metadata.sortOrder)
        return a.metadata.sortOrder < b.metadata.sortOrder;

    if (!a.metadata.isFactory() && a.recentRank != b.recentRank && a.recentRank >= 0 && b.recentRank >= 0)
    {
        return a.recentRank < b.recentRank;
    }

    const auto aCategory = normalizedIdentityText(a.metadata.identity.category);
    const auto bCategory = normalizedIdentityText(b.metadata.identity.category);
    if (aCategory != bCategory)
        return aCategory < bCategory;

    return normalizedIdentityText(a.metadata.identity.name) < normalizedIdentityText(b.metadata.identity.name);
}

[[nodiscard]] inline std::vector<std::size_t> queryPresetCatalogEntries(std::vector<PresetCatalogEntry> entries,
                                                                        const PresetCatalogQuery& query)
{
    struct Match
    {
        PresetCatalogEntry entry;
        int searchRank = std::numeric_limits<int>::max();
    };

    std::vector<Match> matches;
    matches.reserve(entries.size());
    for (auto& entry : entries)
    {
        if (!presetCatalogEntryMatchesQuery(entry, query))
            continue;

        const auto searchRank = searchRankForPresetMetadata(entry.metadata, query.searchText);
        matches.push_back({std::move(entry), searchRank});
    }

    std::sort(matches.begin(), matches.end(), [](const Match& a, const Match& b) {
        if (a.searchRank != b.searchRank)
            return a.searchRank < b.searchRank;
        return presetCatalogEntryCanonicalLess(a.entry, b.entry);
    });

    if (!query.engine.empty())
    {
        const auto queryEngine = normalizedIdentityText(query.engine);
        const auto it = std::find_if(matches.begin(), matches.end(), [&](const Match& match) {
            return match.entry.metadata.cleanStarter &&
                   normalizedIdentityText(match.entry.metadata.engine) == queryEngine;
        });

        if (it != matches.end() && it != matches.begin())
            std::rotate(matches.begin(), it, std::next(it));
    }

    std::vector<std::size_t> indices;
    indices.reserve(matches.size());
    for (const auto& match : matches)
        indices.push_back(match.entry.index);
    return indices;
}

template <typename KeyGetter>
[[nodiscard]] inline std::vector<std::size_t> presetCatalogNavigationAttemptIndices(
    std::vector<PresetCatalogEntry> entries, const PresetCatalogQuery& query, const std::string& currentKey,
    PresetNavigationDirection direction, KeyGetter&& keyGetter)
{
    const auto orderedIndices = queryPresetCatalogEntries(entries, query);
    auto&& getKey = keyGetter;

    std::unordered_map<std::size_t, std::string> keysByIndex;
    keysByIndex.reserve(entries.size());
    for (const auto& entry : entries)
        keysByIndex.emplace(entry.index, getKey(entry));

    std::vector<std::string> orderedKeys;
    orderedKeys.reserve(orderedIndices.size());
    for (const auto orderedIndex : orderedIndices)
    {
        const auto found = keysByIndex.find(orderedIndex);
        orderedKeys.push_back(found != keysByIndex.end() ? found->second : std::string {});
    }

    const auto attemptPositions = presetNavigationAttemptOrder(orderedKeys, currentKey, direction);

    std::vector<std::size_t> attemptIndices;
    attemptIndices.reserve(attemptPositions.size());
    for (const auto attemptPosition : attemptPositions)
        attemptIndices.push_back(orderedIndices[attemptPosition]);

    return attemptIndices;
}

[[nodiscard]] inline std::vector<std::string> presetDisplayNames(const std::vector<PresetMetadata>& presets)
{
    std::vector<std::string> names;
    names.reserve(presets.size());
    for (const auto& preset : presets)
        names.push_back(preset.identity.name);
    return names;
}

[[nodiscard]] inline const PresetMetadata* findPresetMetadata(const std::vector<PresetMetadata>& presets,
                                                              const PresetIdentity& identity)
{
    const auto found = std::find_if(presets.begin(), presets.end(), [&](const auto& preset) {
        return matchesPresetLookupIdentity(preset.identity, identity);
    });

    if (found == presets.end())
        return nullptr;

    return &(*found);
}

[[nodiscard]] inline std::optional<std::size_t> findPresetRestoreIndex(const std::vector<PresetMetadata>& presets,
                                                                       std::string pluginId, std::string name,
                                                                       std::string category, std::string stableId,
                                                                       bool allowNameOnlyFallback = true)
{
    if (!stableId.empty())
    {
        const auto stableLookup = PresetIdentity::modern(stableId, pluginId, name, category);
        for (std::size_t i = 0; i < presets.size(); ++i)
        {
            if (matchesPresetLookupIdentity(presets[i].identity, stableLookup))
                return i;
        }

        return std::nullopt;
    }

    const auto lookupIdentities = presetRestoreLookupIdentities(pluginId, name, category, stableId);

    for (const auto& lookupIdentity : lookupIdentities)
    {
        for (std::size_t i = 0; i < presets.size(); ++i)
        {
            if (matchesPresetLookupIdentity(presets[i].identity, lookupIdentity))
                return i;
        }
    }

    if (allowNameOnlyFallback && !name.empty() && category.empty())
    {
        for (std::size_t i = 0; i < presets.size(); ++i)
        {
            if (presets[i].identity.name == name)
                return i;
        }
    }

    return std::nullopt;
}

} // namespace bws::preset

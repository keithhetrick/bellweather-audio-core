// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <algorithm>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace bws::preset
{

inline constexpr std::string_view kPresetLibraryStateFileName = "library_state.json";
inline constexpr int kDefaultMaxRecentPresets = 12;

struct PresetLibraryState
{
    std::set<std::string> favoriteKeys;
    std::vector<std::string> recentPresetKeys;
    std::string startupPresetKey;

    bool operator==(const PresetLibraryState&) const = default;
};

[[nodiscard]] inline PresetLibraryState migrateLibraryStateKey(PresetLibraryState state, const std::string& oldKey,
                                                               const std::string& newKey)
{
    if (oldKey.empty() || newKey.empty() || oldKey == newKey)
        return state;

    if (state.favoriteKeys.erase(oldKey) > 0)
        state.favoriteKeys.insert(newKey);

    for (auto& recentKey : state.recentPresetKeys)
    {
        if (recentKey == oldKey)
            recentKey = newKey;
    }

    if (state.startupPresetKey == oldKey)
        state.startupPresetKey = newKey;

    return state;
}

[[nodiscard]] inline PresetLibraryState removeLibraryStateKey(PresetLibraryState state, const std::string& key)
{
    if (key.empty())
        return state;

    state.favoriteKeys.erase(key);
    state.recentPresetKeys.erase(std::remove(state.recentPresetKeys.begin(), state.recentPresetKeys.end(), key),
                                 state.recentPresetKeys.end());

    if (state.startupPresetKey == key)
        state.startupPresetKey.clear();

    return state;
}

[[nodiscard]] inline PresetLibraryState promoteRecentPreset(PresetLibraryState state, const std::string& key,
                                                            int maxRecentPresets = kDefaultMaxRecentPresets)
{
    if (key.empty() || maxRecentPresets <= 0)
        return state;

    state.recentPresetKeys.erase(std::remove(state.recentPresetKeys.begin(), state.recentPresetKeys.end(), key),
                                 state.recentPresetKeys.end());
    state.recentPresetKeys.insert(state.recentPresetKeys.begin(), key);

    if (static_cast<int>(state.recentPresetKeys.size()) > maxRecentPresets)
        state.recentPresetKeys.resize(static_cast<size_t>(maxRecentPresets));

    return state;
}

[[nodiscard]] inline int recentPresetRank(const PresetLibraryState& state, const std::string& key)
{
    if (key.empty())
        return -1;

    const auto found = std::find(state.recentPresetKeys.begin(), state.recentPresetKeys.end(), key);
    if (found == state.recentPresetKeys.end())
        return -1;

    return static_cast<int>(std::distance(state.recentPresetKeys.begin(), found));
}

[[nodiscard]] inline PresetLibraryState clearInvalidLibraryStateReferences(PresetLibraryState state,
                                                                           const std::set<std::string>& validKeys)
{
    for (auto it = state.favoriteKeys.begin(); it != state.favoriteKeys.end();)
    {
        if (!validKeys.contains(*it))
            it = state.favoriteKeys.erase(it);
        else
            ++it;
    }

    state.recentPresetKeys.erase(
        std::remove_if(state.recentPresetKeys.begin(), state.recentPresetKeys.end(),
                       [&validKeys](const std::string& key) { return !validKeys.contains(key); }),
        state.recentPresetKeys.end());

    if (!state.startupPresetKey.empty() && !validKeys.contains(state.startupPresetKey))
        state.startupPresetKey.clear();

    return state;
}

} // namespace bws::preset

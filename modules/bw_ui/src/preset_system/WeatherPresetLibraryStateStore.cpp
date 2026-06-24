// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "WeatherPresetLibraryStateStore.h"

#include <algorithm>

namespace bws::weather
{

std::optional<bws::preset::PresetLibraryState> parsePresetLibraryStateDocument(const juce::var& document)
{
    if (!document.isObject())
        return std::nullopt;

    const auto* object = document.getDynamicObject();
    if (object == nullptr)
        return std::nullopt;

    bws::preset::PresetLibraryState state;

    if (auto* favorites = object->getProperty("favorites").getArray())
    {
        for (const auto& item : *favorites)
        {
            const auto key = item.toString().trim();
            if (key.isNotEmpty())
                state.favoriteKeys.insert(key.toStdString());
        }
    }

    if (auto* recents = object->getProperty("recents").getArray())
    {
        for (const auto& item : *recents)
        {
            const auto key = item.toString().trim();
            if (key.isNotEmpty() && std::find(state.recentPresetKeys.begin(), state.recentPresetKeys.end(),
                                              key.toStdString()) == state.recentPresetKeys.end())
            {
                state.recentPresetKeys.push_back(key.toStdString());
            }
        }
    }

    state.startupPresetKey = object->getProperty("startup_default").toString().trim().toStdString();
    return state;
}

juce::var makePresetLibraryStateDocument(const bws::preset::PresetLibraryState& state)
{
    auto* object = new juce::DynamicObject();

    juce::Array<juce::var> favorites;
    for (const auto& key : state.favoriteKeys)
        favorites.add(juce::String(key));
    object->setProperty("favorites", favorites);

    juce::Array<juce::var> recents;
    for (const auto& key : state.recentPresetKeys)
        recents.add(juce::String(key));
    object->setProperty("recents", recents);

    object->setProperty("startup_default", juce::String(state.startupPresetKey));
    return juce::var(object);
}

} // namespace bws::weather

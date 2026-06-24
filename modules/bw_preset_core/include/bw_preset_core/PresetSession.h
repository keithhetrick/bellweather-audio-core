// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_preset_core/PresetIdentity.h>
#include <bw_preset_core/PresetNavigation.h>

#include <string>
#include <utility>
#include <vector>

namespace bws::preset
{

enum class PresetSessionNavigationMode
{
    LoadCommittedPreset,
    PreviewBrowserPreset
};

template <typename Identity>
struct PresetSessionStateT
{
    bool browserOpen = false;
    bool previewSessionActive = false;
    Identity committedPresetIdentity;
    Identity previewedPresetIdentity;
    Identity selectedBrowserIdentity;
    int presetListRevision = 0;

    bool operator==(const PresetSessionStateT&) const = default;
};

template <typename Identity>
struct PresetSessionActionT
{
    enum class Kind
    {
        BrowserOpened,
        BrowserSelectionChanged,
        PreviewedPresetChanged,
        PreviewCommitted,
        PreviewCommittedAndContinued,
        PreviewCancelled,
        ExternalPresetChanged,
        PresetListRefreshed
    };

    Kind kind = Kind::ExternalPresetChanged;
    Identity identity;
    int presetListRevision = 0;
};

template <typename Identity>
[[nodiscard]] PresetSessionStateT<Identity> makePresetSessionState(Identity currentIdentity,
                                                                   bool previewSessionActive = false,
                                                                   int presetListRevision = 0)
{
    return PresetSessionStateT<Identity> {false,           previewSessionActive, currentIdentity,
                                          currentIdentity, currentIdentity,      presetListRevision};
}

template <typename Identity>
[[nodiscard]] PresetSessionStateT<Identity> reducePresetSessionState(const PresetSessionStateT<Identity>& state,
                                                                     const PresetSessionActionT<Identity>& action)
{
    auto next = state;

    switch (action.kind)
    {
    case PresetSessionActionT<Identity>::Kind::BrowserOpened:
        next.browserOpen = true;
        next.previewSessionActive = true;
        next.committedPresetIdentity = action.identity;
        next.previewedPresetIdentity = action.identity;
        next.selectedBrowserIdentity = action.identity;
        break;

    case PresetSessionActionT<Identity>::Kind::BrowserSelectionChanged:
        next.selectedBrowserIdentity = action.identity;
        break;

    case PresetSessionActionT<Identity>::Kind::PreviewedPresetChanged:
        next.previewedPresetIdentity = action.identity;
        next.selectedBrowserIdentity = action.identity;
        break;

    case PresetSessionActionT<Identity>::Kind::PreviewCommitted:
        next.browserOpen = false;
        next.previewSessionActive = false;
        next.committedPresetIdentity = action.identity;
        next.previewedPresetIdentity = action.identity;
        next.selectedBrowserIdentity = action.identity;
        break;

    case PresetSessionActionT<Identity>::Kind::PreviewCommittedAndContinued:
        next.browserOpen = true;
        next.previewSessionActive = true;
        next.committedPresetIdentity = action.identity;
        next.previewedPresetIdentity = action.identity;
        next.selectedBrowserIdentity = action.identity;
        break;

    case PresetSessionActionT<Identity>::Kind::PreviewCancelled:
        next.browserOpen = false;
        next.previewSessionActive = false;
        next.committedPresetIdentity = action.identity;
        next.previewedPresetIdentity = action.identity;
        next.selectedBrowserIdentity = action.identity;
        break;

    case PresetSessionActionT<Identity>::Kind::ExternalPresetChanged:
        if (next.previewSessionActive)
        {
            next.previewedPresetIdentity = action.identity;
            next.selectedBrowserIdentity = action.identity;
        }
        else
        {
            next.committedPresetIdentity = action.identity;
            next.previewedPresetIdentity = action.identity;
            next.selectedBrowserIdentity = action.identity;
        }
        break;

    case PresetSessionActionT<Identity>::Kind::PresetListRefreshed:
        next.presetListRevision = action.presetListRevision;
        break;
    }

    return next;
}

template <typename Identity>
[[nodiscard]] const Identity& presetSessionNavigationAnchor(const PresetSessionStateT<Identity>& state,
                                                            const Identity& fallbackIdentity)
{
    if (state.previewSessionActive && state.selectedBrowserIdentity.isValid())
        return state.selectedBrowserIdentity;

    if (state.previewedPresetIdentity.isValid())
        return state.previewedPresetIdentity;

    if (state.committedPresetIdentity.isValid())
        return state.committedPresetIdentity;

    return fallbackIdentity;
}

template <typename Identity, typename KeyGetter>
[[nodiscard]] std::string presetSessionNavigationAnchorKey(const PresetSessionStateT<Identity>& state,
                                                           const Identity& fallbackIdentity, KeyGetter&& keyGetter)
{
    return std::forward<KeyGetter>(keyGetter)(presetSessionNavigationAnchor(state, fallbackIdentity));
}

template <typename Identity, typename KeyGetter>
[[nodiscard]] std::vector<std::size_t> presetSessionNavigationAttemptOrder(
    const PresetSessionStateT<Identity>& state, const Identity& fallbackIdentity,
    const std::vector<std::string>& orderedPresetKeys, PresetNavigationDirection direction, KeyGetter&& keyGetter)
{
    return presetNavigationAttemptOrder(
        orderedPresetKeys,
        presetSessionNavigationAnchorKey(state, fallbackIdentity, std::forward<KeyGetter>(keyGetter)), direction);
}

template <typename Identity, typename KeyGetter, typename ContainsKey>
[[nodiscard]] std::string presetSessionBrowserProjectionKey(const PresetSessionStateT<Identity>& state,
                                                            const Identity& currentIdentity, KeyGetter&& keyGetter,
                                                            ContainsKey&& containsKey)
{
    auto&& getKey = keyGetter;
    auto&& hasKey = containsKey;
    const auto currentKey = getKey(currentIdentity);
    auto previewedKey = getKey(state.previewedPresetIdentity);
    auto selectedKey = getKey(state.selectedBrowserIdentity);

    if (previewedKey.empty())
        previewedKey = currentKey;

    if (!previewedKey.empty() && !hasKey(previewedKey))
        previewedKey.clear();

    if (!selectedKey.empty() && !hasKey(selectedKey))
        selectedKey.clear();

    if (!selectedKey.empty())
        return selectedKey;

    if (!previewedKey.empty())
        return previewedKey;

    return currentKey;
}

template <typename Identity>
[[nodiscard]] PresetSessionNavigationMode presetSessionNavigationMode(const PresetSessionStateT<Identity>& state)
{
    if (state.browserOpen && state.previewSessionActive)
        return PresetSessionNavigationMode::PreviewBrowserPreset;

    return PresetSessionNavigationMode::LoadCommittedPreset;
}

using PresetSessionState = PresetSessionStateT<PresetIdentity>;
using PresetSessionAction = PresetSessionActionT<PresetIdentity>;

} // namespace bws::preset

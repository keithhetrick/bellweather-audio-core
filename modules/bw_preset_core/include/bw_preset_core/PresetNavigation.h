// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace bws::preset
{

enum class PresetNavigationDirection
{
    Next,
    Previous
};

[[nodiscard]] inline std::vector<std::size_t> presetNavigationAttemptOrder(
    const std::vector<std::string>& orderedPresetKeys, const std::string& currentPresetKey,
    PresetNavigationDirection direction)
{
    std::vector<std::size_t> attempts;
    const auto presetCount = orderedPresetKeys.size();
    if (presetCount == 0)
        return attempts;

    attempts.reserve(presetCount);

    int orderedPosition = direction == PresetNavigationDirection::Next ? -1 : 0;
    if (!currentPresetKey.empty())
    {
        for (std::size_t i = 0; i < presetCount; ++i)
        {
            if (orderedPresetKeys[i] == currentPresetKey)
            {
                orderedPosition = static_cast<int>(i);
                break;
            }
        }
    }

    for (std::size_t attempt = 0; attempt < presetCount; ++attempt)
    {
        if (direction == PresetNavigationDirection::Next)
            orderedPosition = (orderedPosition + 1) % static_cast<int>(presetCount);
        else
            orderedPosition = (orderedPosition - 1 + static_cast<int>(presetCount)) % static_cast<int>(presetCount);

        attempts.push_back(static_cast<std::size_t>(orderedPosition));
    }

    return attempts;
}

} // namespace bws::preset

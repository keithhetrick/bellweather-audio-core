// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_preset_core/PresetRecord.h>
#include <bw_preset_core/PresetUuid.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <string_view>

namespace bws::preset
{

inline constexpr int kImplicitFactorySortOrderBase = 10000;

[[nodiscard]] inline int implicitFactorySortOrderForDeclarationIndex(std::size_t declarationIndex)
{
    return kImplicitFactorySortOrderBase + static_cast<int>(declarationIndex);
}

[[nodiscard]] inline constexpr std::array<std::string_view, 12> allowedFactoryPresetTags()
{
    return {"glue",    "transparent", "punchy",     "warm",       "vintage", "saturated",
            "forward", "smooth",      "controlled", "aggressive", "dense",   "wide"};
}

[[nodiscard]] inline bool isAllowedFactoryPresetTag(std::string tag)
{
    tag = normalizedIdentityText(std::move(tag));
    const auto allowed = allowedFactoryPresetTags();
    return std::find(allowed.begin(), allowed.end(), tag) != allowed.end();
}

[[nodiscard]] inline bool hasRequiredFactoryPresetCurationMetadata(const PresetMetadata& metadata)
{
    if (!metadata.isFactory())
        return true;

    if (!isValidUuidV4(metadata.identity.stableId) || metadata.engine.empty() || metadata.useCase.empty() ||
        metadata.intensity == PresetIntensity::Unknown)
    {
        return false;
    }

    return std::all_of(metadata.tags.begin(), metadata.tags.end(),
                       [](const auto& tag) { return isAllowedFactoryPresetTag(tag); });
}

} // namespace bws::preset

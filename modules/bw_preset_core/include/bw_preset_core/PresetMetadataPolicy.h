// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_preset_core/PresetRecord.h>

#include <string>
#include <string_view>

namespace bws::preset
{

inline constexpr std::string_view kDefaultPresetCategory = "Uncategorized";
inline constexpr std::string_view kDefaultUserPresetCategory = "User";
inline constexpr std::string_view kDefaultFactoryPresetAuthor = "Bellweather Studios";
inline constexpr std::string_view kDefaultUserPresetAuthor = "User";

[[nodiscard]] inline std::string presetCategoryOrDefault(std::string category,
                                                         std::string_view fallback = kDefaultPresetCategory)
{
    if (normalizedIdentityText(category).empty())
        return std::string(fallback);

    return category;
}

[[nodiscard]] inline std::string presetAuthorOrDefault(std::string author, PresetSource source)
{
    if (!normalizedIdentityText(author).empty())
        return author;

    return source == PresetSource::Factory ? std::string(kDefaultFactoryPresetAuthor)
                                           : std::string(kDefaultUserPresetAuthor);
}

} // namespace bws::preset

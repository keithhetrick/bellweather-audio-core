// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_preset_core/PresetRecord.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace bws::preset
{

struct PresetRelativePath
{
    std::vector<std::string> directories;
    std::string filename;

    [[nodiscard]] bool isValid() const { return !filename.empty(); }
};

[[nodiscard]] inline std::string trimmedPresetText(std::string value)
{
    const auto first =
        std::find_if_not(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c) != 0; });
    const auto last =
        std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) { return std::isspace(c) != 0; }).base();

    if (first >= last)
        return {};

    return std::string(first, last);
}

[[nodiscard]] inline bool isUnsafePresetPathByte(unsigned char c)
{
    switch (c)
    {
    case '/':
    case '\\':
    case ':':
    case '*':
    case '?':
    case '"':
    case '<':
    case '>':
    case '|':
        return true;
    default:
        return std::iscntrl(c) != 0;
    }
}

[[nodiscard]] inline std::string safePresetPathSegment(std::string value, std::string fallback)
{
    value = trimmedPresetText(std::move(value));
    if (value.empty())
        value = trimmedPresetText(std::move(fallback));

    for (auto& c : value)
    {
        if (isUnsafePresetPathByte(static_cast<unsigned char>(c)))
            c = '_';
    }

    value = trimmedPresetText(std::move(value));
    if (value.empty())
        return "Untitled";

    return value;
}

[[nodiscard]] inline PresetRelativePath userPresetRelativePathForMetadata(const PresetMetadata& metadata)
{
    const auto extension = extensionForPresetFormat(metadata.format);
    if (extension.empty())
        return {};

    return PresetRelativePath {{safePresetPathSegment(metadata.identity.category, "User")},
                               safePresetPathSegment(metadata.identity.name, "Untitled") + extension};
}

} // namespace bws::preset

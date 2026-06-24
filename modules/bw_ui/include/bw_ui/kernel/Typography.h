// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>

namespace bws::ui::kernel
{
enum class FontWeight
{
    Regular,
    Medium,
    SemiBold,
    Bold
};

enum class TextRole
{
    BrandSmall,
    Title,
    SectionLabel,
    ControlText,
    Readout,
    Tab,
    Overlay,
    HeaderBrand,
    HeaderPreset,
    Annotation
};

constexpr std::size_t kTextRoleCount = 10;

struct TextStyle
{
    float height {12.0f};
    FontWeight weight {FontWeight::Regular};
    float tracking {0.0f};
    bool uppercase {false};
};

std::size_t toIndex(TextRole role) noexcept;

} // namespace bws::ui::kernel

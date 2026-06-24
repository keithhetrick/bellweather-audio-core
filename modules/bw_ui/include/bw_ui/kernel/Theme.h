// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "bw_ui/kernel/Spacing.h"
#include "bw_ui/kernel/Typography.h"

#include <array>

namespace bws::ui::kernel
{
enum class ProductThemeId
{
    Shared,
    Barometer,
    Unknown
};

struct ThemeSnapshot
{
    ProductThemeId productTheme {ProductThemeId::Shared};
    Density density {Density::Standard};
    float densityScale {1.0f};
    SpacingScale spacing {};
    HeaderSpacingMetrics headerSpacing {};
    ReadoutMetricSet readouts {};
    std::array<TextStyle, kTextRoleCount> textStyles {};
};

const TextStyle& textStyle(const ThemeSnapshot& theme, TextRole role) noexcept;

} // namespace bws::ui::kernel

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace bws::ui::kernel
{
enum class Density
{
    Standard,
    Compact
};

struct SpacingScale
{
    float xxs {2.0f};
    float xs {4.0f};
    float sm {8.0f};
    float md {12.0f};
    float lg {16.0f};
    float xl {24.0f};
    float xxl {32.0f};
};

struct HeaderSpacingMetrics
{
    float brandZoneWidth {120.0f};
    float iconSize {16.0f};
    float iconPadding {8.0f};
    float nameIconGap {4.0f};
    float nameRightPad {8.0f};
    float nameCategoryGap {2.0f};
    float funcLeftOffset {10.0f};
    float navWidth {22.0f};
    float saveWidth {28.0f};
    float abWidth {36.0f};
    float settingsWidth {22.0f};
    float rightInset {8.0f};
    float gap {6.0f};
    float renamePadV {4.0f};
};

struct ReadoutMetrics
{
    float minWidth {70.0f};
    float height {28.0f};
    float paddingX {8.0f};
    float paddingY {4.0f};
    float stripHeight {34.0f};
    float buttonWidth {130.0f};
};

struct ReadoutMetricSet
{
    ReadoutMetrics standard {};
    ReadoutMetrics compact {};
};

} // namespace bws::ui::kernel

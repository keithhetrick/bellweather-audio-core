// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace bws::ui
{

struct ArcTickMetrics
{
    float outerDelta;
    float innerDelta;
    float lineThickness;
    float labelInnerDelta;
    float labelFontScale;
    float labelBoxW;
    float labelBoxH;
    float labelOffsetX;
    float labelOffsetY;
};

// Ratios calibrated so a 200px (hero) knob reproduces the historical pixel values.
[[nodiscard]] inline ArcTickMetrics resolveArcTickMetrics(float diameter) noexcept
{
    constexpr float kBaseDiameter = 200.0f;
    const float s = diameter / kBaseDiameter;
    return ArcTickMetrics {.outerDelta = 4.0f * s,
                           .innerDelta = 8.0f * s,
                           .lineThickness = 2.0f * s,
                           .labelInnerDelta = 8.0f * s,
                           .labelFontScale = s,
                           .labelBoxW = 12.0f * s,
                           .labelBoxH = 10.0f * s,
                           .labelOffsetX = 6.0f * s,
                           .labelOffsetY = 5.0f * s};
}

} // namespace bws::ui

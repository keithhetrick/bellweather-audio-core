// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "bw_ui/foundation/UiTheme.h"
#include "bw_ui/generated/BwTokens.h"
#include "bw_ui/rendering/IRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace bws::ui
{

// Derived readout geometry. Coefficients are tokens; the floors are clamps (a
// control cannot collapse below a legible minimum), not design values.
struct ReadoutGeometry
{
    float cornerRadius;
    float outlineThickness;
    float verticalSafety;
    float shellPaddingX;
    float minShellWidth;
    float compactVerticalPad;
    float standardVerticalPad;
    float underlineWidthFactor;
    float underlineYOffsetFactor;
    float underlineThicknessFactor;
    float selectionCornerFactor;
    float caretHeightFactor;
};

inline ReadoutGeometry resolveReadoutGeometry(const UiReadoutMetrics& m, bool compact) noexcept
{
    namespace d = bws::tokens::shared::readout_derivation;
    ReadoutGeometry g;
    g.cornerRadius = compact ? std::max(4.0f, m.height * d::COMPACT_CORNER_FACTOR) : m.cornerRadius;
    g.outlineThickness = m.outlineThickness;
    g.verticalSafety = compact ? std::max(2.0f, m.paddingY * d::VERTICAL_SAFETY_COMPACT_FACTOR)
                               : std::max(2.0f, m.paddingY * d::VERTICAL_SAFETY_STANDARD_FACTOR);
    g.shellPaddingX = std::max(8.0f, m.paddingX + d::SHELL_PADDING_XADD);
    g.minShellWidth = std::max(26.0f, m.height * d::MIN_SHELL_WIDTH_FACTOR);
    g.compactVerticalPad = std::floor(m.paddingY * d::COMPACT_VERTICAL_PAD_FACTOR);
    g.standardVerticalPad = std::floor(m.paddingY);
    g.underlineWidthFactor = d::UNDERLINE_WIDTH_FACTOR;
    g.underlineYOffsetFactor = d::UNDERLINE_YOFFSET_FACTOR;
    g.underlineThicknessFactor = d::UNDERLINE_THICKNESS_FACTOR;
    g.selectionCornerFactor = d::SELECTION_CORNER_FACTOR;
    g.caretHeightFactor = d::CARET_HEIGHT_FACTOR;
    return g;
}

// Emits the pill fill + outline as a backend-agnostic command stream.
inline void paintReadoutPill(rendering::IRenderer& r, float x, float y, float w, float h, std::uint32_t fillArgb,
                             std::uint32_t outlineArgb, float cornerRadius, float outlineThickness) noexcept
{
    r.setColour(fillArgb);
    r.fillRoundedRect(x, y, w, h, cornerRadius);
    r.setColour(outlineArgb);
    r.drawRoundedRect(x, y, w, h, cornerRadius, outlineThickness);
}

} // namespace bws::ui

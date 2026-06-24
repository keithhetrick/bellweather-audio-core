// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * @file GoldenRatioConstants.h
 * @brief Golden Ratio Design System - Bellweather Studios
 *
 * DESIGN PHILOSOPHY:
 * All spacing and sizing derives from golden ratio (φ = 1.618...) relationships.
 * This creates mathematical harmony and visual consistency across all BWS plugins.
 *
 * USAGE:
 * @code
 * #include <bw_ui/GoldenRatioConstants.h>
 *
 * // In your layout code:
 * int gap = bws::design::kSpacingM;  // 16px base unit
 * int heroKnobSize = bws::design::kHeroPrimaryKnob;  // 144px Fibonacci
 * @endcode
 *
 * CORE PRINCIPLE:
 * φ (phi) = 1.618033988749895
 *
 * Spacing hierarchy:
 *   φ^-3 ≈ 0.236 → micro spacing
 *   φ^-2 ≈ 0.382 → small spacing
 *   φ^-1 ≈ 0.618 → medium spacing
 *   φ^0  = 1.000 → base unit
 *   φ^1  ≈ 1.618 → large spacing
 *   φ^2  ≈ 2.618 → extra large spacing
 */

namespace bws::design
{

//==============================================================================
// GOLDEN RATIO CORE CONSTANTS
//==============================================================================

/** Golden ratio constant (φ) */
constexpr float kPhi = 1.618033988749895f;

/** Inverse golden ratio (1/φ ≈ 0.618) */
constexpr float kPhiInv = 1.0f / kPhi;

/** Golden ratio squared (φ² ≈ 2.618) */
constexpr float kPhiSq = kPhi * kPhi;

/** Inverse golden ratio squared (φ^-2 ≈ 0.382) */
constexpr float kPhiInvSq = kPhiInv * kPhiInv;

//==============================================================================
// BASE UNIT
//==============================================================================

/**
 * Base unit: 16px
 * Common UI foundation, divisible nicely.
 * All other dimensions derive from this using golden ratio.
 */
constexpr int kBaseUnit = 16;

//==============================================================================
// SPACING HIERARCHY (derived from base unit and φ)
//==============================================================================

/** Extra small spacing: ~6px (base × φ^-2) - micro gaps */
constexpr int kSpacingXS = static_cast<int>(kBaseUnit * kPhiInvSq);

/** Small spacing: ~10px (base × φ^-1) - tight elements */
constexpr int kSpacingS = static_cast<int>(kBaseUnit * kPhiInv);

/** Medium spacing: 16px (base) - standard gap */
constexpr int kSpacingM = kBaseUnit;

/** Large spacing: ~26px (base × φ) - section separation */
constexpr int kSpacingL = static_cast<int>(kBaseUnit * kPhi);

/** Extra large spacing: ~42px (base × φ²) - major divisions */
constexpr int kSpacingXL = static_cast<int>(kBaseUnit * kPhiSq);

//==============================================================================
// FIBONACCI SPACING (for precise control layouts)
//==============================================================================

/** Fibonacci 8: Micro gap (label to legend) */
constexpr int kFibonacci8 = 8;

/** Fibonacci 13: Small gap (knob to label) */
constexpr int kFibonacci13 = 13;

/** Fibonacci 21: Medium gap (section spacing) */
constexpr int kFibonacci21 = 21;

/** Fibonacci 34: Large gap */
constexpr int kFibonacci34 = 34;

/** Fibonacci 55: Extra large gap */
constexpr int kFibonacci55 = 55;

/** Fibonacci 89: Hero knob size (secondary) */
constexpr int kFibonacci89 = 89;

/** Fibonacci 144: Hero knob size (primary) */
constexpr int kFibonacci144 = 144;

//==============================================================================
// SEMANTIC ALIASES (for readability)
//==============================================================================

/** Gap between knob bottom and label top */
constexpr int kGapKnobLabel = kFibonacci13;

/** Gap between label bottom and legend top */
constexpr int kGapLabelLegend = kFibonacci8;

/** Gap between sections */
constexpr int kGapSection = kFibonacci21;

/** Standard margin */
constexpr int kMargin = kSpacingM;

//==============================================================================
// HERO CONTROL DIMENSIONS (3-knob layouts)
//==============================================================================

/** Primary hero knob size: 144px (Fibonacci) */
constexpr int kHeroPrimaryKnob = kFibonacci144;

/** Secondary hero knob size: 89px (144/φ ≈ 89, also Fibonacci) */
constexpr int kHeroSecondaryKnob = kFibonacci89;

/** Gap between hero knobs: 21px (Fibonacci) */
constexpr int kHeroKnobGap = kFibonacci21;

/** Total height for hero section with 3 knobs + labels */
constexpr int kHeroSectionHeight = 240;

//==============================================================================
// PRECISION CONTROL DIMENSIONS (grid layouts)
//==============================================================================

/** Precision grid spacing ratio: φ^-1 × 0.25 ≈ 0.155 */
constexpr float kPrecisionSpacingRatio = kPhiInv * 0.25f;

/** Precision control label height: ~10px */
constexpr int kPrecisionLabelHeight = kSpacingS;

/** Precision control label gap: ~3px (micro) */
constexpr int kPrecisionLabelGap = static_cast<int>(kSpacingXS * 0.5f);

/** Precision toggle height: 16px */
constexpr int kPrecisionToggleHeight = kSpacingM;

/** Precision toggle row height: ~26px */
constexpr int kPrecisionToggleRowHeight = kSpacingL;

//==============================================================================
// TIER SYSTEM DIMENSIONS
//==============================================================================

/**
 * Plugin Size (1-4 controls)
 * Where the dial IS the plugin at 80-90% width
 */
constexpr int kTier1Width = 400;
constexpr int kTier1Height = 500;

/**
 * Plugin Size (5+ controls)
 * Where the environment IS the identity
 */
constexpr int kTier2Width = 600;
constexpr int kTier2Height = 720;

/**
 * (Flagship) Plugin Size
 */
constexpr int kTier0Width = 800;
constexpr int kTier0Height = 1000;

//==============================================================================
// HELPER FUNCTIONS
//==============================================================================

/**
 * Calculate a dimension using golden ratio
 * @param base Base dimension
 * @param power Power of phi (-2, -1, 0, 1, 2)
 * @return Dimension scaled by φ^power
 */
constexpr int goldenScale(int base, int power)
{
    if (power == 0)
        return base;
    if (power == 1)
        return static_cast<int>(static_cast<float>(base) * kPhi);
    if (power == -1)
        return static_cast<int>(static_cast<float>(base) * kPhiInv);
    if (power == 2)
        return static_cast<int>(static_cast<float>(base) * kPhiSq);
    if (power == -2)
        return static_cast<int>(static_cast<float>(base) * kPhiInvSq);

    // For other powers, calculate iteratively
    float scale = 1.0f;
    for (int i = 0; i < power; ++i)
        scale *= kPhi;
    for (int i = 0; i > power; --i)
        scale *= kPhiInv;
    return static_cast<int>(static_cast<float>(base) * scale);
}

/**
 * Calculate spacing between two points using golden ratio
 * @param distance Total distance to divide
 * @param useGoldenRatio If true, divides at φ point; if false, uses center
 * @return Position of division point
 */
constexpr int goldenDivision(int distance, bool useGoldenRatio = true)
{
    return useGoldenRatio ? static_cast<int>(static_cast<float>(distance) * kPhiInv) : distance / 2;
}

} // namespace bws::design

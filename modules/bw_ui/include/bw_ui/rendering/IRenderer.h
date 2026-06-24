// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>

namespace bws::ui::rendering
{

/**
 * IRenderer - provider-agnostic 2D rendering interface.
 *
 * Same pattern as IPaymentProvider on the web side: the interface models
 * what plugins need to draw, not what any backend calls its API.
 * JUCE, Visage, Skia, Metal - each adapts to this interface.
 * Swap the adapter, zero plugin code changes.
 *
 * Zero JUCE imports. Colour is ARGB uint32_t. Fonts are size + weight.
 * Geometry is float primitives. Gradients use opaque handles (same
 * pattern as ShaderHandle/PipelineHandle in IGpuDevice).
 *
 * Threading: message thread only.
 */

// -------------------------------------------------------------------------
// Value types
// -------------------------------------------------------------------------

enum class FontWeight
{
    Regular,
    Bold,
    Mono
};
enum class Justification
{
    Left,
    Centre,
    Right,
    CentreBottom
};
enum class WindingRule
{
    NonZero,
    EvenOdd
};

struct GradientHandle
{
    uint32_t id = 0;
};

// -------------------------------------------------------------------------
// Interface
// -------------------------------------------------------------------------

class IRenderer
{
public:
    virtual ~IRenderer() = default;

    // ── Colour + state ──────────────────────────────────────────────────

    virtual void setColour(uint32_t argb) = 0;
    virtual void setOpacity(float alpha) = 0;
    virtual void saveState() = 0;
    virtual void restoreState() = 0;
    virtual void clipRect(float x, float y, float w, float h) = 0;

    // ── Shapes ──────────────────────────────────────────────────────────

    virtual void fillRect(float x, float y, float w, float h) = 0;
    virtual void fillRoundedRect(float x, float y, float w, float h, float radius) = 0;
    virtual void fillEllipse(float x, float y, float w, float h) = 0;
    virtual void drawRect(float x, float y, float w, float h, float thickness) = 0;
    virtual void drawRoundedRect(float x, float y, float w, float h, float radius, float thickness) = 0;
    virtual void drawEllipse(float x, float y, float w, float h, float thickness) = 0;
    virtual void drawLine(float x1, float y1, float x2, float y2, float thickness) = 0;

    // ── Text ────────────────────────────────────────────────────────────

    virtual void setFont(float size, FontWeight weight) = 0;
    virtual void drawText(const char* text, float x, float y, float w, float h, Justification j) = 0;

    // ── Gradients (opaque handle pattern) ───────────────────────────────

    [[nodiscard]] virtual GradientHandle createLinearGradient(float x1, float y1, float x2, float y2) = 0;
    [[nodiscard]] virtual GradientHandle createRadialGradient(float cx, float cy, float radius) = 0;
    [[nodiscard]] virtual GradientHandle createRadialGradient(float x1, float y1, float x2, float y2) = 0;
    virtual void addGradientStop(GradientHandle h, float position, uint32_t argb) = 0;
    virtual void applyGradient(GradientHandle h) = 0;
    virtual void destroyGradient(GradientHandle h) = 0;

    // ── Path builder ────────────────────────────────────────────────────

    virtual void beginPath() = 0;
    virtual void moveTo(float x, float y) = 0;
    virtual void lineTo(float x, float y) = 0;
    virtual void quadraticTo(float cx, float cy, float x, float y) = 0;
    virtual void cubicTo(float c1x, float c1y, float c2x, float c2y, float x, float y) = 0;
    virtual void arcTo(float cx, float cy, float radius, float startAngle, float endAngle) = 0;
    virtual void closePath() = 0;
    virtual void fillPath(WindingRule rule = WindingRule::NonZero) = 0;
    virtual void setLineDash(const float* pattern, int count) = 0;
    virtual void clearLineDash() = 0;
    virtual void strokePath(float thickness) = 0;
    virtual void clipPath() = 0;
};

} // namespace bws::ui::rendering

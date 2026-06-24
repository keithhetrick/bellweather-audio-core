// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_ui/rendering/IRenderer.h>

#include <algorithm>
#include <cstdint>
#include <span>
#include <vector>

namespace bws::barometer::paint
{

namespace rendering = bws::ui::rendering;

// Pack an alpha onto a 0x00RRGGBB token, dropping any existing alpha.
[[nodiscard]] inline std::uint32_t withAlpha(std::uint32_t rgb, float alpha) noexcept
{
    const auto a = static_cast<std::uint32_t>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f + 0.5f);
    return (a << 24) | (rgb & 0x00FFFFFFu);
}

// Short-term LUFS trend sparkline: auto-ranged, decimated, Catmull-Rom stroked.
// history is the ring buffer; writePos is the oldest sample. Pure: emits draw commands
inline void sparkline(rendering::IRenderer& r, std::span<const float> history, std::size_t writePos, float x, float y,
                      float w, float h, float strokeThickness, std::uint32_t strokeArgb)
{
    const std::size_t total = history.size();
    const std::size_t decimateN = std::max<std::size_t>(1, total / 60);

    float minVal = 1000.0f;
    float maxVal = -1000.0f;
    for (float v : history)
    {
        if (v > -69.0f)
        {
            minVal = std::min(minVal, v);
            maxVal = std::max(maxVal, v);
        }
    }
    if (maxVal < minVal)
        return; // no valid samples yet

    if (maxVal - minVal < 2.0f) // keep a steady level visible
    {
        const float mid = (minVal + maxVal) * 0.5f;
        minVal = mid - 1.0f;
        maxVal = mid + 1.0f;
    }
    const float range = maxVal - minVal;

    struct Pt
    {
        float x, y;
    };
    std::vector<Pt> points;
    points.reserve(total / decimateN + 1);

    for (std::size_t i = 0; i < total; i += decimateN)
    {
        const std::size_t end = std::min(i + decimateN, total);
        float sum = 0.0f;
        std::size_t count = 0;
        for (std::size_t j = i; j < end; ++j)
        {
            const float v = history[(writePos + j) % total];
            if (v > -69.0f)
            {
                sum += v;
                ++count;
            }
        }
        if (count == 0)
            continue;

        const float avg = sum / static_cast<float>(count);
        const float px = x + w * (static_cast<float>(i) / static_cast<float>(total));
        float py = y + h * (1.0f - ((avg - minVal) / range));
        py = std::clamp(py, y, y + h);
        points.push_back({px, py});
    }

    if (points.size() < 2)
        return;

    r.setColour(strokeArgb);
    r.beginPath();
    r.moveTo(points[0].x, points[0].y);
    for (std::size_t i = 0; i + 1 < points.size(); ++i)
    {
        const Pt p0 = points[i == 0 ? 0 : i - 1];
        const Pt p1 = points[i];
        const Pt p2 = points[i + 1];
        const Pt p3 = points[i + 1 == points.size() - 1 ? i + 1 : i + 2];

        r.cubicTo(p1.x + (p2.x - p0.x) / 6.0f, p1.y + (p2.y - p0.y) / 6.0f, p2.x - (p3.x - p1.x) / 6.0f,
                  p2.y - (p3.y - p1.y) / 6.0f, p2.x, p2.y);
    }
    r.strokePath(strokeThickness);
}

struct BarRect
{
    float x, y, w, h;
};
struct MeterBars
{
    BarRect left;
    BarRect right;
};

// Equal-width L/R bars symmetric about the container centre; symmetry keeps paint
// aligned with centre-based hit-testing at any scale.
[[nodiscard]] inline MeterBars meterBarRects(float x, float y, float w, float h, float pad, int barWidth,
                                             int gap) noexcept
{
    const float iy = y + pad;
    const float ih = h - (2.0f * pad);
    const float innerW = w - (2.0f * pad);
    const float barFrac = static_cast<float>(barWidth) / static_cast<float>(2 * barWidth + gap);
    const float barW = innerW * barFrac;
    const float leftX = x + pad;
    const float rightX = x + w - pad - barW;
    return MeterBars {.left = {.x = leftX, .y = iy, .w = barW, .h = ih},
                      .right = {.x = rightX, .y = iy, .w = barW, .h = ih}};
}

// Gradient stops + clip colour for a level meter bar.
struct MeterColors
{
    std::uint32_t green;
    std::uint32_t yellow;
    std::uint32_t red;
    std::uint32_t clip;
};

// Filled meter bar: a green/yellow/red gradient scaled to peakDb over
// [minDb, maxDb], plus a clip cap when peakDb exceeds 0 dB. Bounds are
// integer-valued; the fill height truncates to whole pixels as the meter does.
inline void meterBar(rendering::IRenderer& r, float x, float y, float w, float h, float peakDb, float minDb,
                     float maxDb, const MeterColors& c)
{
    const float range = maxDb - minDb;
    const float normalized = (std::clamp(peakDb, minDb, maxDb) - minDb) / range;
    const int fillHeight = static_cast<int>(normalized * h);

    if (fillHeight > 0)
    {
        const auto g = r.createLinearGradient(x, y + h, x, y);
        r.addGradientStop(g, 0.0f, c.green);
        r.addGradientStop(g, 0.5f, c.yellow);
        r.addGradientStop(g, 0.91f, c.yellow);
        r.addGradientStop(g, 1.0f, c.red);
        r.applyGradient(g);
        r.destroyGradient(g);
        r.fillRoundedRect(x, (y + h) - static_cast<float>(fillHeight), w, static_cast<float>(fillHeight), 1.5f);
    }

    if (peakDb > 0.0f)
    {
        r.setColour(c.clip);
        r.fillRoundedRect(x, y, w, 3.0f, 1.0f);
    }
}

// 2 px horizontal RMS tick at rmsDb, drawn only when inside the bar.
inline void rmsTick(rendering::IRenderer& r, float x, float y, float w, float h, float rmsDb, float minDb, float maxDb,
                    std::uint32_t argb)
{
    const float range = maxDb - minDb;
    const float normalized = (std::clamp(rmsDb, minDb, maxDb) - minDb) / range;
    const float tickY = (y + h) - static_cast<float>(static_cast<int>(normalized * h));
    if (tickY < (y + h) - 1.0f && tickY > y)
    {
        r.setColour(argb);
        r.fillRect(x, tickY - 1.0f, w, 2.0f);
    }
}

// 1 px peak-hold line at heldDb, drawn only when above the floor and inside the bar.
inline void peakHoldLine(rendering::IRenderer& r, float x, float y, float w, float h, float heldDb, float minDb,
                         float maxDb, std::uint32_t argb)
{
    if (heldDb <= minDb)
    {
        return;
    }
    const float range = maxDb - minDb;
    const float normalized = (std::clamp(heldDb, minDb, maxDb) - minDb) / range;
    const float holdY = (y + h) - static_cast<float>(static_cast<int>(normalized * h));
    if (holdY < (y + h) - 1.0f && holdY > y)
    {
        r.setColour(argb);
        r.fillRect(x, holdY, w, 1.0f);
    }
}

} // namespace bws::barometer::paint

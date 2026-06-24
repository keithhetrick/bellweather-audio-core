// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <cmath>
#include "bw_ui/foundation/UiTheme.h"

namespace bws::layout
{

enum class Breakpoint
{
    Compact,
    Regular
};

inline Breakpoint breakpointForWidth(int w)
{
    return w < 520 ? Breakpoint::Compact : Breakpoint::Regular;
}

enum class AreaId
{
    Header,
    Hero,
    Body
};

struct LayoutSpec
{
    bool heroVisible {false};
    int titleH {0};
    int subtitleH {0};
    int paddingX {0};
    int paddingY {0};
    float heroFraction {0.28f};
    float heroMin {140.0f};
    float heroMaxCompact {210.0f};
    float heroMaxRegular {220.0f};
};

struct AreaLayoutResult
{
    juce::Rectangle<int> header;
    juce::Rectangle<int> hero;
    juce::Rectangle<int> body;
};

inline LayoutSpec makeDefaultLayoutSpec(const bws::ui::UiThemeResolved& theme, bool heroVisible)
{
    LayoutSpec spec;
    spec.heroVisible = heroVisible;
    spec.titleH = static_cast<int>(std::lround(theme.spacing.xl + theme.spacing.sm));
    spec.subtitleH = static_cast<int>(std::lround(theme.spacing.md + theme.spacing.xs));
    spec.paddingX = static_cast<int>(std::lround(theme.spacing.xl));
    spec.paddingY = static_cast<int>(std::lround(theme.spacing.lg));
    return spec;
}

inline AreaLayoutResult layoutAreas(juce::Rectangle<int> bounds, const bws::ui::UiThemeResolved& theme,
                                    const LayoutSpec& spec)
{
    bounds.reduce(spec.paddingX, spec.paddingY);

    const int headerHeight = spec.titleH + spec.subtitleH;
    auto headerArea = bounds.removeFromTop(headerHeight);

    juce::Rectangle<int> heroArea;
    auto contentArea = bounds;
    if (spec.heroVisible)
    {
        auto breakpoint = breakpointForWidth(bounds.getWidth());
        float assetW = contentArea.proportionOfWidth(spec.heroFraction);
        const float maxW = (breakpoint == Breakpoint::Compact) ? spec.heroMaxCompact : spec.heroMaxRegular;
        assetW = juce::jlimit<float>(spec.heroMin, maxW, assetW);
        heroArea = contentArea.removeFromLeft(static_cast<int>(assetW));
        heroArea = heroArea.reduced(static_cast<int>(std::lround(theme.spacing.sm)));
    }

    AreaLayoutResult result;
    result.header = headerArea;
    result.hero = heroArea;
    result.body = contentArea;
    return result;
}

inline AreaLayoutResult layoutEditorScaffold(juce::Rectangle<int> bounds, const bws::ui::UiThemeResolved& theme,
                                             bool heroVisible)
{
    return layoutAreas(bounds, theme, makeDefaultLayoutSpec(theme, heroVisible));
}

struct GridSpec
{
    int tileCount {0};
    int gap {0};
    int tileMin {0};
    int tileMax {0};
    int preferredColsHint {0};
    std::vector<int> colSpans;
};

inline bool spansValid(const GridSpec& spec, int cols)
{
    if (spec.colSpans.empty())
        return false;
    if ((int)spec.colSpans.size() != spec.tileCount)
        return false;
    for (auto s : spec.colSpans)
        if (s < 1 || s > cols)
            return false;
    return true;
}

inline int computeRowWidth(const std::vector<int>& spans, int tileW, int gap)
{
    int width = 0;
    for (size_t i = 0; i < spans.size(); ++i)
    {
        const int span = spans[i];
        width += span * tileW;
        width += (span - 1) * gap;
        if (i + 1 < spans.size())
            width += gap;
    }
    return width;
}

inline std::vector<juce::Rectangle<int>> layoutGrid(juce::Rectangle<int> bounds, const GridSpec& spec)
{
    std::vector<juce::Rectangle<int>> rects;
    if (spec.tileCount <= 0)
        return rects;

    const int cols = (spec.preferredColsHint > 0)
                         ? spec.preferredColsHint
                         : (spec.tileCount <= 4 ? 2 : (int)std::ceil(std::sqrt((float)spec.tileCount)));
    const int rows = (int)std::ceil(spec.tileCount / (float)cols);

    const int availW = juce::jmax(1, bounds.getWidth() - (cols + 1) * spec.gap);
    const int availH = juce::jmax(1, bounds.getHeight() - (rows + 1) * spec.gap);
    const int tileW = juce::jlimit(spec.tileMin, spec.tileMax, availW / cols);
    const int tileH = juce::jlimit(spec.tileMin, spec.tileMax, availH / rows);

    const int contentW = cols * tileW + (cols + 1) * spec.gap;
    const int contentH = rows * tileH + (rows + 1) * spec.gap;
    auto content = bounds.withSizeKeepingCentre(contentW, contentH);
    content.reduce(spec.gap, spec.gap);

    const bool useSpans = spansValid(spec, cols);
    if (!useSpans)
    {
        rects.reserve((size_t)spec.tileCount);
        int index = 0;
        for (int r = 0; r < rows; ++r)
        {
            const int itemsThisRow = (r == rows - 1) ? (spec.tileCount - r * cols) : cols;
            const int rowWidth = itemsThisRow * tileW + (itemsThisRow - 1) * spec.gap;
            int x = content.getX() + (content.getWidth() - rowWidth) / 2;
            int y = content.getY() + r * (tileH + spec.gap);

            for (int c = 0; c < itemsThisRow && index < spec.tileCount; ++c)
            {
                rects.emplace_back(x, y, tileW, tileH);
                x += tileW + spec.gap;
                ++index;
            }
        }
    }
    else
    {
        rects.reserve((size_t)spec.tileCount);
        int index = 0;
        int rowIndex = 0;
        int cursorCols = cols;
        std::vector<int> currentRowSpans;

        auto flushRow = [&](bool force) {
            if (currentRowSpans.empty())
                return;
            const int rowWidth = computeRowWidth(currentRowSpans, tileW, spec.gap);
            int x = content.getX() + (content.getWidth() - rowWidth) / 2;
            int y = content.getY() + rowIndex * (tileH + spec.gap);
            for (size_t i = 0; i < currentRowSpans.size(); ++i)
            {
                const int span = currentRowSpans[i];
                const int w = span * tileW + (span - 1) * spec.gap;
                rects.emplace_back(x, y, w, tileH);
                x += w + spec.gap;
            }
            if (force || !currentRowSpans.empty())
                ++rowIndex;
            currentRowSpans.clear();
            cursorCols = cols;
        };

        while (index < spec.tileCount)
        {
            int span = spec.colSpans[(size_t)index];
            span = juce::jlimit(1, cols, span);

            if (span == cols)
            {
                flushRow(false);
                currentRowSpans.push_back(span);
                flushRow(true);
                ++index;
                continue;
            }

            if (span > cursorCols)
            {
                flushRow(true);
            }

            currentRowSpans.push_back(span);
            cursorCols -= span;
            ++index;

            if (cursorCols <= 0 || index >= spec.tileCount)
                flushRow(true);
        }
    }

    return rects;
}

inline std::vector<juce::Rectangle<int>> layoutCenteredGrid(juce::Rectangle<int> bounds, int tileCount, int gap,
                                                            int tileMin, int tileMax, int preferredColsHint = 0)
{
    GridSpec spec {tileCount, gap, tileMin, tileMax, preferredColsHint, {}};
    return layoutGrid(bounds, spec);
}

} // namespace bws::layout

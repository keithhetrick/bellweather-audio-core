// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_ui/foundation/UiTheme.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace bws::weather::control_surface
{

enum class CompactActionKind
{
    TogglePill,
    ActionPill,
    UtilityAction,
    SplitCompare
};

enum class CompactActionWidth
{
    Narrow,
    Standard
};

struct ControlVisualState
{
    bool enabled = true;
    bool hovered = false;
    bool pressed = false;
    bool selected = false;
    bool focused = false;
};

struct SelectorMetrics
{
    float cornerRadius = 4.0f;
    float borderWidth = 1.0f;
    float dividerInset = 4.0f;
    float padding = 4.0f;
    float focusWidth = 1.0f;
};

struct SelectorPalette
{
    juce::Colour fill;
    juce::Colour border;
    juce::Colour divider;
    juce::Colour text;
    juce::Colour focus;
};

inline SelectorMetrics resolveSelectorMetrics(const bws::ui::UiThemeResolved& theme, float scale)
{
    return {theme.weatherColors.radiusMd * scale, 1.0f, 4.0f * scale, 4.0f * scale, 1.0f};
}

inline SelectorPalette resolveSelectorPalette(const bws::ui::UiThemeResolved& theme, const ControlVisualState& state)
{
    const auto& colors = theme.weatherColors;

    auto fill = colors.bgRaised;
    auto border = colors.borderLight;
    auto divider = colors.borderLight;
    auto text = colors.textSecondary;

    if (!state.enabled)
    {
        fill =
            colors.bgRaised.withMultipliedAlpha(bws::tokens::shared::opacity::weather_control_surface::DISABLED_FILL);
        border = colors.borderLight.withMultipliedAlpha(
            bws::tokens::shared::opacity::weather_control_surface::DISABLED_BORDER);
        divider = colors.borderLight.withMultipliedAlpha(
            bws::tokens::shared::opacity::weather_control_surface::DISABLED_DIVIDER);
        text = colors.textSecondary.withMultipliedAlpha(
            bws::tokens::shared::opacity::weather_control_surface::DISABLED_TEXT);
    }
    else if (state.selected)
    {
        fill = colors.glow;
        border = colors.accent.withMultipliedAlpha(
            state.focused ? bws::tokens::shared::opacity::weather_control_surface::SELECTED_BORDER_FOCUSED
                          : bws::tokens::shared::opacity::weather_control_surface::SELECTED_BORDER);
        divider =
            colors.accent.withMultipliedAlpha(bws::tokens::shared::opacity::weather_control_surface::SELECTED_DIVIDER);
        text = colors.textPrimary;
    }
    else if (state.pressed)
    {
        fill = colors.bgElevated.darker(0.08f);
        border = colors.borderLight.withMultipliedAlpha(bws::tokens::shared::opacity::outline::BOLD);
    }
    else if (state.hovered)
    {
        fill = colors.bgElevated.brighter(0.05f);
        border = colors.borderLight.withMultipliedAlpha(
            bws::tokens::shared::opacity::weather_control_surface::HOVERED_BORDER);
    }

    const float focusAlpha = state.focused ? bws::tokens::shared::opacity::weather_control_surface::FOCUS_RING : 0.0f;
    auto focus = colors.accent.withAlpha(focusAlpha);
    if (!state.enabled)
        focus = focus.withMultipliedAlpha(bws::tokens::shared::opacity::weather_control_surface::DISABLED_FOCUS_MULT);

    return {fill, border, divider, text, focus};
}

inline void paintContainer(juce::Graphics& g, juce::Rectangle<float> bounds, const SelectorMetrics& metrics,
                           const SelectorPalette& palette)
{
    g.setColour(palette.fill);
    g.fillRoundedRectangle(bounds, metrics.cornerRadius);

    g.setColour(palette.border);
    g.drawRoundedRectangle(bounds.reduced(bws::tokens::shared::geometry::STROKE_HALF_PX), metrics.cornerRadius,
                           metrics.borderWidth);
}

inline void paintFocusRing(juce::Graphics& g, juce::Rectangle<float> bounds, const SelectorMetrics& metrics,
                           const SelectorPalette& palette)
{
    if (palette.focus.getAlpha() <= 0)
        return;

    g.setColour(palette.focus);
    g.drawRoundedRectangle(bounds.reduced(bws::tokens::shared::geometry::STROKE_ONE_HALF_PX),
                           juce::jmax(1.0f, metrics.cornerRadius - 1.0f), metrics.focusWidth);
}

inline juce::Path makeJoinedRoundedPath(const juce::Rectangle<float>& bounds, float cornerRadius, bool roundLeft,
                                        bool roundRight)
{
    juce::Path path;
    path.addRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), cornerRadius,
                             cornerRadius, roundLeft, roundRight, roundLeft, roundRight);
    return path;
}

struct CompactActionMetrics
{
    float cornerRadius = 6.0f;
    float borderWidth = 1.0f;
    float focusWidth = 1.0f;
    float horizontalPadding = 10.0f;
    float dividerInset = 3.0f;
};

struct CompactActionPalette
{
    juce::Colour fill;
    juce::Colour border;
    juce::Colour text;
    juce::Colour focus;
    juce::Colour divider;
};

inline CompactActionMetrics resolveCompactActionMetrics(const bws::ui::UiThemeResolved& theme, float scale,
                                                        CompactActionWidth widthKind = CompactActionWidth::Standard)
{
    const float basePadding = (widthKind == CompactActionWidth::Narrow) ? 7.0f : 10.0f;
    return {juce::jmax(5.0f, theme.weatherColors.radiusMd * scale), 1.0f, 1.0f, basePadding * scale, 3.0f * scale};
}

inline CompactActionPalette resolveCompactActionPalette(const bws::ui::UiThemeResolved& theme,
                                                        const ControlVisualState& state, CompactActionKind kind)
{
    const auto& colors = theme.weatherColors;

    juce::Colour fill;
    juce::Colour border;
    juce::Colour text;

    switch (kind)
    {
    case CompactActionKind::TogglePill:
        if (state.selected)
        {
            fill = colors.bgRaised.brighter(state.hovered ? 0.06f : 0.04f);
            border = colors.accent.withAlpha(
                state.focused
                    ? bws::tokens::shared::opacity::weather_control_surface::TOGGLE_PILL_SELECTED_BORDER_FOCUSED
                    : bws::tokens::shared::opacity::weather_control_surface::TOGGLE_PILL_SELECTED_BORDER);
            text = colors.textPrimary;
        }
        else
        {
            fill = state.pressed
                       ? colors.bgRaised.withMultipliedAlpha(
                             bws::tokens::shared::opacity::weather_control_surface::TOGGLE_PILL_PRESSED_FILL)
                       : (state.hovered
                              ? colors.bgRaised.brighter(0.025f)
                              : colors.bgRaised.withMultipliedAlpha(
                                    bws::tokens::shared::opacity::weather_control_surface::TOGGLE_PILL_IDLE_FILL));
            border = colors.borderLight.withMultipliedAlpha(
                state.hovered ? bws::tokens::shared::opacity::weather_control_surface::TOGGLE_PILL_HOVERED_BORDER
                              : bws::tokens::shared::opacity::weather_control_surface::TOGGLE_PILL_IDLE_BORDER);
            text = colors.textSecondary.withMultipliedAlpha(
                bws::tokens::shared::opacity::weather_control_surface::TOGGLE_PILL_IDLE_TEXT);
        }
        break;

    case CompactActionKind::ActionPill:
        fill =
            state.pressed
                ? colors.bgRaised.withMultipliedAlpha(
                      bws::tokens::shared::opacity::weather_control_surface::ACTION_PILL_PRESSED_FILL)
                : (state.hovered ? colors.bgRaised.brighter(0.03f)
                                 : colors.bgRaised.withMultipliedAlpha(
                                       bws::tokens::shared::opacity::weather_control_surface::ACTION_PILL_IDLE_FILL));
        border = colors.borderLight.withMultipliedAlpha(
            state.hovered ? bws::tokens::shared::opacity::weather_control_surface::ACTION_PILL_HOVERED_BORDER
                          : bws::tokens::shared::opacity::weather_control_surface::ACTION_PILL_IDLE_BORDER);
        text = colors.textSecondary.withMultipliedAlpha(
            bws::tokens::shared::opacity::weather_control_surface::ACTION_PILL_TEXT);
        break;

    case CompactActionKind::UtilityAction:
        fill = state.pressed
                   ? colors.bgRaised.withMultipliedAlpha(
                         bws::tokens::shared::opacity::weather_control_surface::UTILITY_PRESSED_FILL)
                   : (state.hovered ? colors.bgRaised.withMultipliedAlpha(
                                          bws::tokens::shared::opacity::weather_control_surface::UTILITY_HOVERED_FILL)
                                    : juce::Colours::transparentBlack);
        {
            const float utilBorderAlpha =
                state.hovered ? bws::tokens::shared::opacity::weather_control_surface::UTILITY_HOVERED_BORDER : 0.0f;
            border = colors.borderLight.withMultipliedAlpha(utilBorderAlpha);
        }
        text = colors.textSecondary.withMultipliedAlpha(
            state.hovered ? bws::tokens::shared::opacity::weather_control_surface::UTILITY_HOVERED_TEXT
                          : bws::tokens::shared::opacity::weather_control_surface::UTILITY_IDLE_TEXT);
        break;

    case CompactActionKind::SplitCompare:
        fill = state.pressed
                   ? colors.bgRaised.withMultipliedAlpha(
                         bws::tokens::shared::opacity::weather_control_surface::SPLIT_COMPARE_PRESSED_FILL)
                   : (state.hovered
                          ? colors.bgRaised.withMultipliedAlpha(
                                bws::tokens::shared::opacity::weather_control_surface::SPLIT_COMPARE_HOVERED_FILL)
                          : juce::Colours::transparentBlack);
        border = colors.borderLight.withMultipliedAlpha(
            state.hovered ? bws::tokens::shared::opacity::weather_control_surface::SPLIT_COMPARE_HOVERED_BORDER
                          : bws::tokens::shared::opacity::weather_control_surface::SPLIT_COMPARE_IDLE_BORDER);
        text = colors.textSecondary.withMultipliedAlpha(
            state.hovered ? bws::tokens::shared::opacity::weather_control_surface::SPLIT_COMPARE_HOVERED_TEXT
                          : bws::tokens::shared::opacity::weather_control_surface::SPLIT_COMPARE_IDLE_TEXT);
        break;
    }

    if (!state.enabled)
    {
        fill = fill.withMultipliedAlpha(bws::tokens::shared::opacity::weather_control_surface::COMPACT_DISABLED_FILL);
        border = border.withMultipliedAlpha(bws::tokens::shared::opacity::weather_control_surface::DISABLED_DIVIDER);
        text = text.withMultipliedAlpha(bws::tokens::shared::opacity::weather_control_surface::DISABLED_TEXT);
    }

    const float focusAlpha =
        state.focused ? bws::tokens::shared::opacity::weather_control_surface::COMPACT_FOCUS_RING : 0.0f;
    auto focus = colors.accent.withAlpha(focusAlpha);
    if (!state.enabled)
        focus = focus.withMultipliedAlpha(bws::tokens::shared::opacity::weather_control_surface::DISABLED_FOCUS_MULT);

    auto divider = border.withMultipliedAlpha(bws::tokens::shared::opacity::outline::EMPHASIZED);
    return {fill, border, text, focus, divider};
}

inline void paintCompactAction(juce::Graphics& g, juce::Rectangle<float> bounds, const CompactActionMetrics& metrics,
                               const CompactActionPalette& palette)
{
    g.setColour(palette.fill);
    g.fillRoundedRectangle(bounds, metrics.cornerRadius);

    if (palette.border.getAlpha() > 0)
    {
        g.setColour(palette.border);
        g.drawRoundedRectangle(bounds.reduced(bws::tokens::shared::geometry::STROKE_HALF_PX), metrics.cornerRadius,
                               metrics.borderWidth);
    }
}

inline void paintCompactFocusRing(juce::Graphics& g, juce::Rectangle<float> bounds, const CompactActionMetrics& metrics,
                                  const CompactActionPalette& palette)
{
    if (palette.focus.getAlpha() <= 0)
        return;

    g.setColour(palette.focus);
    g.drawRoundedRectangle(bounds.reduced(bws::tokens::shared::geometry::STROKE_ONE_HALF_PX),
                           juce::jmax(1.0f, metrics.cornerRadius - 1.0f), metrics.focusWidth);
}

} // namespace bws::weather::control_surface

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/Components/UiMeter.h"
#include "bw_ui/generated/BwTokens.h"
#include <algorithm>

namespace bws::ui
{

UiMeter::UiMeter(const UiThemeResolved& themeIn, Orientation orientationIn)
    : theme(themeIn)
    , orientation(orientationIn)
{
    setInterceptsMouseClicks(false, false);
    setRepaintsOnMouseActivity(false);
}

void UiMeter::setTheme(const UiThemeResolved& newTheme)
{
    theme = newTheme;
    repaint();
}

void UiMeter::setScale(float newScale)
{
    scale = newScale;
    repaint();
}

void UiMeter::setOrientation(Orientation o)
{
    if (orientation == o)
        return;
    orientation = o;
    repaint();
}

void UiMeter::setValue(float normalized01)
{
    const float clamped = juce::jlimit(0.0f, 1.0f, normalized01);
    if (juce::approximatelyEqual(clamped, value))
        return;
    value = clamped;
    repaint();
}

void UiMeter::setSecondaryValue(float normalized01)
{
    const float clamped = juce::jlimit(0.0f, 1.0f, normalized01);
    if (juce::approximatelyEqual(clamped, secondary))
        return;
    secondary = clamped;
    repaint();
}

void UiMeter::setDisabled(bool shouldDisable)
{
    if (disabled == shouldDisable)
        return;
    disabled = shouldDisable;
    repaint();
}

float UiMeter::padding() const
{
    return theme.spacing.xs * scale;
}

float UiMeter::radius() const
{
    return theme.surfaces.radiusSm * scale;
}

juce::Rectangle<float> UiMeter::trackBounds() const
{
    auto bounds = getLocalBounds().toFloat();
    const float pad = padding();
    bounds = bounds.reduced(pad);
    return bounds;
}

juce::Rectangle<float> UiMeter::levelBounds(const juce::Rectangle<float>& track, float v) const
{
    if (v <= 0.0f)
        return {};

    v = juce::jlimit(0.0f, 1.0f, v);
    if (orientation == Orientation::Horizontal)
    {
        const float w = track.getWidth() * v;
        return {track.getX(), track.getY(), w, track.getHeight()};
    }
    else
    {
        const float h = track.getHeight() * v;
        return {track.getX(), track.getBottom() - h, track.getWidth(), h};
    }
}

void UiMeter::paint(juce::Graphics& g)
{
    auto track = trackBounds();

    namespace opm = bws::tokens::shared::opacity::ui_meter;
    g.setColour(disabled ? theme.colors.outline0.withAlpha(opm::TRACK_DISABLED) : theme.colors.bg1);
    g.fillRoundedRectangle(track, radius());

    g.setColour(theme.colors.outline0.withAlpha(disabled ? opm::TRACK_OUTLINE_DISABLED : opm::TRACK_OUTLINE_DEFAULT));
    g.drawRoundedRectangle(track, radius(), theme.surfaces.strokeThin);

    const auto levelRect = levelBounds(track, value);
    if (!levelRect.isEmpty())
    {
        g.setColour(disabled ? theme.colors.textDisabled : theme.colors.accent0);
        g.fillRoundedRectangle(levelRect, radius());
    }

    if (secondary >= 0.0f)
    {
        const auto secondaryRect = levelBounds(track, secondary);
        if (!secondaryRect.isEmpty())
        {
            namespace opm = bws::tokens::shared::opacity::ui_meter;
            g.setColour(disabled ? theme.colors.textDisabled.withAlpha(opm::ACCENT_DISABLED)
                                 : theme.colors.accent1.withAlpha(opm::ACCENT_SECONDARY));
            g.fillRoundedRectangle(secondaryRect, radius());
        }
    }
}

} // namespace bws::ui

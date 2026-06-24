// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
//
// Popup draw code reads ALL proportional ratios (row height, tick glyph, etc.)
// from UiDropdowns::metrics(role, scale, theme). Do not introduce literal
// factors (e.g. `font.getHeight() * 0.6f`) inline; add a named field to
// UiDropdownMetrics instead. Color-state alphas live next to the drawing code
// that consumes them.

#include "bw_ui/Components/ThemedPopupMenuLookAndFeel.h"
#include "bw_ui/generated/BwTokens.h"

#include <algorithm>

namespace bws::ui
{

namespace
{
void syncPopupMenuHostColours(juce::LookAndFeel_V4& lookAndFeel, const UiThemeResolved& theme)
{
    // Keep the JUCE popup host/window transparent so only the themed shell
    // is visible. JUCE uses PopupMenu::backgroundColourId to decide whether
    // the host component is opaque; if left opaque it fills the whole popup
    // window before our rounded shell is drawn, which creates the visible
    // outer rectangle behind the corners.
    lookAndFeel.setColour(juce::PopupMenu::backgroundColourId, juce::Colours::transparentBlack);
    lookAndFeel.setColour(juce::PopupMenu::textColourId, theme.colors.text0);
    lookAndFeel.setColour(juce::PopupMenu::highlightedBackgroundColourId, theme.colors.bg2);
    lookAndFeel.setColour(juce::PopupMenu::highlightedTextColourId, theme.colors.text0);
}
} // namespace

//==============================================================================
// dropdown_internal - pure compute helpers

namespace dropdown_internal
{
float tickGlyphRadius(float fontHeight, float tickGlyphFactor, float minTickGlyphRadius) noexcept
{
    return std::max(minTickGlyphRadius, fontHeight * tickGlyphFactor);
}

int idealPopupItemWidth(const juce::String& text, float fontHeight, float scale, const UiThemeResolved& theme,
                        UiDropdownRole role) noexcept
{
    const auto metrics = UiDropdowns::metrics(role, scale, theme);
    // JUCE deprecated Font::getStringWidthFloat; static GlyphArrangement helper is the replacement.
    const float textW = juce::GlyphArrangement::getStringWidth(juce::Font(juce::FontOptions(fontHeight)), text);
    // Tick reserve scales with the tick glyph (which scales with font height).
    // Formula: 2 × padding (both sides of the glyph) + 2 × radius (glyph diameter).
    const float tickReserve = metrics.padding * 2.0f +
                              tickGlyphRadius(fontHeight, metrics.tickGlyphFactor, metrics.minTickGlyphRadius) * 2.0f;
    const float total = metrics.padding + textW + metrics.padding + tickReserve;
    return static_cast<int>(std::lround(std::max(total, metrics.minPopupWidth)));
}

juce::Colour popupItemBackground(bool isHighlighted, bool isActive, const UiThemeResolved& theme) noexcept
{
    if (!isHighlighted || !isActive)
        return juce::Colours::transparentBlack;
    return theme.colors.bg2;
}

juce::Colour popupItemTextColour(bool isActive, bool /*isTicked*/, const UiThemeResolved& theme) noexcept
{
    return isActive ? theme.colors.text0 : theme.colors.textDisabled;
}
} // namespace dropdown_internal

namespace popup_menu
{
namespace
{
juce::PopupMenu::Options applySharedPopupOptions(juce::PopupMenu::Options options, juce::Component* parentComponent,
                                                 const UiThemeResolved& theme, float scale, UiDropdownRole role,
                                                 int requestedMinimumWidth, juce::Component* deletionCheckComponent)
{
    if (parentComponent != nullptr)
        options = options.withParentComponent(parentComponent);

    options = options.withMinimumWidth(resolvedMinimumWidth(theme, role, scale, requestedMinimumWidth));

    if (deletionCheckComponent != nullptr)
        options = options.withDeletionCheck(*deletionCheckComponent);

    return options;
}
} // namespace

int resolvedMinimumWidth(const UiThemeResolved& theme, UiDropdownRole role, float scale,
                         int requestedMinimumWidth) noexcept
{
    const auto metrics = UiDropdowns::metrics(role, scale, theme);
    const int roleFloor = static_cast<int>(std::lround(metrics.minPopupWidth));
    return std::max(roleFloor, requestedMinimumWidth);
}

juce::PopupMenu::Options makePopupMenuOptionsForComponent(juce::Component& targetComponent,
                                                          juce::Component* parentComponent,
                                                          const UiThemeResolved& theme, float scale,
                                                          UiDropdownRole role, int requestedMinimumWidth,
                                                          juce::Component* deletionCheckComponent)
{
    auto options = juce::PopupMenu::Options().withTargetComponent(targetComponent);
    return applySharedPopupOptions(options, parentComponent, theme, scale, role, requestedMinimumWidth,
                                   deletionCheckComponent);
}

juce::PopupMenu::Options makePopupMenuOptionsForScreenArea(juce::Rectangle<int> targetScreenArea,
                                                           juce::Component* parentComponent,
                                                           const UiThemeResolved& theme, float scale,
                                                           UiDropdownRole role, int requestedMinimumWidth,
                                                           juce::Component* deletionCheckComponent)
{
    auto options = juce::PopupMenu::Options().withTargetScreenArea(targetScreenArea);
    return applySharedPopupOptions(options, parentComponent, theme, scale, role, requestedMinimumWidth,
                                   deletionCheckComponent);
}
} // namespace popup_menu

//==============================================================================
// ThemedPopupMenuLookAndFeel

ThemedPopupMenuLookAndFeel::ThemedPopupMenuLookAndFeel(const UiThemeResolved& theme, UiDropdownRole role)
    : theme_(theme)
    , kernelTheme_(adapters::makeKernelThemeSnapshot(theme))
    , role_(role)
{
    syncPopupMenuHostColours(*this, theme_);
}

void ThemedPopupMenuLookAndFeel::setTheme(const UiThemeResolved& newTheme)
{
    theme_ = newTheme;
    kernelTheme_ = adapters::makeKernelThemeSnapshot(newTheme);
    syncPopupMenuHostColours(*this, theme_);
}

juce::Font ThemedPopupMenuLookAndFeel::getPopupMenuFont()
{
    // Role-aware: Overlay maps to Annotation (micro-chrome); the other popup
    // families keep ControlText and differentiate through role metrics,
    // including a dedicated popupFontScale token that keeps utility/menu text
    // quieter than surrounding primary UI chrome.
    const auto metrics = UiDropdowns::metrics(role_, scale_, theme_);
    const auto textRole =
        (role_ == UiDropdownRole::Overlay) ? kernel::TextRole::Annotation : kernel::TextRole::ControlText;
    return adapters::makeFont(kernelTheme_, textRole, scale_ * metrics.popupFontScale);
}

int ThemedPopupMenuLookAndFeel::getPopupMenuBorderSize()
{
    return 0;
}

int ThemedPopupMenuLookAndFeel::getPopupMenuBorderSizeWithOptions(const juce::PopupMenu::Options& /*options*/)
{
    return 0;
}

void ThemedPopupMenuLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    const auto metrics = UiDropdowns::metrics(role_, scale_, theme_);
    auto bounds = juce::Rectangle<float>(0.0f, 0.0f, (float)width, (float)height)
                      .reduced(bws::tokens::shared::geometry::STROKE_HALF_PX);
    juce::Path shellPath;
    shellPath.addRoundedRectangle(bounds, metrics.cornerRadius);

    // Draw one shared shell only. Letting JUCE paint its stock popup first
    // creates the "double corner" seam where the outer square-ish popup shows
    // through behind our rounded panel.
    juce::DropShadow(theme_.colors.bg0.withAlpha(metrics.shadowAlpha),
                     static_cast<int>(std::lround(metrics.shadowBlur)),
                     {0, static_cast<int>(std::lround(metrics.shadowOffsetY))})
        .drawForPath(g, shellPath);

    g.setColour(theme_.colors.bg1);
    g.fillPath(shellPath);

    g.setColour(theme_.colors.outline0.withAlpha(bws::tokens::shared::opacity::outline::DIVIDER));
    g.strokePath(shellPath, juce::PathStrokeType(metrics.outlineThickness));
}

void ThemedPopupMenuLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                                   bool isSeparator, bool isActive, bool isHighlighted, bool isTicked,
                                                   bool /*hasSubMenu*/, const juce::String& text,
                                                   const juce::String& /*shortcutKeyText*/,
                                                   const juce::Drawable* /*icon*/,
                                                   const juce::Colour* /*textColourToUse*/)
{
    if (isSeparator)
    {
        g.setColour(theme_.colors.outline0.withAlpha(bws::tokens::shared::opacity::outline::DIVIDER));
        const int insetX = static_cast<int>(std::lround(UiDropdowns::metrics(role_, scale_, theme_).separatorInsetX));
        g.fillRect(area.reduced(insetX, 0).withHeight(1).withY(area.getCentreY()));
        return;
    }

    // Hover wash - transparent when not highlighted, elevated bg when it is.
    const auto bgColour = dropdown_internal::popupItemBackground(isHighlighted, isActive, theme_);
    if (bgColour.getAlpha() > 0)
    {
        g.setColour(bgColour);
        g.fillRect(area);
    }

    // Single font source: getPopupMenuFont() → adapters::makeFont(kernelTheme,
    // TextRole, scale). Items, section headers, ideal-size calc, tick-glyph
    // sizing - all flow from this one call. No area-based rescaling.
    auto font = getPopupMenuFont();
    g.setFont(font);

    g.setColour(dropdown_internal::popupItemTextColour(isActive, isTicked, theme_));

    const auto metrics = UiDropdowns::metrics(role_, scale_, theme_);
    const int leftPad = static_cast<int>(std::lround(metrics.padding));
    const float tickRadius =
        dropdown_internal::tickGlyphRadius(font.getHeight(), metrics.tickGlyphFactor, metrics.minTickGlyphRadius);
    const int rightReserve = static_cast<int>(std::lround(metrics.padding * 2.0f + tickRadius * 2.0f));
    g.drawText(text, area.reduced(leftPad, 0).withTrimmedRight(rightReserve), juce::Justification::centredLeft, true);

    if (isTicked && area.getWidth() > rightReserve)
    {
        // Small filled circle on the right - uses theme.colors.ok (semantic stable).
        const float cx = static_cast<float>(area.getRight()) - metrics.padding - tickRadius;
        const float cy = static_cast<float>(area.getCentreY());
        g.setColour(theme_.colors.ok);
        g.fillEllipse(cx - tickRadius, cy - tickRadius, tickRadius * 2.0f, tickRadius * 2.0f);
    }
}

void ThemedPopupMenuLookAndFeel::getIdealPopupMenuItemSize(const juce::String& text, bool isSeparator,
                                                           int standardMenuItemHeight, int& idealWidth,
                                                           int& idealHeight)
{
    if (isSeparator)
    {
        idealWidth = standardMenuItemHeight * 2;
        idealHeight = standardMenuItemHeight > 0 ? standardMenuItemHeight / 10 + 2 : 6;
        return;
    }

    // Row height derives from the theme font - not JUCE's standardMenuItemHeight.
    // Letting the host drive row height would make item draw and row layout
    // disagree (the core of the "cartoon section headers" bug).
    juce::ignoreUnused(standardMenuItemHeight);
    const auto font = getPopupMenuFont();
    const auto metrics = UiDropdowns::metrics(role_, scale_, theme_);
    idealWidth = dropdown_internal::idealPopupItemWidth(text, font.getHeight(), scale_, theme_, role_);
    idealHeight = static_cast<int>(std::lround(font.getHeight() * metrics.rowHeightFactor));
}

void ThemedPopupMenuLookAndFeel::drawPopupMenuSectionHeader(juce::Graphics& g, const juce::Rectangle<int>& area,
                                                            const juce::String& sectionName)
{
    // Match the base popup font size (same as items land on) - bold weight alone
    // carries the hierarchy. Scaling off area.getHeight() here is a trap: JUCE
    // reserves a taller row for section headers than for items, so any
    // proportional scale overshoots the item text by ~40%.
    auto font = getPopupMenuFont().boldened();
    g.setFont(font);

    g.setColour(theme_.colors.text0.withAlpha(bws::tokens::shared::opacity::text::SECTION_HEADER));
    const auto metrics = UiDropdowns::metrics(role_, scale_, theme_);
    const int leftPad = static_cast<int>(std::lround(metrics.padding));
    g.drawText(sectionName, area.reduced(leftPad, 0), juce::Justification::centredLeft, true);

    // Thin divider hairline at the bottom of the section header.
    g.setColour(theme_.colors.outline0.withAlpha(bws::tokens::shared::opacity::outline::HAIRLINE));
    g.fillRect(area.withTop(area.getBottom() - 1).reduced(leftPad, 0).withHeight(1));
}

void ThemedPopupMenuLookAndFeel::drawScrollbar(juce::Graphics& g, juce::ScrollBar& /*bar*/, int x, int y, int width,
                                               int height, bool isScrollbarVertical, int thumbStartPosition,
                                               int thumbSize, bool /*isMouseOver*/, bool /*isMouseDown*/)
{
    if (width <= 0 || height <= 0)
        return;

    // Track: subtle outline-tinted bg
    g.setColour(theme_.colors.outline0.withAlpha(bws::tokens::shared::opacity::scrollbar::TRACK));
    g.fillRect(x, y, width, height);

    if (thumbSize > 0)
    {
        // Thumb: accent-tinted, rounded
        g.setColour(theme_.colors.accent0.withAlpha(bws::tokens::shared::opacity::scrollbar::THUMB));
        if (isScrollbarVertical)
            g.fillRoundedRectangle(static_cast<float>(x + 1), static_cast<float>(thumbStartPosition),
                                   static_cast<float>(std::max(0, width - 2)), static_cast<float>(thumbSize), 2.0f);
        else
            g.fillRoundedRectangle(static_cast<float>(thumbStartPosition), static_cast<float>(y + 1),
                                   static_cast<float>(thumbSize), static_cast<float>(std::max(0, height - 2)), 2.0f);
    }
}

} // namespace bws::ui

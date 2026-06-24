// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "bw_ui/adapters/UiThemeKernelAdapter.h"
#include "bw_ui/foundation/UiTheme.h"

namespace bws::ui
{

/**
 * Standalone themed popup-menu LookAndFeel.
 *
 * Owns the popup drawing logic previously trapped inside
 * UiDropdown::DropdownLookAndFeel. Any caller that wants themed popups
 * (direct juce::PopupMenu users, plugin-specific LnFs that need popup
 * theming, widget-internal LnFs) can inherit from or instantiate this
 * class.
 *
 * Holds UiThemeResolved + kernel::ThemeSnapshot by value. Standalone
 * ownership - no caller-lifetime coupling. Optional role affects font
 * selection (Overlay = Annotation, the other popup families = ControlText)
 * and popup metrics/density (via UiDropdownRole).
 */
class ThemedPopupMenuLookAndFeel : public juce::LookAndFeel_V4
{
public:
    explicit ThemedPopupMenuLookAndFeel(const UiThemeResolved& theme, UiDropdownRole role = UiDropdownRole::Standard);
    ~ThemedPopupMenuLookAndFeel() override = default;

    void setTheme(const UiThemeResolved& newTheme);
    void setRole(UiDropdownRole newRole) noexcept { role_ = newRole; }
    void setScale(float newScale) noexcept { scale_ = newScale; }

    // juce::LookAndFeel_V4 overrides (popup-menu drawing):
    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;
    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area, bool isSeparator, bool isActive,
                           bool isHighlighted, bool isTicked, bool hasSubMenu, const juce::String& text,
                           const juce::String& shortcutKeyText, const juce::Drawable* icon,
                           const juce::Colour* textColourToUse) override;
    void getIdealPopupMenuItemSize(const juce::String& text, bool isSeparator, int standardMenuItemHeight,
                                   int& idealWidth, int& idealHeight) override;
    juce::Font getPopupMenuFont() override;
    int getPopupMenuBorderSize() override;
    int getPopupMenuBorderSizeWithOptions(const juce::PopupMenu::Options& options) override;
    void drawPopupMenuSectionHeader(juce::Graphics& g, const juce::Rectangle<int>& area,
                                    const juce::String& sectionName) override;
    void drawScrollbar(juce::Graphics& g, juce::ScrollBar& bar, int x, int y, int width, int height,
                       bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver,
                       bool isMouseDown) override;

protected:
    const UiThemeResolved& getTheme() const noexcept { return theme_; }
    const kernel::ThemeSnapshot& getKernelTheme() const noexcept { return kernelTheme_; }
    UiDropdownRole getRole() const noexcept { return role_; }
    float getScale() const noexcept { return scale_; }

private:
    UiThemeResolved theme_;
    kernel::ThemeSnapshot kernelTheme_;
    UiDropdownRole role_ {UiDropdownRole::Standard};
    float scale_ {1.0f};
};

// Popup-styling pure helpers. Exposed for unit tests (see
// modules/bw_ui/tests/ui_dropdown_popup_test.cpp). Production code reaches
// these via ThemedPopupMenuLookAndFeel's virtual overrides, not directly.
namespace dropdown_internal
{
/** Scale-aware tick glyph radius. Linear in font height × factor, with a
 *  hard minimum readability floor. Factor is sourced from
 *  UiDropdownMetrics::tickGlyphFactor in production. */
float tickGlyphRadius(float fontHeight, float tickGlyphFactor) noexcept;

/** Reserves text width + left/right padding + tick-glyph right-reserve.
 *  Matches the layout drawPopupMenuItem uses. */
int idealPopupItemWidth(const juce::String& text, float fontHeight, float scale, const UiThemeResolved& theme,
                        UiDropdownRole role) noexcept;

/** Palette-driven hover/normal background. Returns transparent when not
 *  highlighted (caller layers it over the popup-background fill). */
juce::Colour popupItemBackground(bool isHighlighted, bool isActive, const UiThemeResolved& theme) noexcept;

/** Text color picker for active / disabled state. Ticked items keep the
 *  active color and rely on the tick glyph to signal selection. */
juce::Colour popupItemTextColour(bool isActive, bool isTicked, const UiThemeResolved& theme) noexcept;
} // namespace dropdown_internal

namespace popup_menu
{
int resolvedMinimumWidth(const UiThemeResolved& theme, UiDropdownRole role, float scale,
                         int requestedMinimumWidth = 0) noexcept;

juce::PopupMenu::Options makePopupMenuOptionsForComponent(juce::Component& targetComponent,
                                                          juce::Component* parentComponent,
                                                          const UiThemeResolved& theme, float scale,
                                                          UiDropdownRole role, int requestedMinimumWidth = 0,
                                                          juce::Component* deletionCheckComponent = nullptr);

juce::PopupMenu::Options makePopupMenuOptionsForScreenArea(juce::Rectangle<int> targetScreenArea,
                                                           juce::Component* parentComponent,
                                                           const UiThemeResolved& theme, float scale,
                                                           UiDropdownRole role, int requestedMinimumWidth = 0,
                                                           juce::Component* deletionCheckComponent = nullptr);
} // namespace popup_menu

} // namespace bws::ui

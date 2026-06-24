// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "bw_ui/foundation/Fonts.h"
#include "bw_ui/foundation/UiTheme.h"
#include "bw_ui/kernel/Theme.h"
#include "bw_ui/rendering/IRenderer.h"

namespace bws::ui::adapters
{
kernel::FontWeight toKernelFontWeight(bws::ui::FontWeight weight) noexcept;
kernel::TextRole toKernelTextRole(UiTypographyRole role) noexcept;
kernel::Density toKernelDensity(UiDensity density) noexcept;

kernel::ThemeSnapshot makeKernelThemeSnapshot(
    const UiThemeResolved& theme, kernel::ProductThemeId productTheme = kernel::ProductThemeId::Shared) noexcept;

rendering::FontWeight toRendererFontWeight(kernel::FontWeight weight) noexcept;

float scaledFontSize(const kernel::ThemeSnapshot& theme, kernel::TextRole role, float scale) noexcept;

juce::Font makeFont(const kernel::ThemeSnapshot& theme, kernel::TextRole role, float scale);

// A role's font scaled so its height matches targetHeight (in px).
juce::Font fontFittedToHeight(const kernel::ThemeSnapshot& theme, kernel::TextRole role, float targetHeight);

void applyGraphicsFont(juce::Graphics& graphics, const kernel::ThemeSnapshot& theme, kernel::TextRole role,
                       float scale);

void applyRendererFont(rendering::IRenderer& renderer, const kernel::ThemeSnapshot& theme, kernel::TextRole role,
                       float scale);

juce::String applyTextCasing(const kernel::ThemeSnapshot& theme, kernel::TextRole role, const juce::String& text);

} // namespace bws::ui::adapters

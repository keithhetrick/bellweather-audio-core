// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "bw_ui/adapters/UiThemeKernelAdapter.h"

namespace bws::ui::adapters
{

inline int sectionVerticalPadding(const kernel::ThemeSnapshot& theme, bool dense, float scale)
{
    const float pad = dense ? theme.spacing.xs : theme.spacing.sm;
    return (int)std::round(pad * scale);
}

inline int sectionHorizontalPadding(const kernel::ThemeSnapshot& theme, float scale)
{
    return (int)std::round(theme.spacing.sm * scale);
}

inline int sectionGap(const kernel::ThemeSnapshot& theme, float scale)
{
    return (int)std::round(theme.spacing.xs * scale);
}

inline int sectionTextHeight(const kernel::ThemeSnapshot& theme, kernel::TextRole role, float scale)
{
    return (int)std::round(scaledFontSize(theme, role, scale));
}

inline int sectionPreferredHeight(const kernel::ThemeSnapshot& theme, bool dense, float scale, bool hasSubtitle,
                                  kernel::TextRole titleRole = kernel::TextRole::Title,
                                  kernel::TextRole subtitleRole = kernel::TextRole::ControlText)
{
    int height = sectionVerticalPadding(theme, dense, scale) * 2;
    height += sectionTextHeight(theme, titleRole, scale);
    if (hasSubtitle)
    {
        height += sectionGap(theme, scale);
        height += sectionTextHeight(theme, subtitleRole, scale);
    }
    return height;
}

inline int panelPadding(const kernel::ThemeSnapshot& theme, bool dense, float scale)
{
    return (int)std::round((dense ? theme.spacing.sm : theme.spacing.md) * scale);
}

inline int panelTitleGap(const kernel::ThemeSnapshot& theme, float scale)
{
    return sectionGap(theme, scale);
}

} // namespace bws::ui::adapters

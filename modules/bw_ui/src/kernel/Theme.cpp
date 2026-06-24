// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/kernel/Theme.h"

namespace bws::ui::kernel
{
std::size_t toIndex(TextRole role) noexcept
{
    switch (role)
    {
    case TextRole::BrandSmall:
        return 0;
    case TextRole::Title:
        return 1;
    case TextRole::SectionLabel:
        return 2;
    case TextRole::ControlText:
        return 3;
    case TextRole::Readout:
        return 4;
    case TextRole::Tab:
        return 5;
    case TextRole::Overlay:
        return 6;
    case TextRole::HeaderBrand:
        return 7;
    case TextRole::HeaderPreset:
        return 8;
    case TextRole::Annotation:
        return 9;
    }

    return 0;
}

const TextStyle& textStyle(const ThemeSnapshot& theme, TextRole role) noexcept
{
    return theme.textStyles[toIndex(role)];
}

} // namespace bws::ui::kernel

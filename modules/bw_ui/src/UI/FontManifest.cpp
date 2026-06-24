// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/foundation/FontManifest.h"
#include "FontBinaryData.h"

namespace bw
{
FontBlob getFontBlob(FontId id)
{
    switch (id)
    {
    case FontId::MonoRegular:
        return {FontBinaryData::JetBrainsMonoNLRegular_ttf, (size_t)FontBinaryData::JetBrainsMonoNLRegular_ttfSize};
    case FontId::MonoMedium:
        return {FontBinaryData::JetBrainsMonoNLMedium_ttf, (size_t)FontBinaryData::JetBrainsMonoNLMedium_ttfSize};
    case FontId::MonoItalic:
        return {}; // Locale uses the platform fallback font.

    case FontId::UiRegular:
        return {FontBinaryData::JetBrainsMonoNLRegular_ttf, (size_t)FontBinaryData::JetBrainsMonoNLRegular_ttfSize};
    case FontId::UiMedium:
    case FontId::UiBold:
    case FontId::UiSemibold:
        return {FontBinaryData::JetBrainsMonoNLMedium_ttf, (size_t)FontBinaryData::JetBrainsMonoNLMedium_ttfSize};
    case FontId::UiInterRegular:
        return {FontBinaryData::InterRegular_ttf, (size_t)FontBinaryData::InterRegular_ttfSize};
    case FontId::UiInterSemibold:
        return {FontBinaryData::InterSemiBold_ttf, (size_t)FontBinaryData::InterSemiBold_ttfSize};

    default:
        return {};
    }
}

FontId defaultFontForRole(FontRole role)
{
    switch (role)
    {
    case FontRole::Mono:
        return FontId::MonoRegular;
    case FontRole::Title:
        return FontId::UiInterSemibold;
    case FontRole::Caption:
        return FontId::UiInterRegular;
    case FontRole::Body:
    default:
        return FontId::UiInterRegular;
    }
}
} // namespace bw

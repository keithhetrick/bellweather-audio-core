// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>

namespace bw
{
enum class FontRole : uint8_t
{
    Body,
    Caption,
    Title,
    Mono
};

enum class FontId : uint8_t
{
    UiRegular,
    UiMedium,
    UiBold,
    UiSemibold,
    UiInterRegular,
    UiInterSemibold,

    MonoRegular,
    MonoMedium,
    MonoItalic,

    Count
};

struct FontBlob
{
    const void* data = nullptr;
    size_t size = 0;
};

FontBlob getFontBlob(FontId id);

FontId defaultFontForRole(FontRole role);
} // namespace bw

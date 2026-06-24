// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include "FontManifest.h"

namespace bw::Fonts
{
enum class FontFamily
{
    JetBrainsMono,
    Inter
};

struct Config
{
    bw::FontId body = bw::defaultFontForRole(bw::FontRole::Body);
    bw::FontId caption = bw::defaultFontForRole(bw::FontRole::Caption);
    bw::FontId title = bw::defaultFontForRole(bw::FontRole::Title);
    bw::FontId mono = bw::defaultFontForRole(bw::FontRole::Mono);
};

void setConfig(const Config& cfg);
void setFontFamily(FontFamily family);
void init();

juce::Font body(float pt);
juce::Font caption(float pt);
juce::Font title(float pt);
juce::Font mono(float pt);
} // namespace bw::Fonts

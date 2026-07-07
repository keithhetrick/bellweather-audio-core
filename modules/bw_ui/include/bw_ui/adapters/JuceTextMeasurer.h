// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "bw_ui/adapters/UiThemeKernelAdapter.h"
#include "bw_ui/foundation/Fonts.h"
#include "bw_ui/kernel/ReadoutValue.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace bws::ui::adapters
{

// The one place GlyphArrangement text measurement lives (replaces the per-file copies).
inline float glyphWidth(const juce::Font& font, const juce::String& text) noexcept
{
    if (text.isEmpty())
        return 0.0f;
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(font, text, 0.0f, 0.0f);
    return glyphs.getBoundingBox(0, -1, true).getWidth();
}

// JUCE implementation of the kernel text-measurement seam.
// `useMono` measures in the equal-width numeral font used to render values.
class JuceTextMeasurer : public kernel::ITextMeasurer
{
public:
    explicit JuceTextMeasurer(kernel::ThemeSnapshot theme, bool useMono = false)
        : theme_(std::move(theme))
        , useMono_(useMono)
    {}

    float measure(const std::string& text, kernel::TextRole role, float scale) const override
    {
        const auto base = makeFont(theme_, role, scale);
        const auto font = useMono_ ? bw::Fonts::mono(base.getHeight()) : base;
        return glyphWidth(font, juce::String(text));
    }

private:
    kernel::ThemeSnapshot theme_;
    bool useMono_ {false};
};

} // namespace bws::ui::adapters

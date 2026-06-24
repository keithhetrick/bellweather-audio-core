// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_graphics/juce_graphics.h>

namespace bws::toy_shell
{

/// 4-digit zero-padded seconds-elapsed formatter, e.g. 42.7 → "0042".
[[nodiscard]] juce::String fmtTime4(double seconds);

/// Translucent god-mode-toggle overlay shared between easter-egg renderers.
/// `textToShow` defaults to "GOD MODE UNLOCKED"; callers pass alternate
/// copy for the mode-off flash. Caller is responsible for normalising
/// `fade01` into [0, 1] (Storm Runner stores a 0..1 fade directly; Signal
/// Lock stores seconds-remaining and divides by its FLASH_SOLVED constant
/// before calling).
void paintSolvedOverlay(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour wash, juce::Colour text,
                        float fade01, juce::StringRef textToShow = "GOD MODE UNLOCKED");

} // namespace bws::toy_shell

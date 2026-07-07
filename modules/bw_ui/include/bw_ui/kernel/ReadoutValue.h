// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "bw_ui/kernel/Typography.h"
#include <optional>
#include <string>

namespace bws::ui::kernel
{

// Canonical parameter-value display: one formatting engine, one parse inverse, and a
// width-reservation that keeps a value from reflowing as it changes. JUCE-free; text
// measurement is injected via ITextMeasurer (implemented in the adapter layer).

enum class ValueUnit
{
    Plain,
    Hertz,
    Decibels,
    Percent,
    Ratio,
    Milliseconds,
    Custom // free-form affix carried in FormatSpec::unitText (e.g. " dB/s", "Q ").
};

enum class AffixPosition
{
    Suffix,
    Prefix
};

struct FormatSpec
{
    ValueUnit unit {ValueUnit::Plain};
    int decimals {1};
    bool showSign {false};

    // ValueUnit::Custom only: the affix text and which side it sits on.
    std::string unitText {};
    AffixPosition affix {AffixPosition::Suffix};

    // Rail sentinel: at or below floorValue the engine renders floorLabel verbatim
    // (e.g. "-∞ dB" at a gain floor). Applies to any unit.
    std::optional<double> floorValue {};
    std::string floorLabel {};
};

// value is the real (denormalized) parameter value (Hz, dB, %, …).
std::string formatValue(double value, const FormatSpec& spec);

// Inverse of formatValue (best-effort): parses a display string back to a value.
std::optional<double> parseValue(const std::string& text, const FormatSpec& spec);

// Text measurement seam - implemented by adapters::JuceTextMeasurer.
class ITextMeasurer
{
public:
    virtual ~ITextMeasurer() = default;
    virtual float measure(const std::string& text, TextRole role, float scale) const = 0;
};

// Width to reserve so the value cell never reflows: measures the worst-case strings the
// spec can produce over [minValue, maxValue] (range extremes + unit/sign boundaries),
// not just the endpoints.
float reservedValueWidth(const FormatSpec& spec, double minValue, double maxValue, const ITextMeasurer& measurer,
                         TextRole role, float scale);

} // namespace bws::ui::kernel

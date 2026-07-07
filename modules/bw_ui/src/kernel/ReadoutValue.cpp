// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/kernel/ReadoutValue.h"

#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace bws::ui::kernel
{
namespace
{
std::string fixed(double value, int decimals, bool showSign)
{
    char buf[32];
    std::snprintf(buf, sizeof(buf), showSign ? "%+.*f" : "%.*f", decimals, value);
    return buf;
}

// Parse the first signed/decimal number in text, skipping any leading affix (e.g. "Q ").
std::optional<double> leadingNumber(const std::string& text)
{
    size_t i = 0;
    while (i < text.size())
    {
        const char c = text[i];
        if (std::isdigit(static_cast<unsigned char>(c)) || c == '+' || c == '-' || c == '.')
            break;
        ++i;
    }
    if (i >= text.size())
        return std::nullopt;

    const char* begin = text.c_str() + i;
    char* end = nullptr;
    const double v = std::strtod(begin, &end);
    if (end == begin)
        return std::nullopt;
    return v;
}
} // namespace

std::string formatValue(double value, const FormatSpec& spec)
{
    if (spec.floorValue && value <= *spec.floorValue)
        return spec.floorLabel;

    switch (spec.unit)
    {
    case ValueUnit::Hertz:
        if (std::abs(value) >= 1000.0)
            return fixed(value / 1000.0, spec.decimals, spec.showSign) + " kHz";
        return fixed(value, 0, spec.showSign) + " Hz";
    case ValueUnit::Decibels:
        return fixed(value, spec.decimals, spec.showSign) + " dB";
    case ValueUnit::Percent:
        return fixed(value, spec.decimals, spec.showSign) + "%";
    case ValueUnit::Ratio:
        return fixed(value, spec.decimals, false) + ":1";
    case ValueUnit::Milliseconds:
        return fixed(value, spec.decimals, spec.showSign) + " ms";
    case ValueUnit::Custom:
    {
        const std::string num = fixed(value, spec.decimals, spec.showSign);
        return spec.affix == AffixPosition::Prefix ? spec.unitText + num : num + spec.unitText;
    }
    case ValueUnit::Plain:
    default:
        return fixed(value, spec.decimals, spec.showSign);
    }
}

std::optional<double> parseValue(const std::string& text, const FormatSpec& spec)
{
    if (spec.floorValue && !spec.floorLabel.empty() && text == spec.floorLabel)
        return *spec.floorValue;

    const auto raw = leadingNumber(text);
    if (!raw)
        return std::nullopt;

    if (spec.unit == ValueUnit::Hertz)
    {
        // Honor an explicit k/kHz suffix; otherwise the bare number is Hz.
        for (char c : text)
            if (c == 'k' || c == 'K')
                return *raw * 1000.0;
        return *raw;
    }
    return *raw;
}

float reservedValueWidth(const FormatSpec& spec, double minValue, double maxValue, const ITextMeasurer& measurer,
                         TextRole role, float scale)
{
    // Worst-case candidates: range extremes + the boundaries where the rendered string is
    // widest (kHz crossover for frequency, the signed extreme, zero) - not just endpoints,
    // which can miss a mid-range-widest value.
    std::array<double, 5> candidates {minValue, maxValue, 0.0, 0.0, 0.0};
    int count = 2;
    if (minValue < 0.0 && maxValue > 0.0)
        candidates[count++] = 0.0;
    if (spec.unit == ValueUnit::Hertz)
    {
        if (minValue <= 999.0 && maxValue >= 999.0)
            candidates[count++] = 999.0;
        if (minValue <= 1000.0 && maxValue >= 1000.0)
            candidates[count++] = 1000.0;
    }

    float widest = 0.0f;
    for (int i = 0; i < count; ++i)
        widest = std::max(widest, measurer.measure(formatValue(candidates[static_cast<size_t>(i)], spec), role, scale));
    if (spec.floorValue && !spec.floorLabel.empty())
        widest = std::max(widest, measurer.measure(spec.floorLabel, role, scale));
    return widest;
}

} // namespace bws::ui::kernel

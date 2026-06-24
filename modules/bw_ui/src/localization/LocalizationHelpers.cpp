// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/localization/LocalizationHelpers.h"

namespace bws::ui::localization
{
namespace
{
constexpr const char* kFooterKey = "knob.tooltip.footer";
constexpr const char* kFooterWithDefaultKey = "knob.tooltip.footer.withDefault";
constexpr const char* kFooterFallback = "Cmd+Drag: Fine adjust\nAlt+Click: Reset\nDouble-Click: Text entry";
constexpr const char* kFooterWithDefaultFallback =
    "Cmd+Drag: Fine adjust\nAlt+Click: Reset to {default}\nDouble-Click: Text entry";

juce::String resolveOrFallback(const LocalizationManager& loc, juce::StringRef key, juce::StringRef fallback)
{
    return loc.hasKey(key) ? loc.get(key) : juce::String(fallback);
}
} // namespace

juce::String composeKnobTooltip(const LocalizationManager& loc, juce::StringRef purposeKey,
                                juce::StringRef purposeFallback, const juce::String& defaultValue)
{
    const auto purpose = resolveOrFallback(loc, purposeKey, purposeFallback);

    juce::String footer;
    if (defaultValue.isNotEmpty())
    {
        if (loc.hasKey(kFooterWithDefaultKey))
            footer = loc.format(kFooterWithDefaultKey, {{"default", defaultValue}});
        else
            footer = juce::String(kFooterWithDefaultFallback).replace("{default}", defaultValue);
    }
    else
    {
        footer = resolveOrFallback(loc, kFooterKey, kFooterFallback);
    }

    return purpose + "\n\n" + footer;
}

} // namespace bws::ui::localization

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_core/juce_core.h>

namespace bws::ui
{

inline juce::String activeThemeName()
{
    const auto preview = juce::SystemStats::getEnvironmentVariable("BWS_UI_GALLERY_PREVIEW_THEME", "");
    if (preview.isNotEmpty())
        return preview;
    const auto envTheme = juce::SystemStats::getEnvironmentVariable("BWS_UI_THEME", "");
    return envTheme;
}

inline bool isPreviewThemeActive()
{
    auto name = activeThemeName();
    return name.startsWithIgnoreCase("moodboard_");
}

inline bool isPunkTheme()
{
    auto name = activeThemeName();
    return name.startsWithIgnoreCase("moodboard_punk_metal_v1");
}

inline bool isSageTheme()
{
    auto name = activeThemeName();
    return name.startsWithIgnoreCase("moodboard_sage_plate_v1");
}

inline bool isKolorTheme()
{
    auto name = activeThemeName();
    return name.startsWithIgnoreCase("moodboard_kolor_v1");
}

inline juce::String galleryCapturePhase()
{
    const auto phase = juce::SystemStats::getEnvironmentVariable("BWS_UI_GALLERY_CAPTURE_PHASE", "");
    return phase.isNotEmpty() ? phase.toLowerCase() : juce::String("baseline");
}

inline bool isGalleryBaselinePhase()
{
    return galleryCapturePhase().equalsIgnoreCase("baseline");
}

inline bool isGalleryPreviewPhase()
{
    return galleryCapturePhase().equalsIgnoreCase("preview");
}

inline bool layoutDebugEnabled()
{
    const auto v = juce::SystemStats::getEnvironmentVariable("BWS_UI_SHOW_LAYOUT_DEBUG", "");
    return v.isNotEmpty() && v != "0";
}

} // namespace bws::ui

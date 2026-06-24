// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/internal/UiBootstrap.h"

#include <mutex>
#include "bw_ui/foundation/Fonts.h"
#include <cstdlib>

namespace bws::ui
{
namespace
{
std::once_flag gOnce;
} // namespace

void UiBootstrap::init()
{
    std::call_once(gOnce, []() {
        if (const char* ff = std::getenv("BWS_UI_FONT_FAMILY"))
        {
            juce::String fam(ff);
            fam = fam.toLowerCase().trim();
            if (fam == "jetbrains" || fam == "jetbrainsmono")
                bw::Fonts::setFontFamily(bw::Fonts::FontFamily::JetBrainsMono);
            else
                bw::Fonts::setFontFamily(bw::Fonts::FontFamily::Inter);
        }
        bw::Fonts::init();
    });
}
} // namespace bws::ui

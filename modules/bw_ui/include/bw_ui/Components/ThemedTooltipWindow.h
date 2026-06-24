// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "bw_ui/foundation/UiTheme.h"

namespace bws::ui
{

class ThemedTooltipWindow : public juce::TooltipWindow
{
public:
    ThemedTooltipWindow(juce::Component* parentComp, const UiThemeResolved& theme, int delayMs = 200);
    ~ThemedTooltipWindow() override;

    void setTheme(const UiThemeResolved& newTheme);
    void setScale(float newScale) noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace bws::ui

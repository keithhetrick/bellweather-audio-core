// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <optional>
#include <juce_gui_basics/juce_gui_basics.h>
#include "bw_ui/Components/UiLabel.h"
#include "bw_ui/kernel/Theme.h"
#include "bw_ui/foundation/UiTheme.h"

namespace bws::ui
{

class UiSectionHeader : public juce::Component
{
public:
    UiSectionHeader(const UiThemeResolved& themeIn, juce::String titleText,
                    std::optional<juce::String> subtitleText = std::nullopt, bool denseIn = false);

    void setTheme(const UiThemeResolved& newTheme);
    void setScale(float newScale);
    void setTitle(juce::String newTitle);
    void setSubtitle(std::optional<juce::String> newSubtitle);
    void setDense(bool shouldBeDense);

    int preferredHeight() const;
    static int preferredHeight(bool dense, float scale, const UiThemeResolved& theme, bool hasSubtitle);

    void resized() override;

private:
    UiThemeResolved theme;
    kernel::ThemeSnapshot kernelTheme;
    UiLabel titleLabel;
    UiLabel subtitleLabel;
    juce::String title;
    std::optional<juce::String> subtitle;
    float scale {1.0f};
    bool dense {false};

    int verticalPadding() const;
    int horizontalPadding() const;
    int gap() const;
};

} // namespace bws::ui

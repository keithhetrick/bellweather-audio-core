// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <optional>
#include <juce_gui_basics/juce_gui_basics.h>
#include "bw_ui/adapters/UiThemeKernelAdapter.h"
#include "bw_ui/foundation/UiTheme.h"

namespace bws::ui
{

class UiLabel : public juce::Component
{
public:
    UiLabel(UiTypographyRole role, const UiThemeResolved& theme);

    void setText(const juce::String& newText, juce::NotificationType notification = juce::sendNotification);
    void setRole(UiTypographyRole newRole);
    void setTheme(const UiThemeResolved& newTheme);
    void setScale(float newScale);
    void setUseSecondaryColour(bool shouldUseSecondary);
    void setKernelTheme(const kernel::ThemeSnapshot& newTheme);
    void setTextRole(kernel::TextRole newRole);
    const juce::String& getText() const noexcept { return text; }

    void paint(juce::Graphics& g) override;

private:
    UiTypographyRole role;
    UiThemeResolved theme;
    std::optional<kernel::ThemeSnapshot> kernelTheme;
    std::optional<kernel::TextRole> kernelRole;
    juce::String text;
    float scale {1.0f};
    bool useSecondary {false};
};

} // namespace bws::ui

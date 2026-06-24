// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "bw_ui/adapters/UiThemeKernelAdapter.h"
#include "bw_ui/foundation/UiTheme.h"

namespace bws::ui
{

class UiHelpPopover : public juce::Component
{
public:
    explicit UiHelpPopover(const UiThemeResolved& themeIn);

    void setText(const juce::String& newText);
    void setDense(bool shouldBeDense);
    void setTheme(const UiThemeResolved& newTheme);
    void setScale(float newScale);

    juce::Rectangle<int> preferredSize(int maxWidth = 320) const;

    void paint(juce::Graphics& g) override;

private:
    UiThemeResolved theme;
    kernel::ThemeSnapshot kernelTheme;
    juce::String text;
    float scale {1.0f};
    bool dense {false};

    int padding() const;
    float radius() const;
    juce::Font font() const;
};

} // namespace bws::ui

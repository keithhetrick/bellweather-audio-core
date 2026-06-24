// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "bw_ui/adapters/UiThemeKernelAdapter.h"
#include "bw_ui/Components/UiLabel.h"
#include "bw_ui/foundation/UiTheme.h"

namespace bws::ui
{

class UiGraphPanel : public juce::Component
{
public:
    UiGraphPanel(const UiThemeResolved& themeIn, bool denseIn = false);

    void setTheme(const UiThemeResolved& newTheme);
    void setScale(float newScale);
    void setTitle(const juce::String& newTitle);
    void setDense(bool shouldBeDense);

    juce::Rectangle<int> getContentBounds() const noexcept;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void updateLayout();
    float cornerRadius() const;
    int padding() const;
    int titleGap() const;

    UiThemeResolved theme;
    kernel::ThemeSnapshot kernelTheme;
    UiLabel titleLabel;
    juce::String title;
    bool dense {false};
    float scale {1.0f};
    mutable juce::Rectangle<int> contentArea;
};

} // namespace bws::ui

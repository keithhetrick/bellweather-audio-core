// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <functional>
#include <vector>
#include <juce_gui_basics/juce_gui_basics.h>
#include "bw_ui/adapters/UiThemeKernelAdapter.h"
#include "bw_ui/foundation/UiTheme.h"

namespace bws::ui
{

class UiTabBar : public juce::Component
{
public:
    UiTabBar(const UiThemeResolved& themeIn, bool denseIn = false);

    void setTheme(const UiThemeResolved& newTheme);
    void setScale(float newScale);
    void setTabs(const std::vector<juce::String>& titles);
    void setSelectedIndex(int index);
    int getSelectedIndex() const noexcept { return selectedIndex; }
    void setOnChange(std::function<void(int)> handler);
    void setDense(bool shouldBeDense);

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;
    void focusGained(FocusChangeType) override;
    void focusLost(FocusChangeType) override;

private:
    struct TabRect
    {
        juce::Rectangle<int> bounds;
        juce::String label;
    };

    juce::Rectangle<int> contentBounds() const;
    float tabHeight() const;
    float tabPaddingX() const;
    float tabGap() const;
    float cornerRadius() const;
    juce::Font labelFont() const;
    void updateLayout();
    int hitTestIndex(juce::Point<int> pos) const;

    UiThemeResolved theme;
    kernel::ThemeSnapshot kernelTheme;
    std::vector<TabRect> tabs;
    std::function<void(int)> onChange;
    float scale {1.0f};
    int selectedIndex {0};
    int hoverIndex {-1};
    int pressIndex {-1};
    bool dense {false};
};

} // namespace bws::ui

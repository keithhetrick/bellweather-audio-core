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

struct TabSpec
{
    juce::String id;
    juce::String label;
};

class UiTabs : public juce::Component
{
public:
    UiTabs(const UiThemeResolved& theme, UiTabRole role = UiTabRole::Standard);

    void setTabs(const std::vector<TabSpec>& tabs);
    void setTheme(const UiThemeResolved& theme);
    void setRole(UiTabRole role);
    void setScale(float newScale);
    void setOnChange(std::function<void(juce::String)> callback);
    void setActive(const juce::String& id);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    struct TabEntry
    {
        std::unique_ptr<juce::TextButton> button;
        TabSpec spec;
    };

    class TabsLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        TabsLookAndFeel(const UiThemeResolved* themeRef, kernel::ThemeSnapshot* kernelThemeRef, UiTabRole* roleRef,
                        float* scaleRef);
        void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                                  bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
        juce::Font getTextButtonFont(juce::TextButton& b, int height) override;

    private:
        const UiThemeResolved* theme {nullptr};
        kernel::ThemeSnapshot* kernelTheme {nullptr};
        UiTabRole* role {nullptr};
        float* scale {nullptr};
    };

    UiThemeResolved theme;
    kernel::ThemeSnapshot kernelTheme;
    UiTabRole role;
    float scale {1.0f};
    TabsLookAndFeel lnf;
    std::function<void(juce::String)> onChange;
    std::vector<TabEntry> entries;
    juce::String activeId;
};

} // namespace bws::ui

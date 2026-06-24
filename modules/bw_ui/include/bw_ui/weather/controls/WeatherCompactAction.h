// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <bw_ui/foundation/UiTheme.h>
#include "bw_ui/weather/controls/WeatherControlSurface.h"

namespace bws::weather
{

class TogglePill : public juce::Button
{
public:
    explicit TogglePill(const juce::String& label = {},
                        control_surface::CompactActionWidth widthKind = control_surface::CompactActionWidth::Standard);

    void setTheme(const bws::ui::UiThemeResolved& theme);
    void setScaleFactor(float scale);
    void setLocked(bool locked);
    bool isLocked() const noexcept { return locked_; }
    void setWidthKind(control_surface::CompactActionWidth widthKind);

    // Label-fit width: text advance + 2x the padding token.
    int preferredWidth() const;

    void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    const bws::ui::UiThemeResolved& resolvedTheme() const;

    const bws::ui::UiThemeResolved* theme_ = nullptr;
    float scaleFactor_ = 1.0f;
    bool locked_ = false;
    control_surface::CompactActionWidth widthKind_;
};

class ActionPill : public juce::Button
{
public:
    explicit ActionPill(const juce::String& label = {},
                        control_surface::CompactActionWidth widthKind = control_surface::CompactActionWidth::Standard);

    void setTheme(const bws::ui::UiThemeResolved& theme);
    void setScaleFactor(float scale);
    void setWidthKind(control_surface::CompactActionWidth widthKind);

    // Label-fit width: text advance + 2x the padding token.
    int preferredWidth() const;

    void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    const bws::ui::UiThemeResolved& resolvedTheme() const;

    const bws::ui::UiThemeResolved* theme_ = nullptr;
    float scaleFactor_ = 1.0f;
    control_surface::CompactActionWidth widthKind_;
};

class UpdateToken : public ActionPill
{
public:
    explicit UpdateToken(const juce::String& label = {});

    void setProductName(const juce::String& productName);
    void setVersionString(const juce::String& version);
    void clearVersion();

private:
    void refreshTooltip();

    juce::String productName_;
    juce::String versionString_;
};

class UtilityAction : public juce::Button
{
public:
    explicit UtilityAction(const juce::String& label = {});

    void setTheme(const bws::ui::UiThemeResolved& theme);
    void setScaleFactor(float scale);

    void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    const bws::ui::UiThemeResolved& resolvedTheme() const;

    const bws::ui::UiThemeResolved* theme_ = nullptr;
    float scaleFactor_ = 1.0f;
};

} // namespace bws::weather

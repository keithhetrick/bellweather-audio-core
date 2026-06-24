// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <bw_ui/foundation/UiTheme.h>
#include <vector>

namespace bws::weather
{

enum class BadgeTone
{
    Neutral,
    Subtle,
    Accent,
    Status
};

enum class BadgeSize
{
    Compact,
    Default
};

enum class LibraryRowDensity
{
    Compact,
    Default
};

class WeatherFloatingPanelShell : public juce::Component
{
public:
    WeatherFloatingPanelShell() = default;

    void setTheme(const bws::ui::UiThemeResolved& theme);
    void setScaleFactor(float scaleFactor);
    void setDividerY(int dividerY);

    juce::Rectangle<int> getContentBounds() const;

    void paint(juce::Graphics& g) override;

private:
    const bws::ui::UiThemeResolved* theme_ = nullptr;
    float scaleFactor_ = 1.0f;
    int dividerY_ = 0;
};

class WeatherChipButton : public juce::Button
{
public:
    WeatherChipButton();

    void setTheme(const bws::ui::UiThemeResolved& theme);
    void setScaleFactor(float scaleFactor);

    void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    const bws::ui::UiThemeResolved* theme_ = nullptr;
    float scaleFactor_ = 1.0f;
};

class WeatherFilterChipGroup : public juce::Component
{
public:
    struct Option
    {
        juce::String id;
        juce::String label;
    };

    WeatherFilterChipGroup();

    void setTheme(const bws::ui::UiThemeResolved& theme);
    void setScaleFactor(float scaleFactor);

    void setOptions(const std::vector<Option>& options);
    void setSelectedId(const juce::String& selectedId);
    juce::String getSelectedId() const { return selectedId_; }
    const bws::ui::UiThemeResolved& getResolvedTheme() const;
    float getScaleFactor() const { return scaleFactor_; }

    int getPreferredHeight() const;
    int getChipCount() const;

    std::function<void(const juce::String& selectedId)> onSelectionChanged;

    void resized() override;

private:
    void rebuildButtons();
    void syncButtonStates();

    const bws::ui::UiThemeResolved* theme_ = nullptr;
    float scaleFactor_ = 1.0f;
    std::vector<Option> options_;
    juce::String selectedId_;
    juce::OwnedArray<WeatherChipButton> buttons_;
};

class WeatherStatusBadge : public juce::Component
{
public:
    WeatherStatusBadge() = default;

    void setTheme(const bws::ui::UiThemeResolved& theme);
    void setScaleFactor(float scaleFactor);
    void setLabel(const juce::String& label);
    void setTone(BadgeTone tone);
    void setSizeVariant(BadgeSize size);

    juce::String getLabel() const { return label_; }
    BadgeTone getTone() const { return tone_; }

    int getPreferredWidth() const;
    int getPreferredHeight() const;

    void paint(juce::Graphics& g) override;

private:
    const bws::ui::UiThemeResolved* theme_ = nullptr;
    float scaleFactor_ = 1.0f;
    juce::String label_;
    BadgeTone tone_ = BadgeTone::Neutral;
    BadgeSize size_ = BadgeSize::Compact;
};

class WeatherLibraryRow : public juce::Component
{
public:
    struct Badge
    {
        juce::String label;
        BadgeTone tone = BadgeTone::Neutral;
    };

    struct Model
    {
        juce::String title;
        juce::String metadata;
        std::vector<Badge> badges;
        bool selected = false;
        bool highlighted = false;
        bool favourite = false;
        bool favouriteEnabled = true;
    };

    WeatherLibraryRow();

    void setTheme(const bws::ui::UiThemeResolved& theme);
    void setScaleFactor(float scaleFactor);
    void setDensity(LibraryRowDensity density);
    void setModel(const Model& model);

    const Model& getModel() const { return model_; }
    LibraryRowDensity getDensity() const { return density_; }

    int getPreferredHeight() const;
    int getVisibleBadgeCount() const;
    juce::Rectangle<int> getFavouriteBounds() const;

    std::function<void()> onRowPressed;
    std::function<void()> onRowDoubleClicked;
    std::function<void()> onFavouritePressed;

    void resized() override;
    void paint(juce::Graphics& g) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

private:
    void rebuildBadges();

    const bws::ui::UiThemeResolved* theme_ = nullptr;
    float scaleFactor_ = 1.0f;
    LibraryRowDensity density_ = LibraryRowDensity::Default;
    Model model_;
    juce::OwnedArray<WeatherStatusBadge> badges_;
};

} // namespace bws::weather

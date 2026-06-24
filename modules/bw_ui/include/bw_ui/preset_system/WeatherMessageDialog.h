// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <bw_ui/foundation/UiTheme.h>
#include <bw_ui/localization/LocalizationManager.h>
#include <bw_ui/preset_system/WeatherDiscoveryPrimitives.h>

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>

namespace bws::weather
{

enum class WeatherMessageDialogTone
{
    Neutral,
    Warning
};

class WeatherMessageDialog
    : public juce::Component
    , private bws::ui::localization::LocalizationManager::Listener
{
public:
    WeatherMessageDialog(const bws::ui::UiThemeResolved& theme, float scaleFactor, juce::String title,
                         juce::String message, WeatherMessageDialogTone tone, juce::String dismissLabel,
                         std::function<void()> onDismiss);

    WeatherMessageDialog(const bws::ui::UiThemeResolved& theme, float scaleFactor,
                         bws::ui::localization::LocalizationManager* localization,
                         bws::ui::localization::LocalizedText title, bws::ui::localization::LocalizedText message,
                         WeatherMessageDialogTone tone, bws::ui::localization::LocalizedText dismissLabel,
                         std::function<void()> onDismiss);

    ~WeatherMessageDialog() override;

    void setDismissCallback(std::function<void()> cb) { dismissCallback_ = std::move(cb); }
    void refreshText();

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    class MessageTextLayer;

    juce::Rectangle<int> getCardBounds() const;
    void localizationLanguageChanged() override;
    void dismiss();

    const bws::ui::UiThemeResolved& theme_;
    const float scaleFactor_;
    bws::ui::localization::LocalizationManager* localization_ = nullptr;
    bws::ui::localization::LocalizedText titleText_;
    bws::ui::localization::LocalizedText messageText_;
    bws::ui::localization::LocalizedText dismissText_;
    juce::String resolvedTitle_;
    juce::String resolvedMessage_;
    const WeatherMessageDialogTone tone_;
    std::function<void()> dismissCallback_;

    WeatherFloatingPanelShell shell_;
    std::unique_ptr<MessageTextLayer> textLayer_;
    WeatherChipButton dismissButton_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WeatherMessageDialog)
};

void showMessageDialog(const bws::ui::UiThemeResolved& theme, float scaleFactor, juce::String title,
                       juce::String message, juce::Component* parent,
                       WeatherMessageDialogTone tone = WeatherMessageDialogTone::Neutral,
                       juce::String dismissLabel = "OK", bool desktopPromote = false);

void showLocalizedMessageDialog(
    const bws::ui::UiThemeResolved& theme, float scaleFactor, bws::ui::localization::LocalizationManager& localization,
    bws::ui::localization::LocalizedText title, bws::ui::localization::LocalizedText message, juce::Component* parent,
    WeatherMessageDialogTone tone = WeatherMessageDialogTone::Neutral,
    bws::ui::localization::LocalizedText dismissLabel = bws::ui::localization::LocalizedText::key("common.ok"),
    bool desktopPromote = false);

} // namespace bws::weather

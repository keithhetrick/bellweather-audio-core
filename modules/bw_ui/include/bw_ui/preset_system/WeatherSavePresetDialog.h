// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

/**
 * @file WeatherSavePresetDialog.h
 * @brief Shared "Save Preset" dialog for Weather Design System plugins.
 *
 * Captures Name + Category from the user and fires `onCommit(name, category)`.
 * The callback decouples form-capture from facade-save logic: a consumer may
 * invoke savePreset() directly, or chain into its own overwrite-check flow (as
 * Barometer does). Dialog is modal (enterModalState) to block interaction with the editor
 * while text input is focused.
 */

#include <bw_ui/foundation/UiTheme.h>
#include <bw_ui/localization/LocalizationManager.h>
#include <bw_ui/preset_system/WeatherDiscoveryPrimitives.h>

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace bws::weather
{

class WeatherSavePresetDialog
    : public juce::Component
    , private bws::ui::localization::LocalizationManager::Listener
{
public:
    using OnCommit = std::function<void(const juce::String& name, const juce::String& category)>;

    WeatherSavePresetDialog(const bws::ui::UiThemeResolved& theme, float scaleFactor, juce::String defaultName,
                            juce::String defaultCategory, OnCommit onCommit, std::function<void()> onDismiss);

    WeatherSavePresetDialog(const bws::ui::UiThemeResolved& theme, float scaleFactor,
                            bws::ui::localization::LocalizationManager* localization, juce::String defaultName,
                            juce::String defaultCategory, OnCommit onCommit, std::function<void()> onDismiss);

    ~WeatherSavePresetDialog() override;

    void setDismissCallback(std::function<void()> cb) { dismissCallback_ = std::move(cb); }
    void refreshText();

    // Routes keyboard focus onto the Name editor after the dialog is parented
    // and visible, so host transport shortcuts cannot intercept text input.
    void grabInitialKeyboardFocus();

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    juce::Rectangle<int> getCardBounds() const;
    void localizationLanguageChanged() override;
    void commit();
    void cancel();
    void updateSaveEnabled();

    const bws::ui::UiThemeResolved& theme_;
    const float scaleFactor_;
    bws::ui::localization::LocalizationManager* localization_ = nullptr;
    bws::ui::localization::LocalizedText titleText_;
    bws::ui::localization::LocalizedText subtitleText_;
    bws::ui::localization::LocalizedText nameFieldText_;
    bws::ui::localization::LocalizedText categoryFieldText_;
    bws::ui::localization::LocalizedText cancelText_;
    bws::ui::localization::LocalizedText saveText_;
    OnCommit onCommit_;
    std::function<void()> dismissCallback_;

    WeatherFloatingPanelShell shell_;

    juce::Label titleLabel_;
    juce::Label subtitleLabel_;
    juce::Label nameFieldLabel_;
    juce::Label categoryFieldLabel_;

    juce::TextEditor nameEditor_;
    juce::TextEditor categoryEditor_;

    WeatherChipButton saveButton_;
    WeatherChipButton cancelButton_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WeatherSavePresetDialog)
};

// Parent should typically be the editor / top-level component. Dialog is
// allocated on the heap and deletes itself via SafePointer on dismiss.
//
// `desktopPromote` has NO default - callers must consciously decide. In
// practice every real call site should pass
// `bws::adapters::hostInterceptsPluginKeyboardInput()` (from
// bw_juce_adapters/HostEnvironment.h). True → dialog is desktop-promoted
// via addToDesktop (AlertWindow-equivalent; required in Reaper on macOS
// to bypass host-level spacebar hijack). False → embedded overlay with
// backdrop dim (correct default for every other tested DAW).
//
//
// rationale + the list of hosts this distinction matters for.
void showSavePresetDialog(const bws::ui::UiThemeResolved& theme, float scaleFactor, juce::String defaultName,
                          juce::String defaultCategory, WeatherSavePresetDialog::OnCommit onCommit,
                          juce::Component* parent, bool desktopPromote);

void showLocalizedSavePresetDialog(const bws::ui::UiThemeResolved& theme, float scaleFactor,
                                   bws::ui::localization::LocalizationManager& localization, juce::String defaultName,
                                   juce::String defaultCategory, WeatherSavePresetDialog::OnCommit onCommit,
                                   juce::Component* parent, bool desktopPromote);

} // namespace bws::weather

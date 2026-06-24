// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

/**
 * @file WeatherManagePresetsDialog.h
 * @brief Shared "Manage User Presets" dialog for Weather Design System plugins.
 *
 * Thin view over ManagePresetsController. Exposes rename / delete / in-place
 * overwrite ("Update") for user presets. Factory presets never appear.
 *
 * Contract:
 */

#include <bw_ui/foundation/UiTheme.h>
#include <bw_ui/localization/LocalizationManager.h>
#include <bw_ui/preset_system/ManagePresetsController.h>
#include <bw_ui/preset_system/WeatherDiscoveryPrimitives.h>
#include <bw_ui/preset_system/WeatherPresetManager.h>

#include <juce_gui_basics/juce_gui_basics.h>

namespace bws::weather
{

class WeatherManagePresetsDialog
    : public juce::Component
    , private juce::ListBoxModel
    , private bws::ui::localization::LocalizationManager::Listener
{
public:
    WeatherManagePresetsDialog(WeatherPresetManager& presetManager, const bws::ui::UiThemeResolved& theme,
                               float scaleFactor, std::function<void()> onDismiss);

    WeatherManagePresetsDialog(WeatherPresetManager& presetManager, const bws::ui::UiThemeResolved& theme,
                               float scaleFactor, bws::ui::localization::LocalizationManager* localization,
                               std::function<void()> onDismiss);

    ~WeatherManagePresetsDialog() override;

    void setDismissCallback(std::function<void()> cb) { dismissCallback_ = std::move(cb); }
    void refreshText();

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    // ListBoxModel
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent& e) override;

    enum class Mode
    {
        Main,
        ConfirmDelete,
        Rename,
        ConfirmUpdate,
    };

    void refreshList();
    void enterMode(Mode m);
    int getSelectedRow() const;
    juce::String selectedPresetName() const;
    void commitDelete();
    void commitRename();
    void commitUpdate();
    void dismiss();
    void localizationLanguageChanged() override;

    juce::Rectangle<int> getCardBounds() const;

    ManagePresetsController controller_;
    const bws::ui::UiThemeResolved& theme_;
    const float scaleFactor_;
    bws::ui::localization::LocalizationManager* localization_ = nullptr;
    std::function<void()> dismissCallback_;

    Mode mode_ {Mode::Main};
    std::vector<ManagePresetsController::UserPresetInfo> presets_;

    WeatherFloatingPanelShell shell_;

    juce::Label titleLabel_;
    juce::Label modePromptLabel_;

    juce::ListBox presetList_ {"User Presets", this};

    WeatherChipButton updateButton_;
    WeatherChipButton renameButton_;
    WeatherChipButton deleteButton_;
    WeatherChipButton closeButton_;

    WeatherChipButton confirmYesButton_;
    WeatherChipButton confirmNoButton_;

    juce::TextEditor renameEditor_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WeatherManagePresetsDialog)
};

// Parent should typically be the editor / top-level component, not a transient
// popup-menu owner (the popup may dismiss while the dialog is still open).
void showManagePresetsDialog(WeatherPresetManager& presetManager, const bws::ui::UiThemeResolved& theme,
                             float scaleFactor, juce::Component* parent);

void showLocalizedManagePresetsDialog(WeatherPresetManager& presetManager, const bws::ui::UiThemeResolved& theme,
                                      float scaleFactor, bws::ui::localization::LocalizationManager& localization,
                                      juce::Component* parent);

// Appends a "Manage User Presets..." item to an existing popup menu. The item
// is enabled iff the manager reports at least one user preset. On selection,
// the shared dialog opens with `parent` as its overlay host (should be the
// editor / top-level component, not a transient popup owner).
//
// Use this from any plugin that builds its own settings menu - it keeps the
// entry point consistent so plugins can't silently omit CRUD access.
void appendManagePresetsMenuItem(juce::PopupMenu& menu, WeatherPresetManager& presetManager,
                                 const bws::ui::UiThemeResolved& theme, float scaleFactor, juce::Component* parent);

void appendLocalizedManagePresetsMenuItem(juce::PopupMenu& menu, WeatherPresetManager& presetManager,
                                          const bws::ui::UiThemeResolved& theme, float scaleFactor,
                                          bws::ui::localization::LocalizationManager& localization,
                                          juce::Component* parent);

} // namespace bws::weather

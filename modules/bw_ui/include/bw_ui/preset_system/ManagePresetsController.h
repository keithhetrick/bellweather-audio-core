// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <bw_ui/preset_system/WeatherPresetManager.h>

#include <juce_core/juce_core.h>

#include <variant>
#include <vector>

namespace bws::weather
{

// Pure logic layer between WeatherManagePresetsDialog and WeatherPresetManager.
// All dialog-side validation and preview-session orchestration lives here;
// the dialog is a thin view.
//
// Contract:
class ManagePresetsController
{
public:
    struct UserPresetInfo
    {
        juce::String name;
        juce::String category;
        juce::String author;
        juce::String description;
        juce::String presetId;
        bool isFactory {false}; // always false (filter invariant); kept for test clarity
    };

    struct Ok
    {};
    enum class RenameRejection
    {
        Empty,
        Unchanged
    };
    using RenameResult = std::variant<Ok, RenameRejection>;

    explicit ManagePresetsController(WeatherPresetManager& presetManager) noexcept;

    // Contract §B1: enabled only when user presets exist.
    bool shouldEnable() const;

    // Contract §B1: cancels any active preview session. Idempotent.
    void onOpen();

    // Contract §B10: filtered to !isFactory.
    std::vector<UserPresetInfo> listUserPresets() const;

    // Contract §B2: delegates to WeatherPresetManager::deletePreset.
    bool requestDelete(const juce::String& name);

    // Contract §B4-B6: validates non-empty (trimmed) and changed before delegating
    // to WeatherPresetManager::renamePreset.
    RenameResult requestRename(const juce::String& oldName, const juce::String& newName);

    // Contract §B8: overwrites by re-invoking savePreset with the preset's existing
    // category/author/description (NOT saveUserPreset, which forces category="User").
    bool requestUpdate(const juce::String& name);

private:
    WeatherPresetManager& presetManager_;
};

} // namespace bws::weather

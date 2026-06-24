// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#include "bw_ui/preset_system/ManagePresetsController.h"

namespace bws::weather
{

ManagePresetsController::ManagePresetsController(WeatherPresetManager& presetManager) noexcept
    : presetManager_(presetManager)
{}

bool ManagePresetsController::shouldEnable() const
{
    return presetManager_.getUserPresetCount() > 0;
}

void ManagePresetsController::onOpen()
{
    if (presetManager_.isPreviewSessionActive())
        presetManager_.cancelPreviewSession();
}

std::vector<ManagePresetsController::UserPresetInfo> ManagePresetsController::listUserPresets() const
{
    std::vector<UserPresetInfo> out;
    for (const auto& preset : presetManager_.getAllPresets())
    {
        if (preset.isFactory)
            continue;
        out.push_back(UserPresetInfo {
            preset.name,
            preset.category,
            preset.author,
            preset.description,
            preset.presetId,
            false,
        });
    }
    return out;
}

bool ManagePresetsController::requestDelete(const juce::String& name)
{
    return presetManager_.deletePreset(name);
}

ManagePresetsController::RenameResult ManagePresetsController::requestRename(const juce::String& oldName,
                                                                             const juce::String& newName)
{
    const auto trimmed = newName.trim();
    if (trimmed.isEmpty())
        return RenameRejection::Empty;
    if (trimmed == oldName)
        return RenameRejection::Unchanged;

    presetManager_.renamePreset(oldName, trimmed);
    return Ok {};
}

bool ManagePresetsController::requestUpdate(const juce::String& name)
{
    // Contract §B8: preserve the preset's existing category/author/description
    // by re-invoking savePreset with the original metadata. saveUserPreset
    // would force category="User" and orphan presets saved under any other
    // category.
    juce::String category;
    juce::String author;
    juce::String description;
    bool found = false;

    for (const auto& preset : presetManager_.getAllPresets())
    {
        if (preset.isFactory || preset.name != name)
            continue;
        category = preset.category;
        author = preset.author;
        description = preset.description;
        found = true;
        break;
    }

    if (!found)
        return false;

    return presetManager_.savePreset(name, category, author, description);
}

} // namespace bws::weather

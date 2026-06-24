// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

// BarometerEditorPresets.cpp - preset, settings, and preference methods for BarometerEditor

#include "BarometerEditor.h"
#include "ui/BarometerToyOverlay.h"
#include <bw_ui/adapters/UiThemeKernelAdapter.h>
#include <bw_ui/generated/BwTokens.h>
#include <bw_juce_adapters/HostEnvironment.h>
#include <bw_ui/preset_system/WeatherManagePresetsDialog.h>
#include <bw_ui/preset_system/WeatherSavePresetDialog.h>
#include <bw_ui/foundation/UiTheme.h>
#include <array>
#include <vector>

namespace bws::barometer
{

void BarometerEditor::showSettingsMenu()
{
    juce::PopupMenu menu;

    // Tooltips toggle
    menu.addItem(1,
                 (tooltipsEnabled_ ? juce::String::charToString(0x2611) : juce::String::charToString(0x2610)) +
                     " Show Tooltips (Cmd+T)",
                 true, false);

    // Preset directory
    menu.addItem(2, juce::String::charToString(0x1F4C1) + " Preset Directory...");


    menu.addSeparator();

    // Meter interactions info (non-selectable)
    menu.addSectionHeader("Meter Interactions");
    menu.addItem(-1, "  Click IN meter: Solo L", false, false);
    menu.addItem(-1, "  Click OUT meter: Solo R", false, false);
    menu.addItem(-1, "  Cmd+Click meter: Swap L/R", false, false);

    menu.addSeparator();

    // LUFS Target submenu
    juce::PopupMenu targetMenu;
    auto addTargetItem = [&](int itemId, LufsTarget target, const juce::String& label) {
        const bool isCurrent = (lufsTarget_ == target);
        targetMenu.addItem(itemId, (isCurrent ? juce::String::charToString(0x2713) + " " : "  ") + label, true, false);
    };
    addTargetItem(20, LufsTarget::Streaming_14, "Streaming (-14 LUFS)");
    addTargetItem(21, LufsTarget::AppleMusic_16, "Apple Music (-16 LUFS)");
    addTargetItem(22, LufsTarget::Broadcast_23, "Broadcast EBU R128 (-23 LUFS)");
    addTargetItem(23, LufsTarget::Broadcast_24, "Broadcast ATSC A/85 (-24 LUFS)");
    addTargetItem(24, LufsTarget::Off, "Off");
    menu.addSubMenu(juce::String::charToString(0x1F3AF) + " LUFS Target", targetMenu);

    // Reset to defaults
    menu.addItem(4, juce::String::charToString(0x21BA) + " Reset to Defaults");

    menu.addSeparator();

    // About
    menu.addItem(3, "About Barometer");

    // Demo overlay toggle.
    menu.addSeparator();
    menu.addItem(100, juce::String::charToString(0x1F3AE));

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(settingsButton_.get()).withMinimumWidth(180),
                       [this](int result) {
                           if (result == 100)
                           {
                               openSignalLockOverlay();
                               return;
                           }
                           if (result == 1)
                           {
                               setTooltipsEnabled(!tooltipsEnabled_);
                           }
                           else if (result == 2)
                           {
                               showPresetLocation();
                           }
                           else if (result == 3)
                           {
                               bws::weather::AboutConfig cfg;
                               cfg.pluginName = "Barometer";
                               cfg.version = JucePlugin_VersionString;
                               cfg.description = "Precision Gain & Metering Utility\n"
                                                 "by Bellweather Studios\n\n"
                                                 "ITU-R BS.1770-4/5 LUFS | EBU Tech 3342 LRA | 4x True Peak\n"
                                                 "Stereo Correlation | A/B Compare";
                               cfg.manualUrl = "https://www.bellweatherstudios.com/barometer";
                               cfg.licenseStatus = "Free";

                               aboutOverlay_ = std::make_unique<bws::weather::AboutOverlay>(
                                   cfg, [this]() { aboutOverlay_.reset(); });
                               aboutOverlay_->setTheme(getTheme());
                               addAndMakeVisible(*aboutOverlay_);
                               aboutOverlay_->setBounds(getLocalBounds());
                           }
                           else if (result == 4)
                           {
                               // Reset to defaults
                               resetToDefaults();
                           }
                           else if (result >= 20 && result <= 24)
                           {
                               // LUFS target options
                               switch (result)
                               {
                               case 20:
                                   lufsTarget_ = LufsTarget::Streaming_14;
                                   break;
                               case 21:
                                   lufsTarget_ = LufsTarget::AppleMusic_16;
                                   break;
                               case 22:
                                   lufsTarget_ = LufsTarget::Broadcast_23;
                                   break;
                               case 23:
                                   lufsTarget_ = LufsTarget::Broadcast_24;
                                   break;
                               case 24:
                                   lufsTarget_ = LufsTarget::Off;
                                   break;
                               }
                               saveLufsTargetPreference();
                               repaint(lufsReadoutBounds_);
                           }
                       });
}

void BarometerEditor::resetToDefaults()
{
    auto& apvts = processor_.getApvts();

    // Reset all parameters to their default values
    if (auto* param = apvts.getParameter(BarometerProcessor::kGainParamId))
        param->setValueNotifyingHost(param->getDefaultValue());

    if (auto* param = apvts.getParameter(BarometerProcessor::kBalanceParamId))
        param->setValueNotifyingHost(param->getDefaultValue()); // 0.0 = center

    if (auto* param = apvts.getParameter(BarometerProcessor::kWidthParamId))
        param->setValueNotifyingHost(param->getDefaultValue()); // 1.0 = 100%

    if (auto* param = apvts.getParameter(BarometerProcessor::kPhaseInvertLParamId))
        param->setValueNotifyingHost(param->getDefaultValue());

    if (auto* param = apvts.getParameter(BarometerProcessor::kPhaseInvertRParamId))
        param->setValueNotifyingHost(param->getDefaultValue());

    if (auto* param = apvts.getParameter(BarometerProcessor::kPhaseLinkParamId))
        param->setValueNotifyingHost(param->getDefaultValue());

    if (auto* param = apvts.getParameter(BarometerProcessor::kMonoParamId))
        param->setValueNotifyingHost(param->getDefaultValue());

    if (auto* param = apvts.getParameter(BarometerProcessor::kDcFilterParamId))
        param->setValueNotifyingHost(param->getDefaultValue());

    if (auto* param = apvts.getParameter(BarometerProcessor::kSwapLRParamId))
        param->setValueNotifyingHost(param->getDefaultValue());

    if (auto* param = apvts.getParameter(BarometerProcessor::kSoloLParamId))
        param->setValueNotifyingHost(param->getDefaultValue());

    if (auto* param = apvts.getParameter(BarometerProcessor::kSoloRParamId))
        param->setValueNotifyingHost(param->getDefaultValue());

    if (auto* param = apvts.getParameter(BarometerProcessor::kOutputTrimParamId))
        param->setValueNotifyingHost(param->getDefaultValue());

    // Reset Balance/Width mode to Balance
    setBalanceWidthMode(0);

    // Reset preset dropdown to "Default"
    presetCombo_->setSelectedId(1, juce::sendNotification);

    repaint();
}

void BarometerEditor::rebuildPresetCombo()
{
    presetCombo_->clear(juce::dontSendNotification);

    // Factory presets section
    presetCombo_->addSectionHeading("FACTORY PRESETS");
    const auto presetNames = processor_.getPresetNames();
    for (int i = 0; i < presetNames.size(); ++i)
        presetCombo_->addItem(presetNames[i], i + 1); // IDs 1-10

    // User presets section - read directly from the shared facade. Filter
    // !isFactory; the running userIndex matches processor_.getCurrentPresetComboId()'s
    // user-index scheme (both iterate getAllPresets() in order and count
    // user entries only).
    const auto& allPresets = processor_.getPresetManager().getAllPresets();
    bool addedUserHeading = false;
    int userIndex = 0;
    for (const auto& preset : allPresets)
    {
        if (preset.isFactory)
            continue;

        if (!addedUserHeading)
        {
            presetCombo_->addSeparator();
            presetCombo_->addSectionHeading("USER PRESETS");
            addedUserHeading = true;
        }

        if (preset.name.isEmpty())
        {
            DBG("Warning: User preset " << userIndex << " has empty name");
            presetCombo_->addItem("(Unnamed Preset " + juce::String(userIndex + 1) + ")",
                                  kUserPresetIdOffset + userIndex);
        }
        else
        {
            presetCombo_->addItem(preset.name, kUserPresetIdOffset + userIndex);
        }
        ++userIndex;
    }

    // Actions
    presetCombo_->addSeparator();
    presetCombo_->addItem(juce::String::charToString(0x2192) + " Save Current...", kSavePresetId);
    presetCombo_->addItem(juce::String::charToString(0x2192) + " Manage Presets...", kManagePresetsId);
    presetCombo_->addItem(juce::String::charToString(0x2192) + " Show Preset Location...", kShowPresetLocationId);
    presetCombo_->addItem(juce::String::charToString(0x2192) + " Change Preset Location...", kChangePresetLocationId);
}

void BarometerEditor::syncPresetComboSelection()
{
    const int currentPresetId = processor_.getCurrentPresetComboId(kUserPresetIdOffset);
    if (currentPresetId > 0)
    {
        presetCombo_->setSelectedId(currentPresetId, juce::dontSendNotification);
        return;
    }

    presetCombo_->setSelectedId(0, juce::dontSendNotification);
    const auto& currentPreset = processor_.getPresetManager().getCurrentPreset();
    if (currentPreset.name.isNotEmpty())
        presetCombo_->setText(currentPreset.name, juce::dontSendNotification);
}

void BarometerEditor::showSavePresetDialog()
{
    // Primary name/category capture uses the shared styled dialog. The
    // overwrite-confirmation path remains an AlertWindow - modal OS chrome
    // fits the "are you sure?" moment better than the in-editor panel.
    const auto& current = processor_.getPresetManager().getCurrentPreset();
    bws::weather::showSavePresetDialog(
        getTheme(), getScaleFactor(), current.name.isNotEmpty() ? current.name : juce::String("New Preset"),
        // Default category: inherit current preset's category ONLY when it's
        // a user preset. Saving derived-from-factory should default to "User"
        // so customer presets don't file under Bellweather's factory
        // categories (no more "user preset in 'Vocals' group" surprises).
        (current.category.isNotEmpty() && !current.isFactory) ? current.category : juce::String("User"),
        [this](const juce::String& name, const juce::String& category) {
            const auto resolvedCategory = category.isNotEmpty() ? category : juce::String("User");
            // Identity match here must mirror the facade's overwrite rule
            // (name + category per weather-preset-manager.md §B2), not just
            // name - otherwise typing an existing name with a different
            // category shows an overwrite prompt but produces a duplicate
            // instead (no matching target in the overwrite loop).
            const auto& allPresets = processor_.getPresetManager().getAllPresets();
            const bool exists = std::any_of(allPresets.begin(), allPresets.end(), [&](const auto& p) {
                return !p.isFactory && p.name == name && p.category == resolvedCategory;
            });
            if (exists)
            {
                juce::AlertWindow::showOkCancelBox(
                    juce::MessageBoxIconType::WarningIcon, "Overwrite Preset?",
                    "A preset named '" + name + "' already exists. Overwrite it?", "Overwrite", "Cancel", nullptr,
                    juce::ModalCallbackFunction::create([this, name, resolvedCategory](int overwriteResult) {
                        if (overwriteResult == 1)
                        {
                            // savePreset(name, category) performs overwrite-by-identity per
                            // weather-preset-manager.md §B2 AND preserves display metadata
                            // (engine/intensity/tags) per §B2 inheritance rules.
                            processor_.getPresetManager().savePreset(name, resolvedCategory);
                            // UI refresh via weatherPresetListChanged listener.
                        }
                    }));
            }
            else
            {
                processor_.getPresetManager().savePreset(name, resolvedCategory);
                // UI refresh via weatherPresetListChanged listener.
            }
        },
        getTopLevelComponent(), bws::adapters::hostInterceptsPluginKeyboardInput());
}

void BarometerEditor::showPresetLocation()
{
    auto presetDir = processor_.getPresetDirectory();

    // Create directory if it doesn't exist
    if (!presetDir.exists())
    {
        presetDir.createDirectory();
    }

    // Open in Finder/Explorer
    presetDir.revealToUser();
}

void BarometerEditor::showChangePresetLocationDialog()
{
    // Build info message showing current location
    const auto currentDir = processor_.getPresetDirectory();
    const bool isCustom = processor_.hasCustomPresetDirectory();

    juce::String message = "Current location:\n" + currentDir.getFullPathName();
    if (isCustom)
        message += "\n\n(Using custom location)";
    else
        message += "\n\n(Using default location)";

    auto* alertWindow = new juce::AlertWindow("Preset Library Location", message, juce::MessageBoxIconType::NoIcon);

    alertWindow->addButton("Choose New Location...", 2);
    if (isCustom)
        alertWindow->addButton("Reset to Default", 1);
    alertWindow->addButton("Cancel", 0);

    alertWindow->enterModalState(
        true, juce::ModalCallbackFunction::create([this, alertWindow, isCustom](int result) {
            if (result == 2)
            {
                // Choose new location
                auto chooser = std::make_shared<juce::FileChooser>(
                    "Choose Preset Library Location",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "", true);

                chooser->launchAsync(juce::FileBrowserComponent::openMode |
                                         juce::FileBrowserComponent::canSelectDirectories,
                                     [this, chooser](const juce::FileChooser& fc) {
                                         auto results = fc.getResults();
                                         if (results.size() > 0)
                                         {
                                             auto newDir = results[0].getChildFile("Barometer Presets");
                                             processor_.setCustomPresetDirectory(newDir);
                                             // UI refresh via weatherPresetListChanged listener
                                             // (setCustomPresetDirectory → refreshPresetList →
                                             // notifyPresetListChanged).

                                             juce::AlertWindow::showMessageBoxAsync(
                                                 juce::MessageBoxIconType::InfoIcon, "Presets Relocated",
                                                 "Presets have been moved to:\n" + newDir.getFullPathName());
                                         }
                                     });
            }
            else if (result == 1 && isCustom)
            {
                // Reset to default
                processor_.clearCustomPresetDirectory();
                // UI refresh via weatherPresetListChanged listener.

                juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon, "Reset to Default",
                                                       "Preset library reset to default location:\n" +
                                                           processor_.getPresetDirectory().getFullPathName());
            }
            delete alertWindow;
        }),
        true);
}

void BarometerEditor::showManagePresetsDialog()
{
    // Routes through the shared dialog in bw_ui. Post-mutation refreshes come
    // via the listener overrides below - no manual rebuild/sync needed here.
    bws::weather::showManagePresetsDialog(processor_.getPresetManager(), getTheme(), getScaleFactor(),
                                          getTopLevelComponent());
}

void BarometerEditor::weatherPresetChanged()
{
    syncPresetComboSelection();
}

void BarometerEditor::weatherPresetListChanged()
{
    rebuildPresetCombo();
    // Save / update / rename all fire listChanged (not presetChanged), yet
    // currentPreset_ has been updated on the manager side. Keep the combo's
    // selection in lockstep so the UI reflects the new current preset.
    syncPresetComboSelection();
}

void BarometerEditor::setTooltipsEnabled(bool enabled)
{
    tooltipsEnabled_ = enabled;

    if (tooltipsEnabled_)
    {
        // Create tooltip window if it doesn't exist
        if (!tooltipWindow_)
            tooltipWindow_ = std::make_unique<juce::TooltipWindow>(this);
    }
    else
    {
        // Destroy tooltip window to disable tooltips
        tooltipWindow_.reset();
    }

    saveTooltipPreference();
}

void BarometerEditor::saveTooltipPreference()
{
    if (auto* props = getPropertiesFile())
    {
        props->setValue("tooltipsEnabled", tooltipsEnabled_);
        props->saveIfNeeded();
    }
}

void BarometerEditor::loadTooltipPreference()
{
    if (auto* props = getPropertiesFile())
    {
        tooltipsEnabled_ = props->getBoolValue("tooltipsEnabled", true); // Default: enabled
    }

    // Create tooltip window if enabled
    if (tooltipsEnabled_)
        tooltipWindow_ = std::make_unique<juce::TooltipWindow>(this);
}

float BarometerEditor::getLufsTargetValue() const
{
    switch (lufsTarget_)
    {
    case LufsTarget::Streaming_14:
        return -14.0f;
    case LufsTarget::AppleMusic_16:
        return -16.0f;
    case LufsTarget::Broadcast_23:
        return -23.0f;
    case LufsTarget::Broadcast_24:
        return -24.0f;
    case LufsTarget::Off:
        return 0.0f;
    }
    return -14.0f;
}

void BarometerEditor::saveLufsTargetPreference()
{
    if (auto* props = getPropertiesFile())
    {
        props->setValue("lufsTarget", static_cast<int>(lufsTarget_));
        props->saveIfNeeded();
    }
}

void BarometerEditor::loadLufsTargetPreference()
{
    if (auto* props = getPropertiesFile())
    {
        const int stored = props->getIntValue("lufsTarget", static_cast<int>(LufsTarget::Streaming_14));
        if (stored >= 0 && stored <= 4)
            lufsTarget_ = static_cast<LufsTarget>(stored);
    }
}


} // namespace bws::barometer

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "bw_ui/preset_system/WeatherPresetManager.h"

#include <bw_preset_core/PresetMetadataPolicy.h>
#include <bw_preset_core/PresetRecord.h>

#include <optional>
#include <string>
#include <string_view>

namespace bws::weather
{

[[nodiscard]] juce::String juceStringFromStdString(const std::string& value);

[[nodiscard]] juce::String presetCategoryOrDefault(const juce::String& category,
                                                   std::string_view fallback = bws::preset::kDefaultPresetCategory);

[[nodiscard]] juce::String presetAuthorOrDefault(const juce::String& author, bws::preset::PresetSource source);

[[nodiscard]] juce::String intensityToString(WeatherPresetManager::Preset::Intensity intensity);

[[nodiscard]] bws::preset::PresetFormat toCorePresetFormat(WeatherPresetManager::Preset::Format format);

[[nodiscard]] bws::preset::PresetIntensity toCorePresetIntensity(WeatherPresetManager::Preset::Intensity intensity);

[[nodiscard]] WeatherPresetManager::Preset::Format fromCorePresetFormat(bws::preset::PresetFormat format);

[[nodiscard]] WeatherPresetManager::Preset::Intensity fromCorePresetIntensity(bws::preset::PresetIntensity intensity);

[[nodiscard]] bws::preset::PresetStateBytes stateBytesFromProcessorState(juce::AudioProcessor& processor);

[[nodiscard]] juce::String base64FromStateBytes(const bws::preset::PresetStateBytes& state);

[[nodiscard]] juce::ValueTree makePresetStateTree(const juce::String& pluginName, const juce::String& name,
                                                  const juce::String& category, const juce::String& author,
                                                  const juce::String& description, const juce::Time& timestamp,
                                                  const juce::String& stateBlob, bool isFactory, int sortOrder);

[[nodiscard]] bws::preset::PresetMetadata toCorePresetMetadata(const juce::String& pluginName,
                                                               const WeatherPresetManager::Preset& preset);

[[nodiscard]] bws::preset::PresetRecord toCorePresetRecord(const juce::String& pluginName,
                                                           const WeatherPresetManager::Preset& preset,
                                                           bws::preset::PresetStateBytes state);

[[nodiscard]] std::optional<WeatherPresetManager::Preset> fromCorePresetRecord(const juce::String& pluginName,
                                                                               const bws::preset::PresetRecord& record);

} // namespace bws::weather

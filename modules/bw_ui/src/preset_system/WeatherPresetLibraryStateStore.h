// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_preset_core/PresetLibraryState.h>

#include <juce_core/juce_core.h>

#include <optional>

namespace bws::weather
{

[[nodiscard]] std::optional<bws::preset::PresetLibraryState> parsePresetLibraryStateDocument(const juce::var& document);

[[nodiscard]] juce::var makePresetLibraryStateDocument(const bws::preset::PresetLibraryState& state);

} // namespace bws::weather

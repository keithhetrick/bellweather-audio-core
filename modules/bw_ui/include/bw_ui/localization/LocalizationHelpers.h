// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <bw_ui/localization/LocalizationManager.h>

namespace bws::ui::localization
{

/// Compose a knob tooltip: purpose + canonical key-command footer.
/// `defaultValue` empty selects the no-default variant.
juce::String composeKnobTooltip(const LocalizationManager& loc, juce::StringRef purposeKey,
                                juce::StringRef purposeFallback, const juce::String& defaultValue = {});

} // namespace bws::ui::localization

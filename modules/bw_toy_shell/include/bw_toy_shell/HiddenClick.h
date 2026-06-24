// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace bws::toy_shell
{

/// Returns true if the given modifier-key state holds Alt/Option. This is the
/// pure-logic helper - testable with synthesized juce::ModifierKeys fixtures
/// without needing a juce::MouseEvent or MessageManager.
[[nodiscard]] bool hasHiddenTriggerModifier(juce::ModifierKeys mods) noexcept;

/// Returns true if this mouse event is a "hidden trigger" gesture
/// (Alt/Option modifier held). Thin wrapper over hasHiddenTriggerModifier -
/// the canonical convention for opening an easter-egg overlay from a settings
/// gear: plain click -> settings; Alt+click -> overlay.
///
/// Stateless free function. Callers wire one verdict per gear button.
[[nodiscard]] bool isHiddenTriggerClick(const juce::MouseEvent& event) noexcept;

} // namespace bws::toy_shell

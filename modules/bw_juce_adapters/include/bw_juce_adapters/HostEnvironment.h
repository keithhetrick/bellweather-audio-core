// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

/**
 * @file HostEnvironment.h
 * @brief Host-behavior queries for plugin adapters.
 *
 * Pure-C++ interface; JUCE stays inside the .cpp. First entry in what may
 * grow into a larger host-environment seam (dpi handling, modal-stacking
 * rules, etc.) if a polymorphic `IHostEnvironment` is later justified.
 *
 *
 * landscape that motivated the first query added here.
 */

namespace bws::adapters
{

/**
 * True when the host intercepts keyboard shortcuts (notably spacebar for
 * transport) at the plugin-window level, preventing child text-input
 * components from receiving them. Such hosts require text-input dialogs to
 * be desktop-promoted (juce::Component::addToDesktop) rather than embedded
 * child components of the plugin editor.
 *
 * Canonical example: Reaper on macOS. Known-good hosts returning false:
 * Ableton Live, Bitwig Studio, FL Studio, AudioPluginHost.
 *
 * Named by behavior rather than host identity so the implementation can
 * grow to cover other hosts (Logic, Pro Tools, Cubase if they're ever
 * confirmed to hijack) without renaming the seam.
 */
[[nodiscard]] bool hostInterceptsPluginKeyboardInput();

} // namespace bws::adapters

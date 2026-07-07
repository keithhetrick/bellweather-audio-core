// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * @file UiDisplaySlotHooks.h
 * @brief Lifecycle-callable bundle for UiDisplayHost slot components.
 *
 * UiDisplayHost (Components/UiDisplayHost.h) is a thin view-switcher shell;
 * the slot components that occupy its named views are deep on their own. This
 * header defines the seam between the shell and its slots: a record of
 * `std::function` callbacks that the host invokes when host-owned axes change
 * (theme, scale, interactivity). Empty fields are silent no-ops, so a slot
 * registration only opts in to the lifecycle hooks it actually needs.
 *
 * Design rationale (B from design exploration):
 *   - No virtual `IDisplaySlot` interface - slot components remain pure
 *     `juce::Component` and don't gain a bw_ui inheritance edge.
 *   - Adding a new fan-out axis is a struct-field addition, not an inheritance
 *     churn across N slots.
 *   - The host stays the single owner of "what a slot is" - locality wins.
 *
 * The "B + F" picks of - composition for slots (this file), tokens
 * for trace tint (UiEnvelopeTokens) - keep the whole envelope-display
 * migration free of inheritance edges between bw_ui and consumers.
 */

#include <bw_ui/foundation/UiTheme.h>
#include <functional>

namespace bws::ui
{

/// Bundle of lifecycle callbacks supplied at slot registration time. Each
/// field defaults to empty; UiDisplayHost honors only those that are set.
struct UiDisplaySlotHooks
{
    /// Called by the host's `propagateTheme()` fan-out. Slot components that
    /// own theme-dependent state should refresh from the supplied pointer.
    /// Pointer is owned externally by the editor; lifetime must outlive the
    /// host's last propagation call.
    std::function<void(const UiThemeResolved*)> setTheme;

    /// Called by `propagateScale()` on DPI / editor-scale changes.
    std::function<void(float)> setScale;

    /// Called by `propagateInteraction()` when the host enters / leaves an
    /// interactive state (e.g., modal-open suppression).
    std::function<void(bool)> setInteractive;

    /// Called by `cancelAllInteractions()` - slot should drop drag state,
    /// dismiss popovers, etc.
    std::function<void()> cancel;
};

} // namespace bws::ui

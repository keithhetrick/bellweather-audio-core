// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_ui/kernel/ModSet.h>

namespace bws::ui::kernel
{

/// Per-knob fine-drag sensitivity. Default 400/1600 (4× ratio) is the
/// canonical convention; ReadoutStrip uses `{300, 1200}` as a documented
// / exception for its 34 px height.
struct FineDragSensitivity
{
    int baseSensitivity = 400;
    int fineSensitivity = 1600;
};

/// What action a mouse event resolves to under the current modifier-key state.
enum class InteractionAction
{
    Normal,      ///< Default drag/click - value changes proportionally to pixel delta.
    FineDrag,    ///< Cmd+drag - value changes at fineSensitivity (4× slower default).
    Reset,       ///< Alt+Click - reset to parameter default.
    TextEntry,   ///< Double-Click (no Cmd) - open text-entry dialog.
    PairedReset, ///< Cmd+Double-Click (opt-in) - reset paired surface (e.g., Input+Output together).
};

/// Per-component opt-in policy. Default-constructed instance is the canonical
/// contract; pass overrides for documented exceptions.
struct InteractionPolicy
{
    /// Sensitivity ratio for drag (default: canonical 400/1600 = 4×).
    FineDragSensitivity sensitivity {};

    /// Whether Cmd+Double-Click triggers PairedReset instead of TextEntry.
    /// Off by default; opt in via setter at the component callsite for surfaces
    /// like Input/Output that share a "reset both together" semantic.
    bool pairedResetOptIn {false};
};

// =============================================================================
// Pure classification with no framework side effects.
// =============================================================================

/// Resolve a mouse-down event to an `InteractionAction`. Pure function;
/// adapter (FineDragModifier.h) converts juce::ModifierKeys to ModSet.
///
/// Decision table:
///     Cmd  | Alt | DoubleClick | pairedResetOptIn | → Action
///     -----+-----+-------------+------------------+--------------
///     0    | 0   | 0           | any              | Normal
///     0    | 1   | 0           | any              | Reset
///     0    | 0   | 1           | any              | TextEntry
///     1    | 0   | 0           | any              | Normal (Cmd-fine applies on drag, not click)
///     1    | 0   | 1           | 1                | PairedReset
///     1    | 0   | 1           | 0                | TextEntry (Cmd ignored when no paired-reset wired)
[[nodiscard]] inline InteractionAction classifyMouseDown(ModSet mods, bool isDoubleClick,
                                                         InteractionPolicy policy = {}) noexcept
{
    if (isDoubleClick)
    {
        if (mods.cmd && policy.pairedResetOptIn)
            return InteractionAction::PairedReset;
        return InteractionAction::TextEntry;
    }

    // Bare Alt only - Pro Tools reserves Ctrl+Alt+Cmd+click for automation-enable.
    if (mods.alt && !mods.cmd && !mods.ctrl && !mods.shift)
        return InteractionAction::Reset;

    return InteractionAction::Normal;
}

/// Resolve a mouse-drag event to an `InteractionAction`. Polled per call (not
/// latched at mouse-down) - so Cmd released mid-drag returns to coarse on the
/// next call. Cmd is the ONLY drag modifier per contract; Shift / Alt / Ctrl
/// (on Mac; Ctrl == Cmd on Win) return `Normal`.
[[nodiscard]] inline InteractionAction classifyMouseDrag(ModSet mods, InteractionPolicy /*policy*/ = {}) noexcept
{
    return mods.cmd ? InteractionAction::FineDrag : InteractionAction::Normal;
}

// =============================================================================
// Pure sensitivity computation - applies to both juce::Slider and custom
// pixel-to-value components (e.g., a custom orb's bespoke mapping).
// =============================================================================

/// Pixel-sensitivity value for the current modifier state. Use with
/// `juce::Slider::setMouseDragSensitivity` via the adapter in FineDragModifier.h.
[[nodiscard]] inline int sensitivityForDrag(ModSet mods, InteractionPolicy policy = {}) noexcept
{
    return classifyMouseDrag(mods, policy) == InteractionAction::FineDrag ? policy.sensitivity.fineSensitivity
                                                                          : policy.sensitivity.baseSensitivity;
}

/// Drag-range multiplier for components with bespoke pixel-to-value mapping
/// (e.g., a custom rotary control that computes its own `dragRange`).
///
/// Returns `fineSensitivity / baseSensitivity` (default 4.0) when Cmd is held,
/// 1.0 otherwise. Multiply your existing dragRange by this value.
[[nodiscard]] inline float dragRangeMultiplier(ModSet mods, InteractionPolicy policy = {}) noexcept
{
    return classifyMouseDrag(mods, policy) == InteractionAction::FineDrag
               ? static_cast<float>(policy.sensitivity.fineSensitivity) /
                     static_cast<float>(policy.sensitivity.baseSensitivity)
               : 1.0f;
}

[[nodiscard]] inline bool isFineGesture(ModSet mods) noexcept
{
    return mods.cmd;
}

/// Shift selects the secondary axis when wired; Cmd then fine-tunes it (Cmd+Shift = fine trim).
[[nodiscard]] inline bool isSecondaryAxisGesture(ModSet mods, bool secondaryAxisWired) noexcept
{
    return secondaryAxisWired && mods.shift;
}

} // namespace bws::ui::kernel

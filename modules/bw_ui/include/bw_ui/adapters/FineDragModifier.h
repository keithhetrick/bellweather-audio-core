// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_ui/kernel/InteractionContract.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace bws::ui::kernel
{

// Boundary converter. Kernel expects ModSet; adapter
// callsites convert here.
[[nodiscard]] inline ModSet fromJuce(juce::ModifierKeys m) noexcept
{
    return ModSet {
        .cmd = m.isCommandDown(),
        .alt = m.isAltDown(),
        .shift = m.isShiftDown(),
        .ctrl = m.isCtrlDown(),
    };
}

/// Apply the fine-drag convention to a `juce::Slider` based on the current
/// modifier-key state. Must be called from the slider's `mouseDrag` override
/// (polled per-tick - see JUCE forum t/37730 for the latch-vs-poll rationale).
///
/// Side effect: calls `slider.setMouseDragSensitivity(...)`.
inline void applyFineDragModifier(juce::Slider& slider, juce::ModifierKeys mods, FineDragSensitivity sens = {}) noexcept
{
    const InteractionPolicy policy {.sensitivity = sens};
    slider.setMouseDragSensitivity(sensitivityForDrag(fromJuce(mods), policy));
}

/// Drag-range multiplier overload for components that compute their own
/// pixel-to-value mapping (e.g., a custom orb's bespoke
/// `dragRange = height * 0.6f * resistance` formula). Returns the ratio
/// `fineSensitivity / baseSensitivity` (default 4.0) when Cmd is held,
/// 1.0 otherwise.
inline float fineDragRangeMultiplier(juce::ModifierKeys mods, FineDragSensitivity sens = {}) noexcept
{
    const InteractionPolicy policy {.sensitivity = sens};
    return dragRangeMultiplier(fromJuce(mods), policy);
}

// =============================================================================
// Full-contract adapters - combine drag sensitivity + click action dispatch.
// =============================================================================

/// Compose the full interaction contract for a `juce::Slider`-derived component:
/// drag sensitivity is set via `setMouseDragSensitivity`, and click actions
/// (Reset / TextEntry / PairedReset) are dispatched to the supplied callbacks.
///
/// Returns `true` if the event was consumed.
inline bool applySliderInteraction(juce::Slider& slider, juce::ModifierKeys mods, bool isDoubleClick,
                                   InteractionPolicy policy, const std::function<void()>& onReset,
                                   const std::function<void()>& onTextEntry, const std::function<void()>& onPairedReset)
{
    const ModSet ms = fromJuce(mods);
    slider.setMouseDragSensitivity(sensitivityForDrag(ms, policy));

    switch (classifyMouseDown(ms, isDoubleClick, policy))
    {
    case InteractionAction::Reset:
        if (onReset)
        {
            onReset();
            return true;
        }
        return false;

    case InteractionAction::TextEntry:
        if (onTextEntry)
        {
            onTextEntry();
            return true;
        }
        return false;

    case InteractionAction::PairedReset:
        if (onPairedReset)
        {
            onPairedReset();
            return true;
        }
        return false;

    case InteractionAction::FineDrag:
    case InteractionAction::Normal:
    default:
        return false;
    }
}

/// Resolved interaction contract for custom (non-`juce::Slider`) components
/// that compute their own pixel-to-value mapping. The
/// returned struct lets the component decide locally how to apply the
/// multiplier (e.g., multiply into a bespoke `dragRange`).
struct CustomInteractionContract
{
    float dragRangeMultiplier;
    InteractionAction action;
};

[[nodiscard]] inline CustomInteractionContract resolveCustomInteraction(juce::ModifierKeys mods, bool isDoubleClick,
                                                                        InteractionPolicy policy = {}) noexcept
{
    const ModSet ms = fromJuce(mods);
    return CustomInteractionContract {
        .dragRangeMultiplier = dragRangeMultiplier(ms, policy),
        .action = classifyMouseDown(ms, isDoubleClick, policy),
    };
}

} // namespace bws::ui::kernel

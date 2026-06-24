// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace bws::ui::kernel
{
enum class AvailabilityState
{
    Enabled,
    Disabled,
    Locked,
    ReadOnly
};

struct ControlState
{
    AvailabilityState availability {AvailabilityState::Enabled};
    bool hovered {false};
    bool pressed {false};
    bool focused {false};
    bool selected {false};
};

[[nodiscard]] constexpr bool isInteractive(AvailabilityState availability) noexcept
{
    return availability == AvailabilityState::Enabled;
}

[[nodiscard]] constexpr bool allowsHover(AvailabilityState availability) noexcept
{
    return availability != AvailabilityState::Disabled;
}

[[nodiscard]] constexpr bool allowsFocus(AvailabilityState availability) noexcept
{
    return availability != AvailabilityState::Disabled;
}

[[nodiscard]] constexpr ControlState resolveControlState(ControlState state) noexcept
{
    if (!allowsHover(state.availability))
    {
        state.hovered = false;
        state.focused = false;
    }

    if (!isInteractive(state.availability))
        state.pressed = false;

    return state;
}
} // namespace bws::ui::kernel

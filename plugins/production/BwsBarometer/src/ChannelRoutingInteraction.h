// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace bws::barometer
{

enum class ChannelRoutingAction
{
    SoloLeft,
    SwapLeftRight,
    SoloRight,
};

struct ChannelRoutingState
{
    bool soloLeft {};
    bool swapLeftRight {};
    bool soloRight {};

    friend constexpr bool operator==(const ChannelRoutingState& lhs, const ChannelRoutingState& rhs) noexcept
    {
        return lhs.soloLeft == rhs.soloLeft && lhs.swapLeftRight == rhs.swapLeftRight && lhs.soloRight == rhs.soloRight;
    }
};

// BWS-BAROMETER-ROUTING-1: the complete user-action policy. Host/preset
// restoration deliberately bypasses this function so the legacy both-Solo
// passthrough state remains representable.
[[nodiscard]] constexpr ChannelRoutingState applyChannelRoutingUserAction(ChannelRoutingState state,
                                                                          ChannelRoutingAction action) noexcept
{
    switch (action)
    {
    case ChannelRoutingAction::SoloLeft:
        if (state.soloLeft)
            state.soloLeft = false;
        else
        {
            state.soloRight = false;
            state.soloLeft = true;
        }
        break;
    case ChannelRoutingAction::SwapLeftRight:
        state.swapLeftRight = !state.swapLeftRight;
        break;
    case ChannelRoutingAction::SoloRight:
        if (state.soloRight)
            state.soloRight = false;
        else
        {
            state.soloLeft = false;
            state.soloRight = true;
        }
        break;
    }
    return state;
}

} // namespace bws::barometer

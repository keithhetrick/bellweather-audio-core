// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "ChannelRoutingBinding.h"

namespace bws::barometer
{
namespace
{
juce::RangedAudioParameter& requireParameter(juce::AudioProcessorValueTreeState& state, const char* id)
{
    auto* parameter = state.getParameter(id);
    jassert(parameter != nullptr);
    return *parameter;
}
} // namespace

ChannelRoutingBinding::ChannelRoutingBinding(juce::AudioProcessorValueTreeState& state, bws::ui::UiToggle& soloLeft,
                                             bws::ui::UiToggle& swapLeftRight, bws::ui::UiToggle& soloRight,
                                             bws::ui::TooltipHub* tooltipHub)
    : soloLeftParameter_(requireParameter(state, "soloL"))
    , swapParameter_(requireParameter(state, "swapLR"))
    , soloRightParameter_(requireParameter(state, "soloR"))
    , soloLeft_(soloLeft)
    , swapLeftRight_(swapLeftRight)
    , soloRight_(soloRight)
    , tooltipHub_(tooltipHub)
    , soloLeftAttachment_(
          soloLeftParameter_, [this](float value) { applySoloLeft(value); }, nullptr)
    , soloRightAttachment_(
          soloRightParameter_, [this](float value) { applySoloRight(value); }, nullptr)
    , swapAttachment_(swapParameter_, swapLeftRight, tooltipHub)
{
    soloLeft_.setTitle(soloLeftParameter_.getName(bws::ui::editor::kAccessibleNameLimit));
    soloLeft_.setDescription(soloLeftParameter_.getLabel());
    soloRight_.setTitle(soloRightParameter_.getName(bws::ui::editor::kAccessibleNameLimit));
    soloRight_.setDescription(soloRightParameter_.getLabel());
    soloLeft_.setOnChange([this](bool) { performUserAction(ChannelRoutingAction::SoloLeft); });
    soloRight_.setOnChange([this](bool) { performUserAction(ChannelRoutingAction::SoloRight); });
    if (tooltipHub_ != nullptr)
    {
        tooltipHub_->registerControl(soloLeft_, soloLeftParameter_.getParameterID());
        tooltipHub_->registerControl(soloRight_, soloRightParameter_.getParameterID());
    }
    soloLeftAttachment_.sendInitialUpdate();
    soloRightAttachment_.sendInitialUpdate();
}

ChannelRoutingBinding::~ChannelRoutingBinding()
{
    soloLeft_.setOnChange(nullptr);
    soloRight_.setOnChange(nullptr);
    if (tooltipHub_ != nullptr)
    {
        tooltipHub_->unregisterControl(soloLeft_);
        tooltipHub_->unregisterControl(soloRight_);
    }
}

void ChannelRoutingBinding::performUserAction(ChannelRoutingAction action)
{
    if (!stereoAvailable_)
        return;

    const ChannelRoutingState current {soloLeftParameter_.getValue() >= 0.5f, swapParameter_.getValue() >= 0.5f,
                                       soloRightParameter_.getValue() >= 0.5f};
    const auto next = applyChannelRoutingUserAction(current, action);

    if (action == ChannelRoutingAction::SwapLeftRight)
    {
        if (next.swapLeftRight != current.swapLeftRight)
        {
            swapParameter_.beginChangeGesture();
            swapParameter_.setValueNotifyingHost(next.swapLeftRight ? 1.0f : 0.0f);
            swapParameter_.endChangeGesture();
        }
        return;
    }

    // Peer-off precedes target-on. Turning an already-active side off changes
    // only that side, including the host-restored both-on compatibility state.
    if (action == ChannelRoutingAction::SoloLeft)
    {
        if (next.soloRight != current.soloRight)
            soloRightAttachment_.setValueAsCompleteGesture(next.soloRight ? 1.0f : 0.0f);
        if (next.soloLeft != current.soloLeft)
            soloLeftAttachment_.setValueAsCompleteGesture(next.soloLeft ? 1.0f : 0.0f);
    }
    else
    {
        if (next.soloLeft != current.soloLeft)
            soloLeftAttachment_.setValueAsCompleteGesture(next.soloLeft ? 1.0f : 0.0f);
        if (next.soloRight != current.soloRight)
            soloRightAttachment_.setValueAsCompleteGesture(next.soloRight ? 1.0f : 0.0f);
    }
}

void ChannelRoutingBinding::setStereoAvailable(bool available)
{
    stereoAvailable_ = available;
    soloLeft_.setDisabled(!available);
    swapLeftRight_.setDisabled(!available);
    soloRight_.setDisabled(!available);
}

void ChannelRoutingBinding::applySoloLeft(float value)
{
    jassert(juce::MessageManager::existsAndIsCurrentThread());
    soloLeft_.setValue(value >= 0.5f);
}

void ChannelRoutingBinding::applySoloRight(float value)
{
    jassert(juce::MessageManager::existsAndIsCurrentThread());
    soloRight_.setValue(value >= 0.5f);
}

} // namespace bws::barometer

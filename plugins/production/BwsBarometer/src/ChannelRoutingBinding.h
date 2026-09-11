// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ChannelRoutingInteraction.h"

#include <bw_ui/Components/TooltipHub.h>
#include <bw_ui/Components/UiToggle.h>
#include <bw_ui/Editor/ControlAttachments.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace bws::barometer
{

// Concrete JUCE/APVTS adapter for BWS-BAROMETER-ROUTING-1. The coupled Solo
// controls share one mutator; Swap retains the canonical independent binding.
class ChannelRoutingBinding
{
public:
    ChannelRoutingBinding(juce::AudioProcessorValueTreeState& state, bws::ui::UiToggle& soloLeft,
                          bws::ui::UiToggle& swapLeftRight, bws::ui::UiToggle& soloRight,
                          bws::ui::TooltipHub* tooltipHub);
    ~ChannelRoutingBinding();

    void performUserAction(ChannelRoutingAction action);
    void setStereoAvailable(bool available);

private:
    void applySoloLeft(float value);
    void applySoloRight(float value);

    juce::RangedAudioParameter& soloLeftParameter_;
    juce::RangedAudioParameter& swapParameter_;
    juce::RangedAudioParameter& soloRightParameter_;
    bws::ui::UiToggle& soloLeft_;
    bws::ui::UiToggle& swapLeftRight_;
    bws::ui::UiToggle& soloRight_;
    bws::ui::TooltipHub* tooltipHub_ {};
    bool stereoAvailable_ {true};
    juce::ParameterAttachment soloLeftAttachment_;
    juce::ParameterAttachment soloRightAttachment_;
    bws::ui::editor::UiToggleAttachment swapAttachment_;
};

} // namespace bws::barometer

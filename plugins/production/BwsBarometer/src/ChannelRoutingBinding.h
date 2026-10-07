// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ChannelRoutingInteraction.h"

#include <bw_ui/Components/TooltipHub.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace bws::barometer
{

// Concrete JUCE/APVTS adapter for BWS-BAROMETER-ROUTING-1. The coupled Solo
// controls share one mutator; Swap retains the canonical independent binding.
class ChannelRoutingBinding
{
public:
    ChannelRoutingBinding(juce::AudioProcessorValueTreeState& state, juce::Button& soloLeft,
                          juce::Button& swapLeftRight, juce::Button& soloRight, bws::ui::TooltipHub* tooltipHub);
    ~ChannelRoutingBinding();

    void performUserAction(ChannelRoutingAction action);
    void setStereoAvailable(bool available);

private:
    void applySoloLeft(float value);
    void applySwap(float value);
    void applySoloRight(float value);

    juce::RangedAudioParameter& soloLeftParameter_;
    juce::RangedAudioParameter& swapParameter_;
    juce::RangedAudioParameter& soloRightParameter_;
    juce::Button& soloLeft_;
    juce::Button& swapLeftRight_;
    juce::Button& soloRight_;
    bws::ui::TooltipHub* tooltipHub_ {};
    bool stereoAvailable_ {true};
    juce::ParameterAttachment soloLeftAttachment_;
    juce::ParameterAttachment swapAttachment_;
    juce::ParameterAttachment soloRightAttachment_;
};

} // namespace bws::barometer

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace bws::adapters
{

enum class PluginChannelLayoutPolicy
{
    StereoOnly = 0,
    MatchedMonoStereo = 1,
    MatchedMonoStereoSidechain = 2,
    MonoToStereo = 3
};

PluginChannelLayoutPolicy pluginChannelLayoutPolicyFromInt(int value) noexcept;
bool isBusesLayoutSupported(PluginChannelLayoutPolicy policy, const juce::AudioProcessor::BusesLayout& layouts);

inline PluginChannelLayoutPolicy currentPluginChannelLayoutPolicy() noexcept
{
#if defined(BWS_PLUGIN_CHANNEL_LAYOUT_POLICY)
    return pluginChannelLayoutPolicyFromInt(BWS_PLUGIN_CHANNEL_LAYOUT_POLICY);
#else
    return PluginChannelLayoutPolicy::StereoOnly;
#endif
}

} // namespace bws::adapters

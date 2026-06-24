// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_juce_adapters/PluginBusLayoutPolicy.h"

namespace bws::adapters
{
namespace
{

bool isMono(const juce::AudioChannelSet& set)
{
    return set == juce::AudioChannelSet::mono();
}

bool isStereo(const juce::AudioChannelSet& set)
{
    return set == juce::AudioChannelSet::stereo();
}

bool isMonoOrStereo(const juce::AudioChannelSet& set)
{
    return isMono(set) || isStereo(set);
}

bool isMatchedMonoStereoMain(const juce::AudioProcessor::BusesLayout& layouts)
{
    const auto& mainIn = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();
    return isMonoOrStereo(mainIn) && mainOut == mainIn;
}

bool isValidOptionalMonoStereoSidechain(const juce::AudioProcessor::BusesLayout& layouts)
{
    if (layouts.inputBuses.size() <= 1)
        return true;

    const auto& sidechain = layouts.getChannelSet(true, 1);
    return sidechain.isDisabled() || isMono(sidechain) || isStereo(sidechain);
}

} // namespace

PluginChannelLayoutPolicy pluginChannelLayoutPolicyFromInt(int value) noexcept
{
    switch (value)
    {
    case 1:
        return PluginChannelLayoutPolicy::MatchedMonoStereo;
    case 2:
        return PluginChannelLayoutPolicy::MatchedMonoStereoSidechain;
    case 3:
        return PluginChannelLayoutPolicy::MonoToStereo;
    case 0:
    default:
        return PluginChannelLayoutPolicy::StereoOnly;
    }
}

bool isBusesLayoutSupported(PluginChannelLayoutPolicy policy, const juce::AudioProcessor::BusesLayout& layouts)
{
    const auto& mainIn = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    switch (policy)
    {
    case PluginChannelLayoutPolicy::StereoOnly:
        return isStereo(mainIn) && isStereo(mainOut);

    case PluginChannelLayoutPolicy::MatchedMonoStereo:
        return isMatchedMonoStereoMain(layouts);

    case PluginChannelLayoutPolicy::MatchedMonoStereoSidechain:
        return isMatchedMonoStereoMain(layouts) && isValidOptionalMonoStereoSidechain(layouts);

    case PluginChannelLayoutPolicy::MonoToStereo:
        return isMono(mainIn) && isStereo(mainOut);
    }

    return false;
}

} // namespace bws::adapters

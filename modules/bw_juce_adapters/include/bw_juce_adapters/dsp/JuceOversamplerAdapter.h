// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_audio_types/BufferView.h>
#include <bw_juce_adapters/juce_overrides.h> // FEA-suppressed JUCE surface (audio-thread)
#include <cstddef>
#include <utility>

namespace bws::adapters
{

/**
 * Adapter: wraps juce::dsp::Oversampling<float> to satisfy the
 * UpsamplerBlock concept defined in bw_dsp_metering/TruePeakEnforcer.h.
 *
 * This is the only permitted location for juce::dsp::Oversampling
 * in the TruePeakEnforcer path. The enforcer itself remains framework-neutral.
 *
 * Lifetime: the returned channel pointers from processSamplesUp()
 * are views into the oversampler's internal buffer. They remain valid
 * until the next call to processSamplesUp() on the same oversampler.
 * This is safe within a single processBlock scope.
 */
class JuceOversamplerAdapter
{
public:
    explicit JuceOversamplerAdapter(juce::dsp::Oversampling<float>& oversampler) noexcept
        : oversampler_(oversampler)
    {}

    /**
     * Upsample the buffer. After calling, getChannelPointer/getNumChannels/
     * getNumSamples return the upsampled data.
     */
    void processSamplesUp(bws::domain::BufferView buf) noexcept
    {
        block_ = juce::dsp::AudioBlock<float>(buf.channels, static_cast<std::size_t>(buf.numChannels),
                                              static_cast<std::size_t>(buf.numSamples));
        upBlock_ = oversampler_.processSamplesUp(block_);
    }

    [[nodiscard]] const float* getChannelPointer(std::size_t ch) const noexcept
    {
        return upBlock_.getChannelPointer(ch);
    }

    [[nodiscard]] std::size_t getNumChannels() const noexcept { return upBlock_.getNumChannels(); }

    [[nodiscard]] std::size_t getNumSamples() const noexcept { return upBlock_.getNumSamples(); }

private:
    juce::dsp::Oversampling<float>& oversampler_;
    juce::dsp::AudioBlock<float> block_;
    juce::dsp::AudioBlock<float> upBlock_;
};

} // namespace bws::adapters

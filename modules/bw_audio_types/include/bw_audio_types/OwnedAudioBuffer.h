// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <vector>
#include <cstring>
#include <cassert>
#include "bw_audio_types/BufferView.h"
#include "bw_audio_types/ConstBufferView.h"

namespace bws::domain
{

/**
 * Owning multichannel float audio buffer for DSP modules.
 *
 * Allocation policy:
 *   - setSize() is the ONLY allocation point. Call from prepare(), never from process().
 *   - The 5-arg setSize overload supports avoidReallocating for RT-safe reuse.
 *   - All other methods are RT-safe: no allocation, no virtual dispatch.
 *
 * Thread safety: not thread-safe. Single-thread use only (audio thread).
 *
 * Clang consumes the owner annotation together with BufferView's pointer
 * annotation to catch some dangling-view patterns at compile time.
 */
class BWS_AUDIO_TYPES_GSL_OWNER(float) OwnedAudioBuffer
{
public:
    OwnedAudioBuffer() = default;

    OwnedAudioBuffer(const OwnedAudioBuffer&) = delete;
    OwnedAudioBuffer& operator=(const OwnedAudioBuffer&) = delete;

    OwnedAudioBuffer(OwnedAudioBuffer&&) = default;
    OwnedAudioBuffer& operator=(OwnedAudioBuffer&&) = default;

    //==========================================================================
    // Allocation - call from prepare() only, NOT from process()
    //==========================================================================

    void setSize(int numChannels, int numSamples)
    {
        assert(numChannels >= 0 && numSamples >= 0);
        data_.assign(static_cast<size_t>(numChannels), std::vector<float>(static_cast<size_t>(numSamples), 0.0f));
        rebuildPtrs();
        numChannels_ = numChannels;
        numSamples_ = numSamples;
    }

    // RT-safe overload: avoidReallocating=true skips realloc when current
    // allocation is large enough. keepExistingContent is ignored -
    // std::vector::resize preserves existing elements by default.
    void setSize(int numChannels, int numSamples, bool /*keepExistingContent*/, bool clearExtraSpace,
                 bool avoidReallocating)
    {
        if (avoidReallocating && numChannels == numChannels_ && numSamples <= numSamples_)
        {
            numSamples_ = numSamples;
            if (clearExtraSpace)
                clear();
            return;
        }
        setSize(numChannels, numSamples);
        if (clearExtraSpace)
            clear();
    }

    //==========================================================================
    // Query - RT-safe
    //==========================================================================

    [[nodiscard]] int getNumChannels() const noexcept { return numChannels_; }
    [[nodiscard]] int getNumSamples() const noexcept { return numSamples_; }

    //==========================================================================
    // Access - RT-safe
    //==========================================================================

    [[nodiscard]] float* getWritePointer(int channel) noexcept
    {
        assert(channel >= 0 && channel < numChannels_);
        return data_[static_cast<size_t>(channel)].data();
    }

    [[nodiscard]] const float* getReadPointer(int channel) const noexcept
    {
        assert(channel >= 0 && channel < numChannels_);
        return data_[static_cast<size_t>(channel)].data();
    }

    [[nodiscard]] float* const* getArrayOfWritePointers() noexcept { return ptrs_.data(); }

    [[nodiscard]] float getSample(int channel, int sampleIndex) const noexcept
    {
        assert(channel >= 0 && channel < numChannels_);
        assert(sampleIndex >= 0 && sampleIndex < numSamples_);
        return data_[static_cast<size_t>(channel)][static_cast<size_t>(sampleIndex)];
    }

    void setSample(int channel, int sampleIndex, float value) noexcept
    {
        assert(channel >= 0 && channel < numChannels_);
        assert(sampleIndex >= 0 && sampleIndex < numSamples_);
        data_[static_cast<size_t>(channel)][static_cast<size_t>(sampleIndex)] = value;
    }

    //==========================================================================
    // Operations - RT-safe (no allocation)
    //==========================================================================

    void clear() noexcept
    {
        for (auto& ch : data_)
            std::memset(ch.data(), 0, static_cast<size_t>(numSamples_) * sizeof(float));
    }

    void copyFrom(int destChannel, int destStartSample, const OwnedAudioBuffer& src, int srcChannel, int srcStartSample,
                  int numSamples) noexcept
    {
        assert(destChannel >= 0 && destChannel < numChannels_);
        assert(srcChannel >= 0 && srcChannel < src.numChannels_);
        assert(destStartSample + numSamples <= numSamples_);
        assert(srcStartSample + numSamples <= src.numSamples_);
        std::memcpy(data_[static_cast<size_t>(destChannel)].data() + destStartSample,
                    src.data_[static_cast<size_t>(srcChannel)].data() + srcStartSample,
                    static_cast<size_t>(numSamples) * sizeof(float));
    }

    void copyFrom(int destChannel, int destStartSample, ConstBufferView src, int srcChannel, int srcStartSample,
                  int numSamples) noexcept
    {
        assert(destChannel >= 0 && destChannel < numChannels_);
        assert(destStartSample + numSamples <= numSamples_);
        std::memcpy(data_[static_cast<size_t>(destChannel)].data() + destStartSample,
                    src.channel(srcChannel) + srcStartSample, static_cast<size_t>(numSamples) * sizeof(float));
    }

    void addFrom(int destChannel, int destStartSample, const OwnedAudioBuffer& src, int srcChannel, int srcStartSample,
                 int numSamples, float gainFactor = 1.0f) noexcept
    {
        assert(destChannel >= 0 && destChannel < numChannels_);
        assert(srcChannel >= 0 && srcChannel < src.numChannels_);
        assert(destStartSample + numSamples <= numSamples_);
        assert(srcStartSample + numSamples <= src.numSamples_);
        auto* dest = data_[static_cast<size_t>(destChannel)].data() + destStartSample;
        const auto* srcP = src.data_[static_cast<size_t>(srcChannel)].data() + srcStartSample;
        for (int i = 0; i < numSamples; ++i)
            dest[i] += srcP[i] * gainFactor;
    }

    void addFrom(int destChannel, int destStartSample, ConstBufferView src, int srcChannel, int srcStartSample,
                 int numSamples, float gainFactor = 1.0f) noexcept
    {
        assert(destChannel >= 0 && destChannel < numChannels_);
        assert(destStartSample + numSamples <= numSamples_);
        auto* dest = data_[static_cast<size_t>(destChannel)].data() + destStartSample;
        const auto* srcP = src.channel(srcChannel) + srcStartSample;
        for (int i = 0; i < numSamples; ++i)
            dest[i] += srcP[i] * gainFactor;
    }

    //==========================================================================
    // View - zero-copy non-owning views over this buffer's storage
    //==========================================================================

    [[nodiscard]] BufferView bufferView() noexcept { return {ptrs_.data(), numChannels_, numSamples_}; }

    [[nodiscard]] BufferView bufferView(int numChannels, int numSamples) noexcept
    {
        assert(numChannels >= 0 && numChannels <= numChannels_);
        assert(numSamples >= 0 && numSamples <= numSamples_);
        return {ptrs_.data(), numChannels, numSamples};
    }

    [[nodiscard]] ConstBufferView constBufferView() const noexcept
    {
        return {const_cast<const float* const*>(ptrs_.data()), numChannels_, numSamples_};
    }

    [[nodiscard]] ConstBufferView constBufferView(int numChannels, int numSamples) const noexcept
    {
        assert(numChannels >= 0 && numChannels <= numChannels_);
        assert(numSamples >= 0 && numSamples <= numSamples_);
        return {const_cast<const float* const*>(ptrs_.data()), numChannels, numSamples};
    }

private:
    void rebuildPtrs()
    {
        ptrs_.resize(data_.size());
        for (size_t i = 0; i < data_.size(); ++i)
            ptrs_[i] = data_[i].data();
    }

    std::vector<std::vector<float>> data_;
    std::vector<float*> ptrs_;
    int numChannels_ = 0;
    int numSamples_ = 0;
};

} // namespace bws::domain

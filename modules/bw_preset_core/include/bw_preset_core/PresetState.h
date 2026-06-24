// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_state_types/BwStateBlob.h>
#include <bw_preset_core/PresetIdentity.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace bws::preset
{

struct PresetRestoredMetadata
{
    std::string name;
    std::string category;
    bool modified = false;

    [[nodiscard]] bool hasPresetName() const { return !name.empty(); }
};

class PresetStateBytes
{
public:
    PresetStateBytes() = default;
    explicit PresetStateBytes(std::vector<std::byte> bytes)
        : bytes_(std::move(bytes))
    {}

    PresetStateBytes(const void* data, size_t size)
    {
        if (data == nullptr || size == 0)
            return;

        auto* first = static_cast<const std::byte*>(data);
        bytes_.assign(first, first + size);
    }

    [[nodiscard]] bool empty() const { return bytes_.empty(); }
    [[nodiscard]] size_t size() const { return bytes_.size(); }
    [[nodiscard]] const std::byte* data() const { return bytes_.data(); }
    [[nodiscard]] std::span<const std::byte> span() const { return bytes_; }

    [[nodiscard]] bws::domain::BwStateBlob view() const { return bws::domain::BwStateBlob {span()}; }

    std::vector<std::byte>& bytes() { return bytes_; }
    const std::vector<std::byte>& bytes() const { return bytes_; }

private:
    std::vector<std::byte> bytes_;
};

struct PresetDirtyState
{
    bool storedDirty = false;
    bool pendingDirty = false;
    bool eligible = false;
    uint64_t generation = 0;
    uint64_t pendingGeneration = 0;

    [[nodiscard]] bool hasUnsavedChanges() const
    {
        return storedDirty || (pendingDirty && eligible && pendingGeneration == generation);
    }

    void reset(bool nextEligible, bool nextStoredDirty = false)
    {
        storedDirty = nextStoredDirty;
        pendingDirty = false;
        eligible = nextEligible;
        pendingGeneration = 0;
        ++generation;
    }

    void markPending()
    {
        if (!eligible || pendingDirty)
            return;
        pendingGeneration = generation;
        pendingDirty = true;
    }

    bool flushPending()
    {
        if (!(pendingDirty && eligible && pendingGeneration == generation))
        {
            pendingDirty = false;
            return false;
        }

        pendingDirty = false;
        storedDirty = true;
        return true;
    }
};

} // namespace bws::preset

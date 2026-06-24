// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_rt/RtViolationBuffer.h"
#include <algorithm>
#include <cstring>

namespace bws::rt
{

RtViolationBuffer::RtViolationBuffer() noexcept
{
    clear();
}

void RtViolationBuffer::clear() noexcept
{
    writeIndex.store(0, std::memory_order_relaxed);
    for (auto& v : buffer)
    {
        v.type = ViolationType::Other;
        std::memset(v.message, 0, sizeof(v.message));
    }
}

size_t RtViolationBuffer::size() const noexcept
{
    auto idx = writeIndex.load(std::memory_order_relaxed);
    return idx > kCapacity ? kCapacity : static_cast<size_t>(idx);
}

void RtViolationBuffer::record(ViolationType type, const char* message) noexcept
{
    auto idx = writeIndex.fetch_add(1, std::memory_order_relaxed);
    auto slot = idx % kCapacity;
    buffer[slot].type = type;
    if (message != nullptr)
    {
        const auto len = std::min(std::strlen(message), sizeof(buffer[slot].message) - 1);
        std::memcpy(buffer[slot].message, message, len);
        buffer[slot].message[len] = '\0';
    }
    else
    {
        buffer[slot].message[0] = '\0';
    }
}

std::vector<Violation> RtViolationBuffer::snapshot() const
{
    std::vector<Violation> out;
    auto count = size();
    out.reserve(count);

    auto idx = writeIndex.load(std::memory_order_relaxed);
    auto start = (idx >= count) ? (idx - count) : 0;
    for (size_t i = 0; i < count; ++i)
    {
        auto slot = (start + i) % kCapacity;
        out.push_back(buffer[slot]);
    }
    return out;
}

} // namespace bws::rt

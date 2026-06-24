// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/// @file
/// @brief Lock-free record of real-time-safety violations (alloc/lock) seen on the audio thread.

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <array>
#include <vector>

namespace bws::rt
{

enum class ViolationType : uint8_t
{
    Alloc,
    Lock,
    Other
};

struct Violation
{
    ViolationType type {ViolationType::Other};
    char message[96] {};
};

class RtViolationBuffer
{
public:
    static constexpr size_t kCapacity = 128;

    RtViolationBuffer() noexcept;

    void clear() noexcept;
    size_t size() const noexcept;
    void record(ViolationType type, const char* message) noexcept;

    std::vector<Violation> snapshot() const;

private:
    std::array<Violation, kCapacity> buffer {};
    std::atomic<uint32_t> writeIndex {0};
};

} // namespace bws::rt

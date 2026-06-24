// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>
#include <cstddef>
#include <span>

namespace bws::domain
{

/**
 * Neutral serialization payload crossing the state seam.
 * Replaces framework-specific XML/state objects in persistence signatures.
 *
 * Non-owning. Caller manages backing storage lifetime.
 * version: schema version for forward-compat guards.
 */
struct BwStateBlob
{
    std::span<const std::byte> data;
    uint32_t version = 1;

    [[nodiscard]] bool isEmpty() const noexcept { return data.empty(); }
    [[nodiscard]] size_t size() const noexcept { return data.size(); }
};

} // namespace bws::domain

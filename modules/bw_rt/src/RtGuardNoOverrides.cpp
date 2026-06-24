// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_rt/RtGuard.h"

namespace bws::rt
{
void ensureOverridesLinked() noexcept {}

uint64_t skippedNonHeapDeleteTotal() noexcept
{
    return 0;
}

uint64_t skippedNonHeapDeleteUniqueCount() noexcept
{
    return 0;
}
} // namespace bws::rt

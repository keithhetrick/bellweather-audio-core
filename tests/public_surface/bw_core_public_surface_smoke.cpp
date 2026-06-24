// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include <bw_core/BwCompilerFeatures.h>
#include <bw_core/BwResult.h>
#include <bw_core/BwToUnderlying.h>

#include <cstdint>

enum class SmokeValue : std::uint8_t
{
    Ready = 7
};

int main()
{
    const auto result = bws::domain::BwSuccess(42);
    return result && *result == 42 && bws::domain::to_underlying(SmokeValue::Ready) == 7 ? 0 : 1;
}

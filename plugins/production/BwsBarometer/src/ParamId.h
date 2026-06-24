// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ============================================================================
// BwsBarometer - Framework-Agnostic Parameter Identity
// ============================================================================
//
// Stable numeric IDs for all plugin parameters. These map directly to:
//   VST3:  Vst::ParamID (uint32_t)
//   AU:    AudioUnitParameterID (uint32_t)
//
// APPEND-ONLY: New parameters get the next sequential value.
// Never reorder, rename, or reuse numeric values.
//
// See COMPAT_LEDGER.md for the cross-reference table.
// ============================================================================

#include <array>
#include <cstdint>
#include <string_view>

namespace bws::barometer
{

enum class ParamId : uint32_t
{
    Gain = 0,
    PhaseL = 1,
    PhaseR = 2,
    PhaseLink = 3,
    Balance = 4,
    DcFilter = 5,
    Mono = 6,
    Width = 7,
    SwapLR = 8,
    SoloL = 9,
    SoloR = 10,
    OutTrim = 11,
};

inline constexpr uint32_t kParamCount = 12;

/// Maps ParamId ordinal to APVTS string ID. Append-only - never reorder.
inline constexpr std::array<std::string_view, kParamCount> kParamStringId = {{
    "gain",
    "phaseL",
    "phaseR",
    "phaseLink",
    "balance",
    "dc_filter",
    "mono",
    "width",
    "swapLR",
    "soloL",
    "soloR",
    "outTrim",
}};

} // namespace bws::barometer

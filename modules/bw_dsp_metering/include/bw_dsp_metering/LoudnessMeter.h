// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
//
// Canonical production loudness-meter type alias.
//
// `bws::audio::LoudnessMeter` is the meter that shipping plugins consume. It
// resolves to Bs1770Meter - the ITU-R BS.1770 / EBU R128 loudness meter.
#pragma once

/// @file
/// @brief Canonical production loudness meter (ITU-R BS.1770 / EBU R128); alias to Bs1770Meter.

#include "bw_dsp_metering/Bs1770Meter.h"

namespace bws::audio
{

using LoudnessMeter = Bs1770Meter;

} // namespace bws::audio

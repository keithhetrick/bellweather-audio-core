// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * @file LoudnessRange.h
 * @brief Loudness Range (LRA) statistic per EBU Tech 3342 §3.2
 *
 * Single canonical calculator for the LRA percentile-difference statistic.
 * Consumed by:
 *   - bws::audio::LoudnessMeter::serviceLoudnessRange (production runtime)
 *   - bw_bpts_core LoudnessAnalysis::loudnessRange (test-framework batch analysis)
 *
 * SPEC CONTRACT (EBU Tech 3342 §3.2):
 *   LRA = 95th percentile − 10th percentile of the GATED short-term LUFS
 *   distribution. Caller is responsible for applying both gates BEFORE
 *   calling this helper:
 *     - Absolute gate: ST values below -70 LUFS are excluded (BS.1770-5 §3.1).
 *     - Relative gate: ST values below (energy-domain gated mean − 20 LU)
 *       are excluded (EBU Tech 3342 §3.2.3).
 *   The energy-domain mean (NOT the arithmetic-LUFS mean) is the spec-correct
 *   reference point for the relative gate.
 *
 * RT-SAFETY CONTRACT:
 *   This signature deliberately avoids std::vector<float> as a parameter to
 *   preserve allocation discipline and scratch-buffer reuse. Production callers
 *   run the gating walk and sort from a UI timer or worker thread, never from
 *   the audio callback. Caller pre-allocates a scratch buffer, copies the gated
 *   distribution into it, sorts in-place via std::sort, and passes (data, size)
 *   here.
 *
 * PRE-CONDITION: input is sorted ascending, length n. No defensive sort here -
 * caller controls the sort to allow scratch-buffer reuse.
 *
 * PERCENTILE CONVENTION: round-to-nearest. EBU Tech 3342 defines the percentile
 * statistic but does not prescribe a rounding convention for discrete samples.
 */

#include <algorithm>
#include <cstddef>

namespace bws::audio
{

/**
 * @brief Compute Loudness Range (LRA) per EBU Tech 3342 §3.2.
 *
 * @param sortedDistribution Pointer to gated short-term LUFS values, sorted ascending.
 * @param n Number of values.
 * @return LRA in LU (non-negative). Returns 0.0f for empty distribution (n == 0)
 *         OR null pointer with n > 0 (defensive guard).
 *
 * Percentile indices use round-to-nearest (`+0.5`). EBU Tech 3342 does not
 * prescribe a rounding convention for discrete samples.
 */
inline float computeLoudnessRange(const float* sortedDistribution, std::size_t n) noexcept
{
    if (n == 0 || sortedDistribution == nullptr)
        return 0.0f;

    // Round-to-nearest percentile indices (de-facto convention).
    // For n=10: p10 = round(9 * 0.10 + 0.5) = round(1.4) = 1
    //           p95 = round(9 * 0.95 + 0.5) = round(9.05) = 9
    const float nm1 = static_cast<float>(n - 1);
    const std::size_t idx10 = static_cast<std::size_t>(nm1 * 0.10f + 0.5f);
    const std::size_t idx95 = static_cast<std::size_t>(nm1 * 0.95f + 0.5f);

    const float lra = sortedDistribution[idx95] - sortedDistribution[idx10];
    return std::max(0.0f, lra);
}

} // namespace bws::audio

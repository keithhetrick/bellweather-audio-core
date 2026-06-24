// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <array>

namespace bws::barometer::toy
{

inline constexpr int MAX_RESERVES = 3;

// resetSignalLock preserves bestSeconds and clears everything else
// (phase, frequency, trim, drift, error, timer, reserves, condition, all
// flash timers). Used both for reserve-exhaustion and as the NaN/Inf
// recovery path inside updateSignalLock.

/// Per-step dt is clamped to this. Tab-switches / breakpoint pauses can
/// deliver multi-second gaps; cap at 50ms so a single step can't warp state.
inline constexpr double MAX_STEP_SECONDS = 0.05;

inline constexpr double PHASE_TRIM_SPEED = 2.4;
inline constexpr double FREQ_TRIM_SPEED = 0.8;
inline constexpr double FINE_TRIM_MULTIPLIER = 0.18; ///< Shift slows trim ~5x

inline constexpr double DRIFT_PHASE_AMPL = 1.0;
inline constexpr double DRIFT_FREQ_AMPL = 0.55;
inline constexpr double DRIFT_PHASE_OMEGA_A = 0.27;
inline constexpr double DRIFT_PHASE_OMEGA_B = 0.43;
inline constexpr double DRIFT_FREQ_OMEGA_A = 0.19;
inline constexpr double DRIFT_FREQ_OMEGA_B = 0.31;
inline constexpr double DRIFT_RATE_RAMP = 0.012; ///< drift amp grows ~1.7x over 60s

inline constexpr double ERROR_LOCK_THRESHOLD = 0.12;
inline constexpr double ERROR_DANGER_THRESHOLD = 0.42;
inline constexpr double FREQ_WEIGHT = 0.7; ///< freq vs phase in composite error

inline constexpr double STRESS_RISE_RATE = 1.1; ///< full stress → reserve loss in ~0.9s
inline constexpr double STRESS_FALL_RATE = 0.7;
inline constexpr double RESERVE_GRACE_SECONDS = 0.9;

inline constexpr double TOLERANCE_RAMP = 0.0035;
inline constexpr double TOLERANCE_MIN = 0.04;

inline constexpr double FLASH_RESERVE_LOST = 0.22;
inline constexpr double FLASH_NEW_BEST = 0.32;
inline constexpr double FLASH_LOCK_ENTER = 0.18;
inline constexpr double FLASH_MILESTONE = 0.45;
inline constexpr double FLASH_SOLVED = 1.5;

/// Lock-time thresholds (seconds) that fire milestoneFlash on first crossing.
inline constexpr std::array<double, 5> MILESTONE_TIERS = {5.0, 15.0, 30.0, 60.0, 120.0};

/// Sentinel score set by the Konami solve.
inline constexpr double SOLVED_BEST_SECONDS = 9999.0;

} // namespace bws::barometer::toy

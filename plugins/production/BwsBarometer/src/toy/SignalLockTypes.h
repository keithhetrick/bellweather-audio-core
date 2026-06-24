// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace bws::barometer::toy
{

enum class SignalCondition
{
    Locked,
    Drifting,
    Unstable,
    Lost,
};

struct SignalLockInput
{
    int phaseDirection = 0;     ///< -1 / 0 / +1
    int frequencyDirection = 0; ///< -1 / 0 / +1
    bool fineMode = false;
};

/// Aggregate-initializable POD. Fixed-size; no heap after construction.
struct SignalLockState
{
    double timeSeconds = 0.0;
    double lockSeconds = 0.0;
    double bestSeconds = 0.0;
    int reserves = 0;
    double stress = 0.0; ///< 0..1; rises while error past danger threshold

    // Phase/frequency state - effective values + drift + player trim
    double phase = 0.0;         ///< wrapped to [-pi, pi]
    double frequency = 0.0;     ///< dimensionless offset, ~[-1, 1]
    double phaseTrim = 0.0;     ///< player-integrated, clamped [-pi, pi]
    double frequencyTrim = 0.0; ///< player-integrated, clamped [-1, 1]
    double driftPhase = 0.0;    ///< deterministic drift contribution
    double driftFrequency = 0.0;

    double error = 0.0;      ///< composite alignment magnitude, >= 0
    double lockAmount = 1.0; ///< 0..1, exp(-error / lock-threshold)

    SignalCondition condition = SignalCondition::Locked;
    SignalCondition prevCondition = SignalCondition::Locked;
    double graceSeconds = 0.0;

    // Renderer flash timers - driven by sim events.
    double reserveFlash = 0.0;
    double bestFlash = 0.0;
    double lockFlash = 0.0;
    double milestoneFlash = 0.0;
    double solvedFlash = 0.0;

    bool godMode = false;
    bool solvedFlashIsDisable = false;
};

} // namespace bws::barometer::toy

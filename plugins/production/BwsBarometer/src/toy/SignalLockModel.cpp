// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "SignalLockModel.h"
#include "SignalLockTuning.h"

#include <algorithm>
#include <cmath>

namespace bws::barometer::toy
{

namespace
{

constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;

[[nodiscard]] double wrapToPi(double a) noexcept
{
    double x = a;
    while (x > kPi)
        x -= kTwoPi;
    while (x < -kPi)
        x += kTwoPi;
    return x;
}

} // namespace

double computeError(double phase, double frequency) noexcept
{
    const double phaseN = phase / kPi;
    const double freqW = frequency * FREQ_WEIGHT;
    return std::sqrt((phaseN * phaseN) + (freqW * freqW));
}

SignalCondition deriveCondition(int reserves, double error, double tolerance) noexcept
{
    if (reserves <= 0)
        return SignalCondition::Lost;
    if (error < tolerance)
        return SignalCondition::Locked;
    if (error < ERROR_DANGER_THRESHOLD)
        return SignalCondition::Drifting;
    return SignalCondition::Unstable;
}

SignalLockState createInitialSignalLockState() noexcept
{
    SignalLockState s {};
    s.reserves = MAX_RESERVES;
    s.lockAmount = 1.0;
    s.condition = SignalCondition::Locked;
    s.prevCondition = SignalCondition::Locked;
    return s;
}

void resetSignalLock(SignalLockState& state) noexcept
{
    state.lockSeconds = 0.0;
    state.reserves = MAX_RESERVES;
    state.stress = 0.0;
    state.graceSeconds = RESERVE_GRACE_SECONDS;
    state.phaseTrim = 0.0;
    state.frequencyTrim = 0.0;
    state.timeSeconds = 0.0; // difficulty ramp restarts (else BEST unreachable)
    state.condition = SignalCondition::Drifting;
    state.prevCondition = SignalCondition::Drifting;
    // bestSeconds preserved; flash timers tick down on their own.
}

void solveSignalLock(SignalLockState& state) noexcept
{
    const bool willActivate = !state.godMode;
    state.godMode = willActivate;
    state.solvedFlash = FLASH_SOLVED;
    state.solvedFlashIsDisable = !willActivate;
    if (willActivate)
    {
        state.bestSeconds = SOLVED_BEST_SECONDS;
    }
}

void updateSignalLock(SignalLockState& state, double dt, const SignalLockInput& input) noexcept
{
    const double step = std::clamp(dt, 0.0, MAX_STEP_SECONDS);
    state.timeSeconds += step;

    state.reserveFlash = std::max(0.0, state.reserveFlash - step);
    state.bestFlash = std::max(0.0, state.bestFlash - step);
    state.lockFlash = std::max(0.0, state.lockFlash - step);
    state.milestoneFlash = std::max(0.0, state.milestoneFlash - step);
    state.solvedFlash = std::max(0.0, state.solvedFlash - step);

    const double driftAmpScale = 1.0 + (DRIFT_RATE_RAMP * state.timeSeconds);
    const double t = state.timeSeconds;
    state.driftPhase = DRIFT_PHASE_AMPL * driftAmpScale *
                       ((0.6 * std::sin(t * DRIFT_PHASE_OMEGA_A)) + (0.4 * std::sin((t * DRIFT_PHASE_OMEGA_B) + 1.7)));
    state.driftFrequency =
        DRIFT_FREQ_AMPL * driftAmpScale *
        ((0.5 * std::sin((t * DRIFT_FREQ_OMEGA_A) + 0.8)) + (0.5 * std::cos((t * DRIFT_FREQ_OMEGA_B) * 1.618)));

    if (!state.godMode)
    {
        const double trimMult = input.fineMode ? FINE_TRIM_MULTIPLIER : 1.0;
        state.phaseTrim = std::clamp(
            state.phaseTrim + (static_cast<double>(input.phaseDirection) * PHASE_TRIM_SPEED * trimMult * step), -kPi,
            kPi);
        state.frequencyTrim = std::clamp(
            state.frequencyTrim + (static_cast<double>(input.frequencyDirection) * FREQ_TRIM_SPEED * trimMult * step),
            -1.0, 1.0);
    }

    state.phase = wrapToPi(state.driftPhase + state.phaseTrim);
    state.frequency = state.driftFrequency + state.frequencyTrim;
    state.error = computeError(state.phase, state.frequency);
    state.lockAmount = std::exp(-state.error / ERROR_LOCK_THRESHOLD);

    if (state.godMode)
    {
        state.phase = 0.0;
        state.frequency = 0.0;
        state.error = 0.0;
        state.lockAmount = 1.0;
    }

    const double tolerance =
        std::max(TOLERANCE_MIN, ERROR_LOCK_THRESHOLD * (1.0 - (state.timeSeconds * TOLERANCE_RAMP)));

    if (state.graceSeconds > 0.0)
    {
        state.graceSeconds = std::max(0.0, state.graceSeconds - step);
    }
    else if (state.error < tolerance)
    {
        const double prevLock = state.lockSeconds;
        state.lockSeconds += step;
        state.stress = std::max(0.0, state.stress - (STRESS_FALL_RATE * step));
        for (const double tier : MILESTONE_TIERS)
        {
            if (prevLock < tier && state.lockSeconds >= tier)
            {
                state.milestoneFlash = FLASH_MILESTONE;
                break;
            }
        }
    }
    else if (state.error > ERROR_DANGER_THRESHOLD)
    {
        if (state.godMode)
        {
            state.stress = 0.0;
        }
        else
        {
            state.stress += STRESS_RISE_RATE * step;
            if (state.stress >= 1.0)
            {
                state.reserves -= 1;
                state.stress = 0.0;
                state.graceSeconds = RESERVE_GRACE_SECONDS;
                state.reserveFlash = FLASH_RESERVE_LOST;
                if (state.reserves <= 0)
                {
                    resetSignalLock(state);
                }
            }
        }
    }
    else
    {
        state.stress = std::max(0.0, state.stress - (STRESS_FALL_RATE * step));
    }

    if (state.lockSeconds > state.bestSeconds)
    {
        state.bestSeconds = state.lockSeconds;
        if (state.bestSeconds > 1.0)
        {
            state.bestFlash = FLASH_NEW_BEST;
        }
    }

    state.condition = deriveCondition(state.reserves, state.error, tolerance);
    if (state.condition == SignalCondition::Locked && state.prevCondition != SignalCondition::Locked)
    {
        state.lockFlash = FLASH_LOCK_ENTER;
    }
    state.prevCondition = state.condition;

    // Non-finite recovery: any load-bearing scalar gone wild → reset
    // (bestSeconds preserved). Beats silent NaN propagation tick after tick.
    if (!std::isfinite(state.phase) || !std::isfinite(state.frequency) || !std::isfinite(state.phaseTrim) ||
        !std::isfinite(state.frequencyTrim) || !std::isfinite(state.error) || !std::isfinite(state.lockAmount))
    {
        resetSignalLock(state);
    }
}

} // namespace bws::barometer::toy

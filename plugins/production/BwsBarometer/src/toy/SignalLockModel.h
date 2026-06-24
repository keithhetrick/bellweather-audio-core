// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "SignalLockTypes.h"

namespace bws::barometer::toy
{

[[nodiscard]] SignalLockState createInitialSignalLockState() noexcept;

/// Renderer-agnostic signal-lock model.
void updateSignalLock(SignalLockState& state, double dt, const SignalLockInput& input) noexcept;

/// Full reset on reserve exhaustion. Restores reserves + clears stress/grace/
/// trim/timer, but preserves `bestSeconds`.
void resetSignalLock(SignalLockState& state) noexcept;

/// Konami cheat verdict - jams `bestSeconds` to SOLVED_BEST_SECONDS and fires
/// `solvedFlash`. Doesn't reset the run otherwise.
void solveSignalLock(SignalLockState& state) noexcept;

[[nodiscard]] SignalCondition deriveCondition(int reserves, double error, double tolerance) noexcept;

/// Composite alignment error from normalised phase + weighted frequency.
/// Exposed for testability of the load-bearing invariant: error=0 only when
/// both axes align.
[[nodiscard]] double computeError(double phase, double frequency) noexcept;

} // namespace bws::barometer::toy

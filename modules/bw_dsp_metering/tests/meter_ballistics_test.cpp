// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
// MeterBallistics - dt-invariant UI display easing. Pure C++.
// Contract: first-order exponential smoothing, alpha = 1 - exp(-dt/tau).

#include <bw_dsp_metering/MeterBallistics.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cmath>

using bws::audio::MeterBallistics;
using bws::audio::PeakHold;
using Catch::Approx;

TEST_CASE("onePoleCoeff edge cases", "[meter_ballistics]")
{
    REQUIRE(MeterBallistics::onePoleCoeff(100.0f, 0.0f) == Approx(0.0f)); // dt<=0 holds
    REQUIRE(MeterBallistics::onePoleCoeff(100.0f, -1.0f) == Approx(0.0f));
    REQUIRE(MeterBallistics::onePoleCoeff(0.0f, 0.01f) == Approx(1.0f)); // tau<=0 snaps
    const float a = MeterBallistics::onePoleCoeff(100.0f, 0.01f);
    REQUIRE(a > 0.0f);
    REQUIRE(a < 1.0f);
}

TEST_CASE("easeToward is dt-invariant and matches the closed form", "[meter_ballistics]")
{
    const float timeMs = 80.0f;
    const float tau = timeMs * 0.001f;
    const float state0 = -60.0f;
    const float target = 0.0f;
    const float elapsed = 0.25f;

    // Closed form: state(T) = target + (state0 - target) * exp(-T/tau).
    const float closed = target + ((state0 - target) * std::exp(-elapsed / tau));

    auto run = [&](int steps) {
        float s = state0;
        for (int i = 0; i < steps; ++i)
        {
            s = MeterBallistics::easeToward(s, target, timeMs, elapsed / static_cast<float>(steps));
        }
        return s;
    };

    REQUIRE(run(50) == Approx(closed).epsilon(1e-4));
    REQUIRE(run(50) == Approx(run(100)).epsilon(1e-4)); // N vs 2N - dt-invariant
}

TEST_CASE("easeToward is monotonic and never overshoots", "[meter_ballistics]")
{
    float s = 0.0f;
    const float target = -40.0f;
    float prev = s;
    for (int i = 0; i < 200; ++i)
    {
        s = MeterBallistics::easeToward(s, target, 50.0f, 1.0f / 60.0f);
        REQUIRE(s <= prev);   // monotonic down
        REQUIRE(s >= target); // never overshoots past target
        prev = s;
    }
}

TEST_CASE("settling time follows the time-constant law", "[meter_ballistics]")
{
    const float timeMs = 100.0f;
    const float tau = timeMs * 0.001f;
    const float target = 1.0f;

    auto settledFractionAfter = [&](float seconds) {
        float s = 0.0f;
        const int steps = 2000;
        for (int i = 0; i < steps; ++i)
        {
            s = MeterBallistics::easeToward(s, target, timeMs, seconds / static_cast<float>(steps));
        }
        return s; // fraction of the way to target (target = 1)
    };

    REQUIRE(settledFractionAfter(tau) == Approx(1.0f - std::exp(-1.0f)).epsilon(1e-3));        // ~63%
    REQUIRE(settledFractionAfter(3.0f * tau) == Approx(1.0f - std::exp(-3.0f)).epsilon(1e-3)); // ~95%
    REQUIRE(settledFractionAfter(4.6f * tau) == Approx(1.0f - std::exp(-4.6f)).epsilon(1e-3)); // ~99%
}

TEST_CASE("attackRelease selects the faster constant on the rising edge", "[meter_ballistics]")
{
    const float dt = 1.0f / 60.0f;
    const float riseStep = MeterBallistics::attackRelease(0.0f, 1.0f, /*attack*/ 1.0f, /*release*/ 1000.0f, dt);
    const float fallStep = MeterBallistics::attackRelease(1.0f, 0.0f, /*attack*/ 1.0f, /*release*/ 1000.0f, dt);

    REQUIRE(riseStep == Approx(1.0f).epsilon(1e-3)); // fast attack ≈ snaps up
    REQUIRE(std::abs(1.0f - fallStep) < 0.05f);      // slow release ≈ barely moves
}

TEST_CASE("PeakHold window is measured in seconds, not frames (rate-independent)", "[meter_ballistics]")
{
    // Rule-5 negative: a frame-count hold drifts with rate. After equal elapsed
    // time within the window, the held value must be identical at any clock rate.
    const float holdSeconds = 0.1f;
    const float decayMs = 50.0f;

    // Step counts are exact integers at both rates (elapsed is a multiple of
    // 1/30 and 1/120) so the two runs cover identical wall-time.
    auto heldAfter = [&](float elapsed, float dt) {
        PeakHold h;
        h.update(1.0f, holdSeconds, decayMs, dt); // capture the peak
        const int steps = static_cast<int>(std::lround(elapsed / dt));
        for (int i = 0; i < steps; ++i)
        {
            h.update(0.0f, holdSeconds, decayMs, dt); // live value drops to 0
        }
        return h.value;
    };

    // Inside the window: held at the peak at both 30 Hz and 120 Hz. This is the
    // rate-independence proof - a frame-count hold would already have expired at
    // the faster rate (8 frames elapsed vs a 2-3 frame budget) and dropped below 1.
    REQUIRE(heldAfter(2.0f / 30.0f, 1.0f / 30.0f) == Approx(1.0f));
    REQUIRE(heldAfter(2.0f / 30.0f, 1.0f / 120.0f) == Approx(1.0f));

    // Past the window: both rates track the continuous decay curve (one-pole over
    // the post-hold interval), within one coarse step of expiry quantization.
    const float continuous = std::exp(-(0.2f - holdSeconds) / (decayMs * 0.001f)); // exp(-2) ≈ 0.135
    REQUIRE(heldAfter(0.2f, 1.0f / 30.0f) == Approx(continuous).margin(0.07));
    REQUIRE(heldAfter(0.2f, 1.0f / 120.0f) == Approx(continuous).margin(0.07));
}

TEST_CASE("linearFallToward falls at a fixed rate and clamps at target", "[meter_ballistics]")
{
    // dt<=0 or rate<=0 holds; a target above the state snaps up.
    REQUIRE(MeterBallistics::linearFallToward(0.0f, -10.0f, 20.0f, 0.0f) == Approx(0.0f));
    REQUIRE(MeterBallistics::linearFallToward(0.0f, -10.0f, 0.0f, 0.01f) == Approx(0.0f));
    REQUIRE(MeterBallistics::linearFallToward(-10.0f, -3.0f, 20.0f, 0.01f) == Approx(-3.0f));

    // One step falls exactly rate*dt; clamps so it never crosses the target.
    REQUIRE(MeterBallistics::linearFallToward(0.0f, -100.0f, 20.0f, 0.1f) == Approx(-2.0f));
    REQUIRE(MeterBallistics::linearFallToward(-1.0f, -5.0f, 20.0f, 1.0f) == Approx(-5.0f)); // would reach -21, clamps
}

TEST_CASE("PeakHold linear decay matches the absolute closed form within the decay phase", "[meter_ballistics]")
{
    // Capture at 0 dB with no hold, then fall toward a far floor so the run stays
    // wholly inside the decay phase: value(t) = captured - fallPerSec * t.
    const float fallPerSec = 20.0f;
    const float dt = 1.0f / 60.0f;

    auto valueAfter = [&](int steps) {
        PeakHold h;
        h.updateLinear(0.0f, 0.0f, fallPerSec, dt); // capture
        for (int i = 0; i < steps; ++i)
        {
            h.updateLinear(-100.0f, 0.0f, fallPerSec, dt);
        }
        return h.value;
    };

    const int steps = 30;
    const float elapsed = static_cast<float>(steps) * dt;
    REQUIRE(valueAfter(steps) == Approx(0.0f - (fallPerSec * elapsed)).epsilon(1e-5));
}

TEST_CASE("PeakHold linear decay is dt-invariant within the decay phase", "[meter_ballistics]")
{
    // Linear fall telescopes exactly within one phase: N steps == 2N steps over
    // the same wall-time, as long as neither run crosses the hold boundary or
    // the floor clamp.
    const float fallPerSec = 12.0f;
    const float elapsed = 0.25f;

    auto valueOver = [&](int steps) {
        PeakHold h;
        const float dt = elapsed / static_cast<float>(steps);
        h.updateLinear(0.0f, 0.0f, fallPerSec, dt); // capture
        for (int i = 0; i < steps; ++i)
        {
            h.updateLinear(-100.0f, 0.0f, fallPerSec, dt);
        }
        return h.value;
    };

    REQUIRE(valueOver(40) == Approx(valueOver(80)).epsilon(1e-5));
}

TEST_CASE("PeakHold linear decay crossing the clamp in one dt equals many small steps", "[meter_ballistics]")
{
    // A single large dt that overshoots the floor and many small steps that
    // reach it gradually both saturate at the live target.
    const float fallPerSec = 20.0f;
    const float target = -5.0f;

    PeakHold big;
    big.updateLinear(0.0f, 0.0f, fallPerSec, 1.0f / 60.0f); // capture
    big.updateLinear(target, 0.0f, fallPerSec, 1.0f);       // one 1s step overshoots -> clamps

    PeakHold small;
    small.updateLinear(0.0f, 0.0f, fallPerSec, 1.0f / 60.0f);
    for (int i = 0; i < 60; ++i)
    {
        small.updateLinear(target, 0.0f, fallPerSec, 1.0f / 60.0f);
    }

    REQUIRE(big.value == Approx(target));
    REQUIRE(small.value == Approx(target));
}

TEST_CASE("PeakHold linear hold window is measured in seconds, not frames", "[meter_ballistics]")
{
    // Equal wall-time inside the window holds the peak at any rate; a frame-count
    // hold would expire early at 120 Hz.
    const float holdSeconds = 0.1f;
    const float fallPerSec = 20.0f;

    auto heldAfter = [&](float elapsed, float dt) {
        PeakHold h;
        h.updateLinear(1.0f, holdSeconds, fallPerSec, dt);
        const int steps = static_cast<int>(std::lround(elapsed / dt));
        for (int i = 0; i < steps; ++i)
        {
            h.updateLinear(0.0f, holdSeconds, fallPerSec, dt);
        }
        return h.value;
    };

    REQUIRE(heldAfter(2.0f / 30.0f, 1.0f / 30.0f) == Approx(1.0f));
    REQUIRE(heldAfter(2.0f / 30.0f, 1.0f / 120.0f) == Approx(1.0f));
}

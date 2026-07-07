// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
//
// Accuracy-proof battery for Bs1770Meter. Proves the
// correctness surface beyond the EBU published-absolute-target cases in
// bs1770_meter_test.cpp:
//
//   A. Momentary (400 ms) + short-term (3 s) convergence on steady tones.
//   B. Sample-rate matrix (44.1 / 48 / 88.2 / 96 / 176.4 / 192 kHz): each rate
//      must match the 48 kHz reading of the same tone. At 48 kHz the meter uses
//      the published BS.1770 coefficient table verbatim, so the 48 kHz reading
//      is the spec-correct anchor; non-48k rates exercise the bilinear
//      K-weighting re-derivation against it.
//   C. Calibration-independent structural invariants (M=S=I on steady tones; an
//      exact -X dB amplitude change -> -X LU integrated change; block-spread -> LRA).
//   D. Block-size invariance (incl. the 1-sample block).
//   E. Edge /: silence, all-below-gate, gate-boundary, mono, long
//      duration, reset/epoch, post-silence finiteness (denormal tail).
//   F. Grid quantization: an exact -X dB step reads as a -X LU step within the
//      cell bound.
//
// SPEC CITATIONS: ITU-R BS.1770-5; EBU Tech 3341/3342 v2.0.

#include <bw_dsp_metering/Bs1770Meter.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <array>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

using Catch::Matchers::WithinAbs;

namespace
{
constexpr double kPi = 3.14159265358979323846;

struct PlanarBufferView
{
    const float* const* channelData {nullptr};
    int numChannels {0};
    int numSamples {0};

    int getNumSamples() const noexcept { return numSamples; }
    int getNumChannels() const noexcept { return numChannels; }
    const float* getReadPointer(int ch) const noexcept { return channelData[ch]; }
};

// One channel of a sine at `freqHz`, calibrated so a 1 kHz tone of this peak
// amplitude lands near `lufsTarget` (K ~= 0 dB at 1 kHz). Absolute level is
// approximate; the structural and 48 kHz-anchored tests depend on amplitude
// ratios and rate-to-rate agreement, not this absolute calibration.
std::vector<float> sineChannel(double sampleRate, double seconds, double lufsTarget, double freqHz = 1000.0)
{
    const double amp = std::pow(10.0, (lufsTarget + 0.691) / 20.0);
    const auto n = static_cast<std::size_t>(seconds * sampleRate);
    std::vector<float> out(n, 0.0f);
    const double inc = 2.0 * kPi * freqHz / sampleRate;
    double phase = 0.0;
    for (std::size_t i = 0; i < n; ++i)
    {
        out[i] = static_cast<float>(amp * std::sin(phase));
        phase += inc;
        if (phase > 2.0 * kPi)
            phase -= 2.0 * kPi;
    }
    return out;
}

std::vector<float> silenceChannel(double sampleRate, double seconds)
{
    return std::vector<float>(static_cast<std::size_t>(seconds * sampleRate), 0.0f);
}

template <typename A>
std::vector<float> concatCh(std::vector<float> a, const A& b)
{
    a.insert(a.end(), b.begin(), b.end());
    return a;
}

struct Readout
{
    double integrated {};
    double momentary {};
    double shortTerm {};
    double lra {};
};

template <typename Meter>
Readout measureMeter(const std::vector<std::vector<float>>& planar, double sampleRate, int blockSize)
{
    const int numCh = static_cast<int>(planar.size());
    Meter meter;
    meter.prepare(sampleRate, numCh);

    std::vector<std::vector<float>> buf(static_cast<std::size_t>(numCh),
                                        std::vector<float>(static_cast<std::size_t>(blockSize)));
    std::vector<const float*> ptrs(static_cast<std::size_t>(numCh));

    PlanarBufferView view;
    view.numChannels = numCh;
    view.numSamples = blockSize;

    const std::size_t frames = planar[0].size();
    std::size_t f = 0;
    while (f + static_cast<std::size_t>(blockSize) <= frames)
    {
        for (int c = 0; c < numCh; ++c)
        {
            for (int i = 0; i < blockSize; ++i)
                buf[static_cast<std::size_t>(c)][static_cast<std::size_t>(i)] =
                    planar[static_cast<std::size_t>(c)][f + static_cast<std::size_t>(i)];
            ptrs[static_cast<std::size_t>(c)] = buf[static_cast<std::size_t>(c)].data();
        }
        view.channelData = ptrs.data();
        meter.process(view);
        f += static_cast<std::size_t>(blockSize);
    }
    while (meter.serviceAnalytics())
    {}

    Readout r;
    r.integrated = static_cast<double>(meter.getIntegratedLufs());
    r.momentary = static_cast<double>(meter.getMomentaryLufs());
    r.shortTerm = static_cast<double>(meter.getShortTermLufs());
    r.lra = static_cast<double>(meter.getLoudnessRange());
    return r;
}

// Feed in small blocks, sampling the live momentary/short-term after each block
// and tracking the maxima seen - the EBU "Maximum Momentary / Short-term" metric.
template <typename Meter>
void measureMaxMS(const std::vector<std::vector<float>>& planar, double sampleRate, int blockSize, double& maxMomentary,
                  double& maxShortTerm)
{
    const int numCh = static_cast<int>(planar.size());
    Meter meter;
    meter.prepare(sampleRate, numCh);
    std::vector<std::vector<float>> buf(static_cast<std::size_t>(numCh),
                                        std::vector<float>(static_cast<std::size_t>(blockSize)));
    std::vector<const float*> ptrs(static_cast<std::size_t>(numCh));
    PlanarBufferView view;
    view.numChannels = numCh;
    view.numSamples = blockSize;

    maxMomentary = -1000.0;
    maxShortTerm = -1000.0;
    const std::size_t frames = planar[0].size();
    std::size_t f = 0;
    while (f + static_cast<std::size_t>(blockSize) <= frames)
    {
        for (int c = 0; c < numCh; ++c)
        {
            for (int i = 0; i < blockSize; ++i)
                buf[static_cast<std::size_t>(c)][static_cast<std::size_t>(i)] =
                    planar[static_cast<std::size_t>(c)][f + static_cast<std::size_t>(i)];
            ptrs[static_cast<std::size_t>(c)] = buf[static_cast<std::size_t>(c)].data();
        }
        view.channelData = ptrs.data();
        meter.process(view);
        maxMomentary = std::max(maxMomentary, static_cast<double>(meter.getMomentaryLufs()));
        maxShortTerm = std::max(maxShortTerm, static_cast<double>(meter.getShortTermLufs()));
        f += static_cast<std::size_t>(blockSize);
    }
}

std::vector<std::vector<float>> stereo(std::vector<float> ch)
{
    return {ch, ch};
}

bool isFinite(double v)
{
    return std::isfinite(v);
}

} // namespace

//=============================================================================
// A. Momentary + short-term readouts (the LiveOnly path consumers ship).
//=============================================================================
TEST_CASE("Bs1770Meter momentary and short-term converge on a steady tone",
          "[bs1770][momentary][shortterm][calibration]")
{
    const auto sig = stereo(sineChannel(48000.0, 10.0, -23.0));
    const auto r = measureMeter<bws::audio::Bs1770Meter>(sig, 48000.0, 480);
    INFO("M=" << r.momentary << " S=" << r.shortTerm << " I=" << r.integrated);

    // On a steady tone momentary == short-term == integrated. Calibration-independent.
    REQUIRE_THAT(r.momentary, WithinAbs(r.shortTerm, 0.05));
    REQUIRE_THAT(r.integrated, WithinAbs(r.shortTerm, 0.05));
}

TEST_CASE("Bs1770Meter momentary/short-term track a -10 dB amplitude step exactly",
          "[bs1770][momentary][shortterm][calibration]")
{
    // Same frequency, 10 dB quieter -> exactly -10 LU (K gain cancels).
    const auto loud = measureMeter<bws::audio::Bs1770Meter>(stereo(sineChannel(48000.0, 8.0, -23.0)), 48000.0, 480);
    const auto quiet = measureMeter<bws::audio::Bs1770Meter>(stereo(sineChannel(48000.0, 8.0, -33.0)), 48000.0, 480);
    INFO("loud M=" << loud.momentary << " quiet M=" << quiet.momentary);
    REQUIRE_THAT(loud.momentary - quiet.momentary, WithinAbs(10.0, 0.1));
    REQUIRE_THAT(loud.shortTerm - quiet.shortTerm, WithinAbs(10.0, 0.1));
    REQUIRE_THAT(loud.integrated - quiet.integrated, WithinAbs(10.0, 0.1));
}

//=============================================================================
// B. Sample-rate matrix vs the 48 kHz anchor (inverse-bilinear K-weighting).
//=============================================================================
TEST_CASE("Bs1770Meter integrated matches the 48 kHz anchor across the full sample-rate matrix",
          "[bs1770][samplerate][calibration]")
{
    constexpr std::array<double, 6> kRates {44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0};
    // At 48 kHz the meter uses the published spec table verbatim (identity), so
    // its reading is the spec-correct anchor; a re-derivation error at any other
    // rate shifts that rate off the anchor.
    const double anchor48k =
        measureMeter<bws::audio::Bs1770Meter>(stereo(sineChannel(48000.0, 14.0, -23.0)), 48000.0, 512).integrated;
    for (const double sr : kRates)
    {
        DYNAMIC_SECTION("sr=" << sr)
        {
            const auto sig = stereo(sineChannel(sr, 14.0, -23.0));
            const double ours = measureMeter<bws::audio::Bs1770Meter>(sig, sr, 512).integrated;
            INFO("ours=" << ours << " anchor(48k)=" << anchor48k);
            REQUIRE_THAT(ours, WithinAbs(anchor48k, 0.1));
        }
    }
}

TEST_CASE("Bs1770Meter K-weighting tracks the 48 kHz anchor across frequency at non-48k rates",
          "[bs1770][samplerate][kweighting][calibration]")
{
    // The K curve is frequency-dependent; agreement across freq AND rate proves
    // the re-derived filter shape, not just a single-tone gain match.
    constexpr std::array<double, 3> kRates {44100.0, 96000.0, 192000.0};
    constexpr std::array<double, 4> kFreqs {100.0, 1000.0, 3000.0, 10000.0};
    for (const double f : kFreqs)
    {
        // 48 kHz reading of this tone = the spec-correct anchor (published table).
        const double anchor48k =
            measureMeter<bws::audio::Bs1770Meter>(stereo(sineChannel(48000.0, 10.0, -23.0, f)), 48000.0, 512)
                .integrated;
        // The +4 dB high-shelf warps between rates near 10 kHz (each rate's
        // bilinear map differs close to its own Nyquist); widen the band there only.
        const double tol = (f >= 10000.0) ? 0.3 : 0.1;
        for (const double sr : kRates)
        {
            DYNAMIC_SECTION("sr=" << sr << " f=" << f)
            {
                const auto sig = stereo(sineChannel(sr, 10.0, -23.0, f));
                const double ours = measureMeter<bws::audio::Bs1770Meter>(sig, sr, 512).integrated;
                INFO("ours=" << ours << " anchor(48k)=" << anchor48k << " tol=" << tol);
                REQUIRE_THAT(ours, WithinAbs(anchor48k, tol));
            }
        }
    }
}

//=============================================================================
// C. Structural invariants (calibration-independent correctness).
//=============================================================================
TEST_CASE("Bs1770Meter integrated relative gating excludes far-below blocks", "[bs1770][gating][calibration]")
{
    // EBU Tech 3341 Case 3 proportions: 10 s @ -36, 60 s @ -23, 10 s @ -36. The
    // long loud segment lifts the absolute-gated mean enough that the relative
    // gate (mean - 10 LU) rises ABOVE -36, so the -36 blocks are excluded and
    // integrated == the loud block alone. If the -36 blocks were NOT excluded
    // the result would be ~-24.2 (≈1.9 LU lower), so the ±0.1 band genuinely
    // proves exclusion - not mere energy-dominance (the -36 blocks carry real
    // energy: ~1/20 of the loud blocks across 200 blocks).
    const auto loudOnly =
        measureMeter<bws::audio::Bs1770Meter>(stereo(sineChannel(48000.0, 60.0, -23.0)), 48000.0, 480).integrated;
    const auto seq = stereo(concatCh(concatCh(sineChannel(48000.0, 10.0, -36.0), sineChannel(48000.0, 60.0, -23.0)),
                                     sineChannel(48000.0, 10.0, -36.0)));
    const auto gated = measureMeter<bws::audio::Bs1770Meter>(seq, 48000.0, 480).integrated;
    INFO("loudOnly=" << loudOnly << " gated=" << gated);
    REQUIRE_THAT(gated, WithinAbs(loudOnly, 0.1)); // -36 blocks relative-gated out
}

TEST_CASE("Bs1770Meter LRA reflects the block-loudness spread", "[bs1770][lra][calibration]")
{
    // Two-level program: spread 13 LU (-23 vs -36) and 6 LU (-20 vs -26),
    // both within the -20 LU LRA relative gate.
    // A clean 50/50 two-level program: p95 ~ loud level, p10 ~ quiet level, so LRA
    // ~ the level difference (both within the -20 LU LRA relative gate).
    struct Spread
    {
        double a, b, expect;
    };
    for (const auto s : {Spread {-23.0, -36.0, 13.0}, Spread {-20.0, -26.0, 6.0}})
    {
        DYNAMIC_SECTION("spread " << s.expect)
        {
            const auto sig = stereo(concatCh(sineChannel(48000.0, 30.0, s.a), sineChannel(48000.0, 30.0, s.b)));
            const double ours = measureMeter<bws::audio::Bs1770Meter>(sig, 48000.0, 480).lra;
            INFO("ours=" << ours << " expected spread=" << s.expect);
            REQUIRE_THAT(ours, WithinAbs(s.expect, 1.0));
        }
    }
}

TEST_CASE("Bs1770Meter LRA is sample-rate invariant", "[bs1770][lra][samplerate][calibration]")
{
    constexpr std::array<double, 5> kRates {44100.0, 48000.0, 96000.0, 176400.0, 192000.0};
    const auto varied = [](double sr) {
        return stereo(concatCh(sineChannel(sr, 35.0, -23.0), sineChannel(sr, 35.0, -33.0)));
    };
    const double ref = measureMeter<bws::audio::Bs1770Meter>(varied(48000.0), 48000.0, 480).lra;
    REQUIRE(ref > 3.0);
    for (const double sr : kRates)
    {
        DYNAMIC_SECTION("sr=" << sr)
        {
            REQUIRE_THAT(measureMeter<bws::audio::Bs1770Meter>(varied(sr), sr, 480).lra, WithinAbs(ref, 1.0));
        }
    }
}

//=============================================================================
// D. Block-size invariance (incl. the pathological 1-sample block).
//=============================================================================
TEST_CASE("Bs1770Meter integrated + M/S are block-size invariant", "[bs1770][invariance][calibration]")
{
    const auto sig = stereo(sineChannel(48000.0, 14.0, -23.0));
    const auto ref = measureMeter<bws::audio::Bs1770Meter>(sig, 48000.0, 512);
    for (const int bs : {1, 7, 64, 128, 480, 1024, 4096})
    {
        DYNAMIC_SECTION("block=" << bs)
        {
            const auto r = measureMeter<bws::audio::Bs1770Meter>(sig, 48000.0, bs);
            REQUIRE_THAT(r.integrated, WithinAbs(ref.integrated, 0.05));
            REQUIRE_THAT(r.momentary, WithinAbs(ref.momentary, 0.05));
            REQUIRE_THAT(r.shortTerm, WithinAbs(ref.shortTerm, 0.05));
        }
    }
}

//=============================================================================
// E. Edge / cases.
//=============================================================================
TEST_CASE("Bs1770Meter: silence yields no valid integrated and floored readouts", "[bs1770][edge]")
{
    const auto sig = stereo(silenceChannel(48000.0, 6.0));
    bws::audio::Bs1770Meter meter;
    meter.prepare(48000.0, 2);
    const auto r = measureMeter<bws::audio::Bs1770Meter>(sig, 48000.0, 480);
    INFO("I=" << r.integrated << " M=" << r.momentary << " S=" << r.shortTerm);
    REQUIRE(r.integrated <= bws::audio::Bs1770Meter::kMinLufs + 0.001);
    REQUIRE(r.momentary <= bws::audio::Bs1770Meter::kMinLufs + 0.001);
    REQUIRE(isFinite(r.integrated));
    REQUIRE(isFinite(r.lra));
}

TEST_CASE("Bs1770Meter: a program entirely below the -70 absolute gate has no integrated", "[bs1770][edge][gating]")
{
    const auto sig = stereo(sineChannel(48000.0, 8.0, -80.0)); // below -70 abs gate
    const double ours = measureMeter<bws::audio::Bs1770Meter>(sig, 48000.0, 480).integrated;
    INFO("I=" << ours);
    REQUIRE(ours <= bws::audio::Bs1770Meter::kMinLufs + 0.001);
}

TEST_CASE("Bs1770Meter: signal at the absolute-gate boundary stays finite", "[bs1770][edge][gating]")
{
    const auto sig = stereo(sineChannel(48000.0, 8.0, -70.0));
    const auto r = measureMeter<bws::audio::Bs1770Meter>(sig, 48000.0, 480);
    REQUIRE(isFinite(r.integrated));
    REQUIRE(isFinite(r.momentary));
    REQUIRE(isFinite(r.lra));
}

TEST_CASE("Bs1770Meter: mono input reads 3.01 LU below the duplicated-stereo signal",
          "[bs1770][edge][mono][calibration]")
{
    const auto ch = sineChannel(48000.0, 12.0, -23.0);
    const double mono = measureMeter<bws::audio::Bs1770Meter>({ch}, 48000.0, 480).integrated;
    const double stereoDup = measureMeter<bws::audio::Bs1770Meter>({ch, ch}, 48000.0, 480).integrated;
    // BS.1770 sums channel powers: duplicating the channel to stereo adds
    // 10*log10(2) = 3.0103 LU over the single-channel (mono) reading.
    INFO("mono=" << mono << " stereoDup=" << stereoDup);
    REQUIRE(isFinite(mono));
    REQUIRE_THAT(stereoDup - mono, WithinAbs(3.0103, 0.1));
}

TEST_CASE("Bs1770Meter: long-duration program does not saturate the gating grid",
          "[bs1770][edge][duration][calibration]")
{
    // ~5 min vs ~12 s of the same steady tone must read identically - the grid's
    // per-cell counts must not drift or overflow the integrated value over time.
    const double shortRun =
        measureMeter<bws::audio::Bs1770Meter>(stereo(sineChannel(48000.0, 12.0, -23.0)), 48000.0, 4096).integrated;
    const double longRun =
        measureMeter<bws::audio::Bs1770Meter>(stereo(sineChannel(48000.0, 300.0, -23.0)), 48000.0, 4096).integrated;
    INFO("shortRun=" << shortRun << " longRun=" << longRun);
    REQUIRE_THAT(longRun, WithinAbs(shortRun, 0.1));
}

TEST_CASE("Bs1770Meter: logical reset invalidates integrated while M/S stay live", "[bs1770][edge][reset]")
{
    const auto sig = stereo(sineChannel(48000.0, 10.0, -23.0));
    bws::audio::Bs1770Meter meter;
    meter.prepare(48000.0, 2);

    std::vector<float> bufL(480), bufR(480);
    const float* ch[2] = {bufL.data(), bufR.data()};
    PlanarBufferView view;
    view.channelData = ch;
    view.numChannels = 2;
    view.numSamples = 480;
    auto pump = [&](int blocks) {
        std::size_t f = 0;
        for (int b = 0; b < blocks; ++b)
        {
            for (int i = 0; i < 480; ++i)
            {
                bufL[static_cast<std::size_t>(i)] = sig[0][f + static_cast<std::size_t>(i)];
                bufR[static_cast<std::size_t>(i)] = sig[1][f + static_cast<std::size_t>(i)];
            }
            meter.process(view);
            f += 480;
        }
    };
    pump(800); // ~8 s
    while (meter.serviceAnalytics())
    {}
    REQUIRE(meter.areAnalyticsValid());

    meter.resetIntegrated();
    REQUIRE_FALSE(meter.areAnalyticsValid());
    REQUIRE(meter.getIntegratedLufs() <= bws::audio::Bs1770Meter::kMinLufs + 0.001);
    REQUIRE(meter.getMomentaryLufs() > -70.0f); // M/S remain live
}

TEST_CASE("Bs1770Meter: tone followed by silence stays finite (denormal tail)", "[bs1770][edge][rt-safety]")
{
    const auto sig = stereo(concatCh(sineChannel(48000.0, 4.0, -23.0), silenceChannel(48000.0, 4.0)));
    const auto r = measureMeter<bws::audio::Bs1770Meter>(sig, 48000.0, 480);
    REQUIRE(isFinite(r.integrated));
    REQUIRE(isFinite(r.momentary));
    REQUIRE(isFinite(r.shortTerm));
    REQUIRE(isFinite(r.lra));
}

//=============================================================================
// F. Grid quantization worst-case error bound.
//=============================================================================
TEST_CASE("Bs1770Meter integrated quantization error stays within the cell bound",
          "[bs1770][quantization][calibration]")
{
    // Each exact -2.5 dB amplitude step must read as a -2.5 LU integrated step
    // (calibration-independent). The grid stores exact energy sums, so the only
    // error is gate-boundary cell quantization (0.1 LU cells).
    double worstStepError = 0.0;
    double prev = 0.0;
    bool havePrev = false;
    for (double lufs = -55.0; lufs <= -10.0 + 1e-9; lufs += 2.5)
    {
        const auto sig = stereo(sineChannel(48000.0, 10.0, lufs));
        const double ours = measureMeter<bws::audio::Bs1770Meter>(sig, 48000.0, 480).integrated;
        if (havePrev)
            worstStepError = std::max(worstStepError, std::abs((ours - prev) - 2.5));
        prev = ours;
        havePrev = true;
    }
    INFO("worst |step - 2.5 LU| across level sweep = " << worstStepError << " LU");
    REQUIRE(worstStepError <= 0.1);
}

// EBU Tech 3341 case 13 (Maximum Momentary): a 400 ms burst in silence must read
// Max-M = the burst's steady level, regardless of where the burst falls relative
// to the meter's internal grid. A 100 ms momentary cadence underreads a
// grid-misaligned 400 ms burst by ~0.6 LU (measured); the 5 ms sub-step cadence
// captures it within the EBU ±0.1 LU band. Anchored on the steady reference
// (calibration-independent).
TEST_CASE("Bs1770Meter Max-Momentary captures a 400 ms burst within 0.1 LU (EBU 3341 case 13)",
          "[bs1770][momentary][transient][calibration]")
{
    const double sr = 48000.0;
    const double steadyM =
        measureMeter<bws::audio::Bs1770Meter>(stereo(sineChannel(sr, 2.0, -23.0)), sr, 480).momentary;

    // Several burst placements, including offsets that are NOT multiples of the
    // 5 ms sub-step (e.g. 0.3331 s), to exercise genuine sub-grid misalignment.
    for (const double leadSilence : {0.3, 0.35, 0.3331, 0.4173})
    {
        DYNAMIC_SECTION("lead silence " << leadSilence << " s")
        {
            const auto sig = stereo(concatCh(concatCh(silenceChannel(sr, leadSilence), sineChannel(sr, 0.4, -23.0)),
                                             silenceChannel(sr, 1.0)));
            double maxM = 0.0, maxS = 0.0;
            measureMaxMS<bws::audio::Bs1770Meter>(sig, sr, 64, maxM, maxS);
            INFO("steadyM=" << steadyM << " maxM=" << maxM << " under=" << (steadyM - maxM));
            REQUIRE_THAT(maxM, WithinAbs(steadyM, 0.1));
        }
    }
}

// EBU Tech 3341 case 10 (Maximum Short-term): a >=3 s burst must read Max-S = the
// steady short-term level (the 3 s window is wide enough that sub-step alignment
// is a non-issue, but this confirms the short-term max-capture path).
TEST_CASE("Bs1770Meter Max-Short-term captures a 3 s burst within 0.1 LU (EBU 3341 case 10)",
          "[bs1770][shortterm][transient][calibration]")
{
    const double sr = 48000.0;
    const double steadyS =
        measureMeter<bws::audio::Bs1770Meter>(stereo(sineChannel(sr, 4.0, -23.0)), sr, 480).shortTerm;

    const auto sig =
        stereo(concatCh(concatCh(silenceChannel(sr, 0.5), sineChannel(sr, 3.05, -23.0)), silenceChannel(sr, 1.0)));
    double maxM = 0.0, maxS = 0.0;
    measureMaxMS<bws::audio::Bs1770Meter>(sig, sr, 64, maxM, maxS);
    INFO("steadyS=" << steadyS << " maxS=" << maxS);
    REQUIRE_THAT(maxS, WithinAbs(steadyS, 0.1));
}

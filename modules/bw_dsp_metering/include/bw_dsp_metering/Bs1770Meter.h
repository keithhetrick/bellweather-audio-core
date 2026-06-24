// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
//
// ITU-R BS.1770-4/5 + EBU Tech 3341/3342 loudness meter.
//
//   * K-weighting: the BS.1770 published 48 kHz biquad coefficient tables. For
//     other sample rates the biquads are re-derived by a pole/zero bilinear
//     re-mapping - inverse-transform the published 48 kHz poles/zeros to the
//     analog reference response, then bilinear-transform at the target rate.
//   * Gated integrated loudness + LRA use a bounded "energy grid": per 0.1-LU
//     cell, the summed mean-square energy and the observation count. Storing
//     energy sums (not just counts) yields an exact gated mean rather than a
//     bin-centre approximation.
//   * Live momentary/short-term and the gated analytics share a lock-free SPSC
//     ingress + seqlock snapshot bridge (detail/LoudnessAnalyticsBridge.h).
//
//
#pragma once

#include "bw_dsp_metering/detail/LoudnessAnalyticsBridge.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace bws::audio
{

/**
 * Header-only, framework-neutral, allocation-free stereo BS.1770 / EBU R128 meter.
 *
 * Live momentary (400 ms) and short-term (3 s) loudness are computed on the
 * audio callback. Ordered analytics (gated integrated loudness + EBU Tech 3342
 * loudness range) are produced via the shared SPSC ingress and serviced by one
 * non-RT owner.
 */
class Bs1770Meter : public detail::LoudnessAnalyticsBridge<Bs1770Meter>
{
    using Bridge = detail::LoudnessAnalyticsBridge<Bs1770Meter>;
    friend Bridge; // CRTP hooks are private.

public:
    using AnalyticsMode = detail::AnalyticsMode;

    static constexpr int kMaxChannels = 2;
    static constexpr float kMinLufs = detail::kMinLufs;
    static constexpr float kAbsoluteThresholdLufs = detail::kAbsoluteThresholdLufs;
    static constexpr float kRelativeThresholdLu = detail::kRelativeThresholdLu;
    static constexpr float kLraRelativeThresholdLu = detail::kLraRelativeThresholdLu;

    // Energy-grid geometry: [-120, +30) LUFS in 0.1-LU cells, indexed by direct
    // arithmetic (floor((LK - min) / cell)) rather than a boundary-array search.
    static constexpr double kGridMinLufs = -120.0;
    static constexpr double kGridCellLu = 0.1;
    static constexpr int kGridCells = 1500; // [-120, +30) LUFS
    static constexpr double kGridCellsPerLu = 1.0 / kGridCellLu;

    // The momentary/short-term DISPLAY windows slide on a fine 5 ms sub-step so
    // the live readouts (and EBU "Maximum Momentary/Short-term") capture a
    // grid-misaligned ~400 ms burst within ±0.1 LU (a 100 ms cadence underreads
    // such bursts by up to ~0.6 LU - measured). The INTEGRATED/LRA gating block
    // still advances every 100 ms (= 20 sub-steps), 75% overlap per BS.1770-2.
    static constexpr double kSubStepSeconds = 0.005;   // 5 ms
    static constexpr int kSubStepsPerGatingBlock = 20; // 100 ms gating cadence
    static constexpr int kMomentarySubSteps = 80;      // 400 ms window
    static constexpr int kShortTermSubSteps = 600;     // 3 s window

    explicit Bs1770Meter(AnalyticsMode mode = AnalyticsMode::ManualService) noexcept
        : Bridge(mode)
    {}

    // Back-compatible bridge for existing consumers: `false` = live M/S only.
    explicit Bs1770Meter(bool enableAnalytics) noexcept
        : Bridge(enableAnalytics ? AnalyticsMode::ManualService : AnalyticsMode::LiveOnly)
    {}

    void prepare(double sampleRate, int numChannels = 2) noexcept
    {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
        numChannels_ = std::clamp(numChannels, 1, kMaxChannels);
        subStepSamples_ = std::max(1, static_cast<int>(sampleRate_ * kSubStepSeconds));
        computeKWeightingCoefficients();
        if (analyticsMode() != AnalyticsMode::LiveOnly)
        {
            if (!integratedGrid_)
                integratedGrid_ = std::make_unique<EnergyGrid>();
            if (!shortTermGrid_)
                shortTermGrid_ = std::make_unique<EnergyGrid>();
        }
        prepareBridge(sampleRate_);
        clearGatingState();
        resetLiveState();
    }

    /// RT-callable logical reset; queue storage left intact while active.
    void reset() noexcept
    {
        resetLiveState();
        resetIntegratedBridge();
    }

    void resetIntegrated() noexcept { resetIntegratedBridge(); }

    template <typename BufferType>
    void process(const BufferType& buffer) noexcept BWS_METERING_NONBLOCKING
    {
        const int numSamples = buffer.getNumSamples();
        const int channels = std::min(buffer.getNumChannels(), numChannels_);
        if (numSamples <= 0 || channels <= 0)
            return;

        producerSyncEpoch();

        for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
        {
            double sumSquared = 0.0;
            for (int channel = 0; channel < channels; ++channel)
            {
                const auto channelIndex = static_cast<std::size_t>(channel);
                double sample = static_cast<double>(buffer.getReadPointer(channel)[sampleIndex]);
                sample = processBiquad(sample, shelfCoeffs_, shelfState_[channelIndex]);
                sample = processBiquad(sample, hpfCoeffs_, hpfState_[channelIndex]);
                sumSquared += sample * sample;
            }

            stepAccum_ += sumSquared;
            ++stepSampleCount_;
            producerAdvanceSamples(1);

            if (stepSampleCount_ < subStepSamples_)
                continue;

            const double subMeanSquare = stepAccum_ / static_cast<double>(stepSampleCount_);
            history_[historyWrite_] = subMeanSquare;
            historyWrite_ = (historyWrite_ + 1) % history_.size();
            if (historyCount_ < history_.size())
                ++historyCount_;

            const double momentaryMeanSquare = windowMeanSquare(kMomentarySubSteps);
            momentaryLufs_.store(clampLufs(momentaryMeanSquare), std::memory_order_relaxed);

            const double shortTermMeanSquare = windowMeanSquare(kShortTermSubSteps);
            shortTermLufs_.store(clampLufs(shortTermMeanSquare), std::memory_order_relaxed);

            // Emit one gating block per 100 ms (every 20 sub-steps), using the
            // current 400 ms momentary / 3 s short-term windows.
            if (++gatingSubStepCounter_ >= kSubStepsPerGatingBlock)
            {
                gatingSubStepCounter_ = 0;
                if (analyticsMode() != AnalyticsMode::LiveOnly &&
                    historyCount_ >= static_cast<std::size_t>(kMomentarySubSteps))
                {
                    producerEmit(momentaryMeanSquare, historyCount_ >= static_cast<std::size_t>(kShortTermSubSteps)
                                                          ? shortTermMeanSquare
                                                          : 0.0);
                }
            }

            stepAccum_ = 0.0;
            stepSampleCount_ = 0;
        }
    }

    bool serviceLoudnessRange() noexcept { return serviceAnalytics(); }

    float getMomentaryLufs() const noexcept { return momentaryLufs_.load(std::memory_order_relaxed); }
    float getShortTermLufs() const noexcept { return shortTermLufs_.load(std::memory_order_relaxed); }

private:
    struct BiquadCoeffs
    {
        double b0 {}, b1 {}, b2 {}, a1 {}, a2 {};
    };
    struct BiquadState
    {
        double z1 {}, z2 {};
    };

    // Two parallel arrays per gating window: summed mean-square energy and the
    // observation count per loudness cell. The mean energy of any loudness
    // sub-range is then exact (sum / count), not a bin-centre estimate.
    struct EnergyGrid
    {
        std::array<double, kGridCells> energy {};
        std::array<uint64_t, kGridCells> count {};

        void clear() noexcept
        {
            energy = {};
            count = {};
        }
    };

    //--- K-weighting ----------------------------------------------------------

    static double processBiquad(double input, const BiquadCoeffs& c, BiquadState& s) noexcept
    {
        const double output = c.b0 * input + s.z1;
        s.z1 = c.b1 * input - c.a1 * output + s.z2;
        s.z2 = c.b2 * input - c.a2 * output;
        return snapDenormal(output);
    }

    static double snapDenormal(double x) noexcept
    {
        // Guard the recursive state against denormal slowdown on silence tails;
        // FPCR.FZ/MXCSR flush arithmetic only and Apple Silicon lacks DAZ.
        return (std::fabs(x) < 1e-20) ? 0.0 : x;
    }

    // Published ITU-R BS.1770-4/5 48 kHz coefficients (Table 1 / Table 2).
    // These are facts stated verbatim in the standard.
    static constexpr BiquadCoeffs shelf48k() noexcept
    {
        return {1.53512485958697, -2.69169618940638, 1.19839281085285, -1.69065929318241, 0.73248077421585};
    }
    static constexpr BiquadCoeffs rlbHpf48k() noexcept { return {1.0, -2.0, 1.0, -1.99004745483398, 0.99007225036621}; }

    void computeKWeightingCoefficients() noexcept
    {
        // Match the 48 kHz response at a per-stage reference frequency: DC for
        // the high-shelf (its low band is ~unity), 1 kHz for the RLB high-pass
        // (a passband point - its DC gain is a null and cannot be matched).
        shelfCoeffs_ = redesignForRate(shelf48k(), sampleRate_, 0.0);
        hpfCoeffs_ = redesignForRate(rlbHpf48k(), sampleRate_, 1000.0);
    }

    // Re-derive a 48 kHz biquad at an arbitrary rate by mapping its poles and
    // zeros through the inverse then forward bilinear transform, fixing overall
    // gain at `referenceHz`. At 48 kHz this is the identity (returns the table).
    static BiquadCoeffs redesignForRate(const BiquadCoeffs& src, double targetRate, double referenceHz) noexcept
    {
        constexpr double kRate48k = 48000.0;
        if (std::abs(targetRate - kRate48k) < 1e-6)
            return src;

        const double t1 = 1.0 / kRate48k;
        const double t2 = 1.0 / targetRate;

        // Poles: roots of z^2 + a1 z + a2 (denominator is monic).
        std::complex<double> p1, p2;
        quadraticRoots(1.0, src.a1, src.a2, p1, p2);
        const auto np1 = remapRoot(p1, t1, t2);
        const auto np2 = remapRoot(p2, t1, t2);

        // Zeros: roots of b0 z^2 + b1 z + b2.
        std::complex<double> z1, z2;
        quadraticRoots(src.b0, src.b1, src.b2, z1, z2);
        const auto nz1 = remapRoot(z1, t1, t2);
        const auto nz2 = remapRoot(z2, t1, t2);

        BiquadCoeffs out;
        out.a1 = -(np1 + np2).real();
        out.a2 = (np1 * np2).real();
        // Monic numerator from remapped zeros; gain fixed below.
        out.b0 = 1.0;
        out.b1 = -(nz1 + nz2).real();
        out.b2 = (nz1 * nz2).real();

        const double srcMag = magnitudeAt(src, referenceHz, kRate48k);
        const double outMag = magnitudeAt(out, referenceHz, targetRate);
        const double gain = outMag > 0.0 ? srcMag / outMag : 1.0;
        out.b0 *= gain;
        out.b1 *= gain;
        out.b2 *= gain;
        return out;
    }

    // Roots of a z^2 + b z + c (a != 0), real or complex-conjugate pair.
    static void quadraticRoots(double a, double b, double c, std::complex<double>& r1,
                               std::complex<double>& r2) noexcept
    {
        const std::complex<double> disc = std::sqrt(std::complex<double>(b * b - 4.0 * a * c, 0.0));
        r1 = (-b + disc) / (2.0 * a);
        r2 = (-b - disc) / (2.0 * a);
    }

    // Inverse bilinear (z@t1 -> s) then forward bilinear (s -> z@t2).
    static std::complex<double> remapRoot(std::complex<double> z, double t1, double t2) noexcept
    {
        const std::complex<double> s = (2.0 / t1) * (z - 1.0) / (z + 1.0);
        const std::complex<double> half = s * (t2 / 2.0);
        return (1.0 + half) / (1.0 - half);
    }

    static double magnitudeAt(const BiquadCoeffs& c, double freqHz, double sampleRate) noexcept
    {
        constexpr double pi = 3.14159265358979323846;
        const double w = 2.0 * pi * freqHz / sampleRate;
        const std::complex<double> zInv = std::polar(1.0, -w);
        const std::complex<double> zInv2 = zInv * zInv;
        const std::complex<double> num = c.b0 + c.b1 * zInv + c.b2 * zInv2;
        const std::complex<double> den = 1.0 + c.a1 * zInv + c.a2 * zInv2;
        return std::abs(num / den);
    }

    //--- live momentary / short-term ------------------------------------------

    void resetLiveState() noexcept
    {
        shelfState_ = {};
        hpfState_ = {};
        history_ = {};
        historyWrite_ = 0;
        historyCount_ = 0;
        stepAccum_ = 0.0;
        stepSampleCount_ = 0;
        gatingSubStepCounter_ = 0;
        momentaryLufs_.store(kMinLufs, std::memory_order_relaxed);
        shortTermLufs_.store(kMinLufs, std::memory_order_relaxed);
    }

    double windowMeanSquare(std::size_t requested) const noexcept
    {
        const std::size_t count = std::min(requested, historyCount_);
        if (count == 0)
            return 0.0;
        double sum = 0.0;
        for (std::size_t i = 0; i < count; ++i)
            sum += history_[(historyWrite_ + history_.size() - 1 - i) % history_.size()];
        return sum / static_cast<double>(count);
    }

    static double meanSquareToLufs(double meanSquare) noexcept
    {
        return meanSquare > 0.0 ? -0.691 + 10.0 * std::log10(meanSquare) : static_cast<double>(kMinLufs);
    }

    static float clampLufs(double meanSquare) noexcept
    {
        return std::max(static_cast<float>(meanSquareToLufs(meanSquare)), kMinLufs);
    }

    static int gridIndex(double lufs) noexcept
    {
        const int idx = static_cast<int>(std::floor((lufs - kGridMinLufs) * kGridCellsPerLu));
        return std::clamp(idx, 0, kGridCells - 1);
    }

    static double cellCenterLufs(int idx) noexcept
    {
        return kGridMinLufs + (static_cast<double>(idx) + 0.5) * kGridCellLu;
    }

    static void addToGrid(EnergyGrid& grid, double meanSquare) noexcept
    {
        if (meanSquare <= 0.0)
            return;
        const int idx = gridIndex(meanSquareToLufs(meanSquare));
        grid.energy[static_cast<std::size_t>(idx)] += meanSquare;
        grid.count[static_cast<std::size_t>(idx)] += 1;
    }

    //--- CRTP gating hooks (driven by the bridge servicer) --------------------

    void clearGatingState() noexcept
    {
        if (integratedGrid_)
            integratedGrid_->clear();
        if (shortTermGrid_)
            shortTermGrid_->clear();
    }

    void consumeRecord(const detail::LoudnessRecord& record) noexcept
    {
        if (integratedGrid_)
            addToGrid(*integratedGrid_, record.integratedMeanSquare);
        if (shortTermGrid_ && record.shortTermMeanSquare > 0.0)
            addToGrid(*shortTermGrid_, record.shortTermMeanSquare);
    }

    // EBU R128 gated integrated loudness: absolute gate at -70 LUFS, then a
    // relative gate 10 LU below the absolute-gated mean. Energy sums make each
    // gated mean exact; only the boundary cell quantises (0.1 LU << 0.1 LU band).
    float computeIntegratedLufs() const noexcept
    {
        if (!integratedGrid_)
            return kMinLufs;
        const EnergyGrid& g = *integratedGrid_;

        double absEnergy = 0.0;
        uint64_t absCount = 0;
        for (int i = 0; i < kGridCells; ++i)
        {
            if (cellCenterLufs(i) < static_cast<double>(kAbsoluteThresholdLufs))
                continue;
            absEnergy += g.energy[static_cast<std::size_t>(i)];
            absCount += g.count[static_cast<std::size_t>(i)];
        }
        if (absCount == 0)
            return kMinLufs;

        const double absMeanLufs = meanSquareToLufs(absEnergy / static_cast<double>(absCount));
        const double relThreshold =
            std::max(static_cast<double>(kAbsoluteThresholdLufs), absMeanLufs + kRelativeThresholdLu);

        double relEnergy = 0.0;
        uint64_t relCount = 0;
        for (int i = 0; i < kGridCells; ++i)
        {
            if (cellCenterLufs(i) < relThreshold)
                continue;
            relEnergy += g.energy[static_cast<std::size_t>(i)];
            relCount += g.count[static_cast<std::size_t>(i)];
        }
        if (relCount == 0)
            return kMinLufs;
        return clampLufs(relEnergy / static_cast<double>(relCount));
    }

    // EBU Tech 3342 LRA: 95th - 10th percentile of gated 3 s short-term
    // loudness, with a -20 LU relative gate. Percentiles read by cumulative
    // count over the ascending grid.
    float computeLoudnessRange() const noexcept
    {
        if (!shortTermGrid_)
            return 0.0f;
        const EnergyGrid& g = *shortTermGrid_;

        double absEnergy = 0.0;
        uint64_t absCount = 0;
        for (int i = 0; i < kGridCells; ++i)
        {
            if (cellCenterLufs(i) < static_cast<double>(kAbsoluteThresholdLufs))
                continue;
            absEnergy += g.energy[static_cast<std::size_t>(i)];
            absCount += g.count[static_cast<std::size_t>(i)];
        }
        if (absCount == 0)
            return 0.0f;

        const double absMeanLufs = meanSquareToLufs(absEnergy / static_cast<double>(absCount));
        const double relThreshold =
            std::max(static_cast<double>(kAbsoluteThresholdLufs), absMeanLufs + kLraRelativeThresholdLu);

        uint64_t gatedCount = 0;
        for (int i = 0; i < kGridCells; ++i)
            if (cellCenterLufs(i) >= relThreshold)
                gatedCount += g.count[static_cast<std::size_t>(i)];
        if (gatedCount == 0)
            return 0.0f;

        const auto lowRank = static_cast<uint64_t>((static_cast<double>(gatedCount) - 1.0) * 0.10 + 0.5);
        const auto highRank = static_cast<uint64_t>((static_cast<double>(gatedCount) - 1.0) * 0.95 + 0.5);

        const double lowLufs = loudnessAtRank(g, relThreshold, lowRank);
        const double highLufs = loudnessAtRank(g, relThreshold, highRank);
        return static_cast<float>(highLufs - lowLufs);
    }

    static double loudnessAtRank(const EnergyGrid& g, double gateLufs, uint64_t rank) noexcept
    {
        uint64_t seen = 0;
        double lastCenter = gateLufs;
        for (int i = 0; i < kGridCells; ++i)
        {
            const double center = cellCenterLufs(i);
            if (center < gateLufs)
                continue;
            const uint64_t n = g.count[static_cast<std::size_t>(i)];
            if (n == 0)
                continue;
            lastCenter = center;
            if (rank < seen + n)
                return center;
            seen += n;
        }
        return lastCenter;
    }

    bool hasShortTermData() const noexcept
    {
        return shortTermGrid_ && std::any_of(shortTermGrid_->count.begin(), shortTermGrid_->count.end(),
                                             [](uint64_t n) { return n != 0; });
    }

    //--- state ----------------------------------------------------------------

    double sampleRate_ {48000.0};
    int numChannels_ {2};
    int subStepSamples_ {240};    // 5 ms @ 48 kHz
    int gatingSubStepCounter_ {}; // counts sub-steps toward the 100 ms gating block

    BiquadCoeffs shelfCoeffs_ {};
    BiquadCoeffs hpfCoeffs_ {};
    std::array<BiquadState, kMaxChannels> shelfState_ {};
    std::array<BiquadState, kMaxChannels> hpfState_ {};

    std::array<double, kShortTermSubSteps + 1> history_ {}; // 3 s of 5 ms sub-steps
    std::size_t historyWrite_ {};
    std::size_t historyCount_ {};
    double stepAccum_ {};
    int stepSampleCount_ {};

    std::atomic<float> momentaryLufs_ {kMinLufs};
    std::atomic<float> shortTermLufs_ {kMinLufs};

    // Gating grids exist only for analytics modes (allocated in prepare(), off
    // the audio thread). LiveOnly meters leave them null and carry no gating
    // storage at all - more memory-frugal than typical reference
    // implementations, which allocate gating structures regardless of the
    // requested loudness mode.
    std::unique_ptr<EnergyGrid> integratedGrid_;
    std::unique_ptr<EnergyGrid> shortTermGrid_;
};

} // namespace bws::audio

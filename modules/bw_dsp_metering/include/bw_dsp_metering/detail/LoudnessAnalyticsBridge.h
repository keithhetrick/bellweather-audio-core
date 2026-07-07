// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
//
// Lock-free SPSC ingress + seqlock snapshot bridge for loudness analytics.
//
// A pure real-time concurrency substrate - single-producer/single-consumer ring,
// seqlock-published snapshot, epoch/session lifecycle - carrying no BS.1770
// algorithm. The meter (Bs1770Meter.h) layers its gating core on top via CRTP
// hooks.
#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace bws::audio::detail
{

#if defined(__clang__) && defined(__has_cpp_attribute) && defined(__has_warning)
#if __has_cpp_attribute(clang::nonblocking) && __has_warning("-Wfunction-effects")
#define BWS_METERING_NONBLOCKING [[clang::nonblocking]]
#endif
#endif

#ifndef BWS_METERING_NONBLOCKING
#define BWS_METERING_NONBLOCKING
#endif

// Shared loudness-analytics constants (BS.1770 / EBU R128 gate thresholds and
// the meter's display floor). These are spec values, not code fingerprints.
inline constexpr float kMinLufs = -100.0f;
inline constexpr float kAbsoluteThresholdLufs = -70.0f;
inline constexpr float kRelativeThresholdLu = -10.0f;
inline constexpr float kLraRelativeThresholdLu = -20.0f;
inline constexpr double kMinSecondsForLraStability = 60.0;

enum class AnalyticsMode
{
    LiveOnly,
    ManualService,
    ExternalService
};

// One ordered observation handed from the audio thread to the non-RT servicer.
// `integratedMeanSquare` is the 400 ms gating-block energy; `shortTermMeanSquare`
// is the 3 s short-term energy (0.0 until the 3 s window is full).
struct LoudnessRecord
{
    uint64_t sessionId {};
    uint64_t epoch {};
    uint64_t sequence {};
    int64_t samplesProcessed {};
    double integratedMeanSquare {};
    double shortTermMeanSquare {};
};

// The seqlock-published readout. Payload (integratedLufs / loudnessRange /
// lraStable) is computed by the derived gating core; validity/lifecycle is the
// bridge's.
struct LoudnessSnapshot
{
    uint64_t sessionId {};
    uint64_t epoch {};
    float integratedLufs {kMinLufs};
    float loudnessRange {};
    bool valid {};
    bool lraStable {};
};

/**
 * CRTP base providing the lock-free SPSC ingress + seqlock snapshot bridge.
 *
 * The derived meter supplies the gating core via five hooks:
 *   void  clearGatingState() noexcept;            // wipe per-session statistics
 *   void  consumeRecord(const LoudnessRecord&) noexcept;
 *   float computeIntegratedLufs() const noexcept; // gated integrated loudness
 *   float computeLoudnessRange() const noexcept;  // EBU Tech 3342 LRA
 *   bool  hasShortTermData() const noexcept;       // any ST observation seen?
 *
 * The derived meter drives the producer side from its process() via
 * producerSyncEpoch() / producerAdvanceSamples() / producerEmit(), and exposes
 * the readout via the snapshot accessors below.
 *
 * Threading: exactly one audio (producer) thread and at most one servicer
 * (consumer) thread. The servicer is gated by an atomic_flag so concurrent
 * serviceAnalytics() calls are rejected rather than racing.
 */
template <typename Derived>
class LoudnessAnalyticsBridge
{
public:
    static constexpr std::size_t kIngressCapacity = 4096;
    static constexpr std::size_t kMaxDrainBatch = 1024;

    static_assert(std::atomic<float>::is_always_lock_free);
    static_assert(std::atomic<double>::is_always_lock_free);
    static_assert(std::atomic<uint64_t>::is_always_lock_free);
    static_assert(std::atomic<std::size_t>::is_always_lock_free);

    explicit LoudnessAnalyticsBridge(AnalyticsMode mode) noexcept
        : analyticsMode_(mode)
    {}

    AnalyticsMode analyticsMode() const noexcept { return analyticsMode_; }

    /**
     * Drain one bounded non-RT analytics batch. Exactly one owner may call this
     * for a given meter instance; concurrent calls are rejected (counted).
     */
    bool serviceAnalytics() noexcept
    {
        if (analyticsMode_ == AnalyticsMode::LiveOnly)
            return false;
        if (serviceOwner_.test_and_set(std::memory_order_acquire))
        {
            rejectedConcurrentService_.fetch_add(1, std::memory_order_relaxed);
            return false;
        }

        bool serviced = false;
        std::size_t drained = 0;
        LoudnessRecord record;
        while (drained < kMaxDrainBatch && dequeue(record))
        {
            serviced = true;
            ++drained;
            consume(record);
        }
        if (serviced)
            publishAnalytics();

        serviceOwner_.clear(std::memory_order_release);
        return serviced;
    }

    bool areAnalyticsValid() const noexcept { return readSnapshot().valid; }

    float getIntegratedLufs() const noexcept
    {
        const auto snapshot = readSnapshot();
        return snapshot.valid ? snapshot.integratedLufs : kMinLufs;
    }

    float getLoudnessRange() const noexcept
    {
        const auto snapshot = readSnapshot();
        return snapshot.valid ? snapshot.loudnessRange : 0.0f;
    }

    bool isLoudnessRangeStable() const noexcept
    {
        const auto snapshot = readSnapshot();
        return snapshot.valid && snapshot.lraStable;
    }

    uint64_t getDroppedIngressRecords() const noexcept
    {
        return droppedIngressRecords_.load(std::memory_order_relaxed);
    }
    uint64_t getRejectedConcurrentServiceCalls() const noexcept
    {
        return rejectedConcurrentService_.load(std::memory_order_relaxed);
    }
    std::size_t getPendingAnalyticsRecords() const noexcept
    {
        const auto write = ingressWrite_.load(std::memory_order_acquire);
        const auto read = ingressRead_.load(std::memory_order_acquire);
        return write - read;
    }

protected:
    // --- lifecycle (called by the derived meter's prepare/reset) -------------

    void prepareBridge(double sampleRate) noexcept
    {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
        // LiveOnly meters never enqueue/service, so they carry no ingress ring.
        // Analytics modes allocate it once, off the audio thread, in prepare().
        if (analyticsMode_ != AnalyticsMode::LiveOnly && !ingress_)
            ingress_ = std::make_unique<std::array<LoudnessRecord, kIngressCapacity>>();
        ++sessionId_;
        epoch_.store(1, std::memory_order_relaxed);
        producerEpoch_ = 1;
        sequence_ = 0;
        samplesProcessedInEpoch_ = 0;
        ingressWrite_.store(0, std::memory_order_relaxed);
        ingressRead_.store(0, std::memory_order_relaxed);
        producerLossEpoch_.store(0, std::memory_order_relaxed);
        droppedIngressRecords_.store(0, std::memory_order_relaxed);
        rejectedConcurrentService_.store(0, std::memory_order_relaxed);
        clearWorkerAnalytics(sessionId_, producerEpoch_);
        invalidateAnalytics();
    }

    void resetIntegratedBridge() noexcept
    {
        epoch_.fetch_add(1, std::memory_order_acq_rel);
        invalidateAnalytics();
    }

    // --- producer side (RT, called from the derived meter's process()) -------

    // Call once per processed block before stepping samples: picks up any
    // logical reset (epoch advance) and rebases the per-epoch sample counter.
    void producerSyncEpoch() noexcept BWS_METERING_NONBLOCKING
    {
        const uint64_t requestedEpoch = epoch_.load(std::memory_order_relaxed);
        if (requestedEpoch != producerEpoch_)
        {
            producerEpoch_ = requestedEpoch;
            samplesProcessedInEpoch_ = 0;
        }
    }

    void producerAdvanceSamples(int64_t n) noexcept BWS_METERING_NONBLOCKING { samplesProcessedInEpoch_ += n; }

    // Emit one ordered observation for the current epoch.
    void producerEmit(double integratedMeanSquare, double shortTermMeanSquare) noexcept BWS_METERING_NONBLOCKING
    {
        if (analyticsMode_ == AnalyticsMode::LiveOnly)
            return;
        LoudnessRecord record;
        record.sessionId = sessionId_;
        record.epoch = producerEpoch_;
        record.sequence = ++sequence_;
        record.samplesProcessed = samplesProcessedInEpoch_;
        record.integratedMeanSquare = integratedMeanSquare;
        record.shortTermMeanSquare = shortTermMeanSquare;
        enqueue(record);
    }

    double bridgeSampleRate() const noexcept { return sampleRate_; }

private:
    void enqueue(const LoudnessRecord& record) noexcept BWS_METERING_NONBLOCKING
    {
        const std::size_t write = ingressWrite_.load(std::memory_order_relaxed);
        const std::size_t read = ingressRead_.load(std::memory_order_acquire);
        if (write - read >= kIngressCapacity)
        {
            producerLossEpoch_.store(record.epoch, std::memory_order_release);
            droppedIngressRecords_.fetch_add(1, std::memory_order_relaxed);
            return;
        }
        (*ingress_)[write % kIngressCapacity] = record;
        ingressWrite_.store(write + 1, std::memory_order_release);
    }

    bool dequeue(LoudnessRecord& record) noexcept
    {
        const std::size_t read = ingressRead_.load(std::memory_order_relaxed);
        if (read == ingressWrite_.load(std::memory_order_acquire))
            return false;
        record = (*ingress_)[read % kIngressCapacity];
        ingressRead_.store(read + 1, std::memory_order_release);
        return true;
    }

    void consume(const LoudnessRecord& record) noexcept
    {
        if (record.sessionId != workerSession_ || record.epoch != workerEpoch_)
            clearWorkerAnalytics(record.sessionId, record.epoch);

        if (workerExpectedSequence_ != 0 && record.sequence != workerExpectedSequence_ + 1)
            workerValid_ = false;
        workerExpectedSequence_ = record.sequence;
        if (producerLossEpoch_.load(std::memory_order_acquire) == record.epoch)
            workerValid_ = false;
        workerSamplesProcessed_ = record.samplesProcessed;

        static_cast<Derived*>(this)->consumeRecord(record);
    }

    void clearWorkerAnalytics(uint64_t sessionId, uint64_t epoch) noexcept
    {
        static_cast<Derived*>(this)->clearGatingState();
        workerSession_ = sessionId;
        workerEpoch_ = epoch;
        workerExpectedSequence_ = 0;
        workerSamplesProcessed_ = 0;
        workerValid_ = true;
    }

    void publishAnalytics() noexcept
    {
        const auto* self = static_cast<const Derived*>(this);
        LoudnessSnapshot snapshot;
        snapshot.sessionId = workerSession_;
        snapshot.epoch = workerEpoch_;
        snapshot.integratedLufs = self->computeIntegratedLufs();
        snapshot.loudnessRange = self->computeLoudnessRange();
        snapshot.valid = workerValid_ && workerSession_ == sessionId_ &&
                         workerEpoch_ == epoch_.load(std::memory_order_acquire) && snapshot.integratedLufs > kMinLufs;
        snapshot.lraStable =
            snapshot.valid &&
            workerSamplesProcessed_ >= static_cast<int64_t>(sampleRate_ * kMinSecondsForLraStability) &&
            self->hasShortTermData();

        snapshotGeneration_.fetch_add(1, std::memory_order_acq_rel);
        publishedSessionId_.store(snapshot.sessionId, std::memory_order_relaxed);
        publishedEpoch_.store(snapshot.epoch, std::memory_order_relaxed);
        publishedIntegratedLufs_.store(snapshot.integratedLufs, std::memory_order_relaxed);
        publishedLoudnessRange_.store(snapshot.loudnessRange, std::memory_order_relaxed);
        publishedValid_.store(snapshot.valid, std::memory_order_relaxed);
        publishedLraStable_.store(snapshot.lraStable, std::memory_order_relaxed);
        snapshotGeneration_.fetch_add(1, std::memory_order_release);
    }

    void invalidateAnalytics() noexcept
    {
        publishedValid_.store(false, std::memory_order_release);
        publishedLraStable_.store(false, std::memory_order_release);
    }

    LoudnessSnapshot readSnapshot() const noexcept
    {
        for (int attempt = 0; attempt < 3; ++attempt)
        {
            const uint64_t before = snapshotGeneration_.load(std::memory_order_acquire);
            if ((before & 1U) != 0)
                continue;
            LoudnessSnapshot snapshot;
            snapshot.sessionId = publishedSessionId_.load(std::memory_order_relaxed);
            snapshot.epoch = publishedEpoch_.load(std::memory_order_relaxed);
            snapshot.integratedLufs = publishedIntegratedLufs_.load(std::memory_order_relaxed);
            snapshot.loudnessRange = publishedLoudnessRange_.load(std::memory_order_relaxed);
            snapshot.valid = publishedValid_.load(std::memory_order_relaxed);
            snapshot.lraStable = publishedLraStable_.load(std::memory_order_relaxed);
            const uint64_t after = snapshotGeneration_.load(std::memory_order_acquire);
            if (before == after && (after & 1U) == 0)
            {
                if (snapshot.sessionId == sessionId_ && snapshot.epoch == epoch_.load(std::memory_order_acquire) &&
                    producerLossEpoch_.load(std::memory_order_acquire) != snapshot.epoch)
                    return snapshot;
                break;
            }
        }
        LoudnessSnapshot invalid;
        invalid.sessionId = sessionId_;
        invalid.epoch = epoch_.load(std::memory_order_relaxed);
        return invalid;
    }

    AnalyticsMode analyticsMode_ {AnalyticsMode::ManualService};
    double sampleRate_ {48000.0};

    // Producer-owned (audio thread).
    uint64_t producerEpoch_ {1};
    uint64_t sequence_ {};
    int64_t samplesProcessedInEpoch_ {};

    // Shared lifecycle.
    uint64_t sessionId_ {};
    std::atomic<uint64_t> epoch_ {1};

    // SPSC ingress ring. Heap-allocated lazily in prepareBridge() for analytics
    // modes only (prepare() is not the audio thread); LiveOnly leaves it null.
    std::unique_ptr<std::array<LoudnessRecord, kIngressCapacity>> ingress_;
    std::atomic<std::size_t> ingressWrite_ {};
    std::atomic<std::size_t> ingressRead_ {};
    std::atomic<uint64_t> producerLossEpoch_ {};
    std::atomic<uint64_t> droppedIngressRecords_ {};

    // Consumer-owned (servicer thread).
    uint64_t workerSession_ {};
    uint64_t workerEpoch_ {};
    uint64_t workerExpectedSequence_ {};
    int64_t workerSamplesProcessed_ {};
    bool workerValid_ {true};
    std::atomic_flag serviceOwner_;
    std::atomic<uint64_t> rejectedConcurrentService_ {};

    // Seqlock-published snapshot.
    mutable std::atomic<uint64_t> snapshotGeneration_ {};
    std::atomic<uint64_t> publishedSessionId_ {};
    std::atomic<uint64_t> publishedEpoch_ {};
    std::atomic<float> publishedIntegratedLufs_ {kMinLufs};
    std::atomic<float> publishedLoudnessRange_ {};
    std::atomic<bool> publishedValid_ {};
    std::atomic<bool> publishedLraStable_ {};
};

} // namespace bws::audio::detail

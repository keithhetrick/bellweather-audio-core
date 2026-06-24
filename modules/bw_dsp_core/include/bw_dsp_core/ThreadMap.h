// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
/**
 * @file ThreadMap.h
 * @brief Constexpr thread crossing declarations
 *
 * Declares the thread-crossing model for a plugin's atomic data flow.
 * Zero runtime cost - pure compile-time metadata.
 *
 * Usage:
 *   static constexpr auto kThreadCrossings = bws::dsp::ThreadMap{
 *       bws::dsp::AtomicCrossing{
 *           "params",
 *           bws::dsp::Thread::Message >> bws::dsp::Thread::Audio,
 *           bws::dsp::Ordering::Relaxed,
 *           "Parameter sync via APVTS",
 *           "std::atomic<float>"
 *       }
 *   };
 *   static_assert(kThreadCrossings.size() == 1);
 */

#pragma once

#include <cstddef>

namespace bws::dsp
{

// ─── Thread Identity ───────────────────────────────────────────────────

enum class Thread
{
    Audio,
    Message,
    UI
};

enum class Ordering
{
    Relaxed,
    AcquireRelease,
    Sequential
};

// ─── Data Flow Direction ───────────────────────────────────────────────

struct ThreadDirection
{
    Thread from;
    Thread to;
};

// operator>> expresses data flow: Thread::Message >> Thread::Audio
constexpr ThreadDirection operator>>(Thread from, Thread to)
{
    return ThreadDirection {from, to};
}

// ─── Atomic Crossing ──────────────────────────────────────────────────

struct AtomicCrossing
{
    const char* name;
    ThreadDirection direction;
    Ordering ordering;
    const char* purpose;
    const char* atomicType = nullptr; // "std::atomic<float>", etc.
};

// ─── Thread Map ────────────────────────────────────────────────────────
// Size-deduced array. NOT using AtomicCrossing as NTTP to avoid
// const char* portability issues across compilers.

template <size_t N>
struct ThreadMap
{
    AtomicCrossing crossings[N];

    static constexpr size_t size() { return N; }

    constexpr const AtomicCrossing& operator[](size_t i) const { return crossings[i]; }
};

// CTAD deduction guide
template <typename... Args>
ThreadMap(Args...) -> ThreadMap<sizeof...(Args)>;

} // namespace bws::dsp

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>

#if defined(__SSE__) || defined(__x86_64__) || defined(_M_X64) || defined(_M_IX86)
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <immintrin.h>
#endif
#define BWS_HAS_FLUSH_DENORMALS_X86 1
#elif (defined(__aarch64__) || defined(__ARM_ARCH_ISA_A64)) && !defined(_MSC_VER)
#define BWS_HAS_FLUSH_DENORMALS_ARM64 1
#endif

namespace bws::rt
{

/// RAII scope guard that flushes denormal floats to zero for the duration
/// of the scope.
///
/// x86/x64: sets FZ (flush-to-zero, bit 15) and DAZ (denormals-are-zero, bit 6)
///          in MXCSR via _mm_getcsr / _mm_setcsr.
/// ARM64:   sets FZ (bit 24) in FPCR via mrs/msr inline asm.
///          FPCR is 64-bit - saved64_ must be uint64_t to avoid truncation.
///          CRITICAL: ARM64 FPCR is thread-local and NOT inherited by child
///          threads. Must be set on the audio thread itself. Using this guard
///          in processBlockImpl() (which runs on the audio thread) is correct -
///          it sets FZ at the start of every block on the thread that needs it.
/// Other:   no-op (safe default). Windows ARM64 currently uses this path.
///
/// RT-safe: no allocation, no locks, no unbounded work.
struct ScopedFlushDenormals
{
    ScopedFlushDenormals() noexcept
    {
#if BWS_HAS_FLUSH_DENORMALS_X86
        saved32_ = _mm_getcsr();
        _mm_setcsr(saved32_ | 0x8040u); // bit 15 = FZ, bit 6 = DAZ
#elif BWS_HAS_FLUSH_DENORMALS_ARM64
        asm volatile("mrs %0, fpcr" : "=r"(saved64_) : :);
        const uint64_t fz = saved64_ | (1ull << 24u); // bit 24 = FZ
        asm volatile("msr fpcr, %0" : : "r"(fz) :);
#endif
    }

    ~ScopedFlushDenormals() noexcept
    {
#if BWS_HAS_FLUSH_DENORMALS_X86
        _mm_setcsr(saved32_);
#elif BWS_HAS_FLUSH_DENORMALS_ARM64
        asm volatile("msr fpcr, %0" : : "r"(saved64_) :);
#endif
    }

    ScopedFlushDenormals(const ScopedFlushDenormals&) = delete;
    ScopedFlushDenormals& operator=(const ScopedFlushDenormals&) = delete;

private:
    // MXCSR (x86) is 32-bit; FPCR (ARM64) is 64-bit - separate members avoid truncation.
#if BWS_HAS_FLUSH_DENORMALS_X86
    uint32_t saved32_ {0};
#elif BWS_HAS_FLUSH_DENORMALS_ARM64
    uint64_t saved64_ {0};
#endif
};

} // namespace bws::rt

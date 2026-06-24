// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstddef>
#include <memory>

namespace bws::fft
{

/// Framework-neutral FFT interface. Power-of-two sizes only.
/// Order N -> size 2^N.
/// Forward: time-domain float* -> complex spectrum (interleaved re/im).
/// Inverse: complex spectrum -> time-domain float* (unnormalized).
class BwsFFT
{
public:
    explicit BwsFFT(int order);
    ~BwsFFT();

    BwsFFT(const BwsFFT&) = delete;
    BwsFFT& operator=(const BwsFFT&) = delete;

    int order() const noexcept { return order_; }
    int size() const noexcept { return 1 << order_; }

    /// Forward real FFT. data must be size() floats.
    /// Output: interleaved half-spectrum in size() floats:
    ///   data[0]=DC, data[1]=Nyquist, data[2k]=Re[k], data[2k+1]=Im[k] for k=1..N/2-1
    void performRealOnlyForwardTransform(float* data) const;

    /// Inverse real FFT. data is interleaved half-spectrum (size() floats).
    /// Output: real time-domain (size() floats). Unnormalized (scaled by N).
    void performRealOnlyInverseTransform(float* data) const;

    /// Forward real FFT + in-place magnitude extraction.
    /// Output: data[k] = magnitude at bin k, for k=0..N/2. Upper half zeroed.
    void performFrequencyOnlyForwardTransform(float* data) const;

    /// Complex forward FFT. data is interleaved complex (2*size() floats).
    void performComplexForwardTransform(float* data) const;

    /// Complex inverse FFT. data is interleaved complex (2*size() floats).
    /// Unnormalized (scaled by N).
    void performComplexInverseTransform(float* data) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    int order_;
};

} // namespace bws::fft

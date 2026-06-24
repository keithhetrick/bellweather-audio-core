// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#include <bw_fft_adapters/BwsFFT.h>

#if defined(__APPLE__)
#include <Accelerate/Accelerate.h>
#define BWS_FFT_USE_VDSP 1
#else
#include "../vendor/pffft/pffft.h"
#define BWS_FFT_USE_PFFFT 1
#endif

#include <cmath>
#include <cstring>
#include <vector>

namespace bws::fft
{

// ─────────────────────────────────────────────────────────────────────────────
// Impl - dual setup: real + complex
// ─────────────────────────────────────────────────────────────────────────────

struct BwsFFT::Impl
{
#if BWS_FFT_USE_VDSP
    // Real (zrop = zero-padded real, out-of-place)
    vDSP_DFT_Setup realFwd {nullptr};
    vDSP_DFT_Setup realInv {nullptr};
    // Complex (zop = zero-padded, out-of-place)
    vDSP_DFT_Setup complexFwd {nullptr};
    vDSP_DFT_Setup complexInv {nullptr};
    // Split-complex scratch buffers
    std::vector<float> realBuf, imagBuf;
    std::vector<float> realBuf2, imagBuf2; // second pair for complex transforms
#else
    PFFFT_Setup* realSetup {nullptr};
    PFFFT_Setup* complexSetup {nullptr};
    std::vector<float> workBuf; // max(n, 2*n) floats
#endif
};

// ─────────────────────────────────────────────────────────────────────────────
// Construction / Destruction
// ─────────────────────────────────────────────────────────────────────────────

BwsFFT::BwsFFT(int order)
    : impl_(std::make_unique<Impl>())
    , order_(order)
{
    [[maybe_unused]] const int n = size();

#if BWS_FFT_USE_VDSP
    // Real FFT setups
    impl_->realFwd = vDSP_DFT_zrop_CreateSetup(nullptr, static_cast<vDSP_Length>(n), vDSP_DFT_FORWARD);
    impl_->realInv = vDSP_DFT_zrop_CreateSetup(nullptr, static_cast<vDSP_Length>(n), vDSP_DFT_INVERSE);
    impl_->realBuf.resize(static_cast<size_t>(n / 2));
    impl_->imagBuf.resize(static_cast<size_t>(n / 2));

    // Complex FFT setups
    impl_->complexFwd = vDSP_DFT_zop_CreateSetup(nullptr, static_cast<vDSP_Length>(n), vDSP_DFT_FORWARD);
    impl_->complexInv = vDSP_DFT_zop_CreateSetup(nullptr, static_cast<vDSP_Length>(n), vDSP_DFT_INVERSE);
    impl_->realBuf2.resize(static_cast<size_t>(n));
    impl_->imagBuf2.resize(static_cast<size_t>(n));
#else
    impl_->realSetup = pffft_new_setup(n, PFFFT_REAL);
    impl_->complexSetup = pffft_new_setup(n, PFFFT_COMPLEX);
    impl_->workBuf.resize(static_cast<size_t>(2 * n)); // complex needs 2*n
#endif
}

BwsFFT::~BwsFFT()
{
#if BWS_FFT_USE_VDSP
    if (impl_->realFwd)
        vDSP_DFT_DestroySetup(impl_->realFwd);
    if (impl_->realInv)
        vDSP_DFT_DestroySetup(impl_->realInv);
    if (impl_->complexFwd)
        vDSP_DFT_DestroySetup(impl_->complexFwd);
    if (impl_->complexInv)
        vDSP_DFT_DestroySetup(impl_->complexInv);
#else
    if (impl_->realSetup)
        pffft_destroy_setup(impl_->realSetup);
    if (impl_->complexSetup)
        pffft_destroy_setup(impl_->complexSetup);
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// Real FFT
// ─────────────────────────────────────────────────────────────────────────────

void BwsFFT::performRealOnlyForwardTransform(float* data) const
{
    [[maybe_unused]] const int n = size();
    (void)n;

#if BWS_FFT_USE_VDSP
    // Pack interleaved real into split-complex (even/odd)
    DSPSplitComplex input {impl_->realBuf.data(), impl_->imagBuf.data()};
    vDSP_ctoz(reinterpret_cast<DSPComplex*>(data), 2, &input, 1, static_cast<vDSP_Length>(n / 2));

    // Forward real DFT
    vDSP_DFT_Execute(impl_->realFwd, input.realp, input.imagp, input.realp, input.imagp);

    // vDSP zrop forward applies 2x scaling vs mathematical DFT convention.
    // Compensate to match pffft / JUCE output.
    const float half = 0.5f;
    vDSP_vsmul(input.realp, 1, &half, input.realp, 1, static_cast<vDSP_Length>(n / 2));
    vDSP_vsmul(input.imagp, 1, &half, input.imagp, 1, static_cast<vDSP_Length>(n / 2));

    // Unpack split-complex back to interleaved format:
    // data[0]=DC, data[1]=Nyquist, data[2k]=Re[k], data[2k+1]=Im[k]
    data[0] = input.realp[0]; // DC
    data[1] = input.imagp[0]; // Nyquist (vDSP packs Nyquist in imagp[0])
    for (int k = 1; k < n / 2; ++k)
    {
        data[2 * k] = input.realp[k];
        data[2 * k + 1] = input.imagp[k];
    }
#else
    pffft_transform_ordered(impl_->realSetup, data, data, impl_->workBuf.data(), PFFFT_FORWARD);
#endif
}

void BwsFFT::performRealOnlyInverseTransform(float* data) const
{
    [[maybe_unused]] const int n = size();
    (void)n;

#if BWS_FFT_USE_VDSP
    // Pack interleaved half-spectrum into split-complex
    DSPSplitComplex input {impl_->realBuf.data(), impl_->imagBuf.data()};
    input.realp[0] = data[0]; // DC
    input.imagp[0] = data[1]; // Nyquist
    for (int k = 1; k < n / 2; ++k)
    {
        input.realp[k] = data[2 * k];
        input.imagp[k] = data[2 * k + 1];
    }

    // Inverse real DFT
    vDSP_DFT_Execute(impl_->realInv, input.realp, input.imagp, input.realp, input.imagp);

    // Unpack split-complex back to interleaved real
    vDSP_ztoc(&input, 1, reinterpret_cast<DSPComplex*>(data), 2, static_cast<vDSP_Length>(n / 2));
#else
    pffft_transform_ordered(impl_->realSetup, data, data, impl_->workBuf.data(), PFFFT_BACKWARD);
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// Frequency-only (real forward + magnitude extraction)
// ─────────────────────────────────────────────────────────────────────────────

void BwsFFT::performFrequencyOnlyForwardTransform(float* data) const
{
    performRealOnlyForwardTransform(data);

    [[maybe_unused]] const int n = size();
    const int half = n / 2;

    // Extract magnitudes in-place.
    // After real FFT: data[0]=DC, data[1]=Nyquist, data[2k]=Re[k], data[2k+1]=Im[k]
    // Write magnitudes: data[0..(N/2)]
    // Safe because write index k < read indices 2k, 2k+1 for k >= 1.
    const float dc = data[0];
    const float nyquist = data[1];

    for (int k = 1; k < half; ++k)
    {
        const float re = data[2 * k];
        const float im = data[2 * k + 1];
        data[k] = std::sqrt(re * re + im * im);
    }
    data[0] = std::abs(dc);
    data[half] = std::abs(nyquist);

    // Zero upper half
    for (int k = half + 1; k < n; ++k)
        data[k] = 0.0f;
}

// ─────────────────────────────────────────────────────────────────────────────
// Complex FFT
// ─────────────────────────────────────────────────────────────────────────────

void BwsFFT::performComplexForwardTransform(float* data) const
{
    [[maybe_unused]] const int n = size();

#if BWS_FFT_USE_VDSP
    // Deinterleave into split-complex
    float* rp = impl_->realBuf2.data();
    float* ip = impl_->imagBuf2.data();
    for (int k = 0; k < n; ++k)
    {
        rp[k] = data[2 * k];
        ip[k] = data[2 * k + 1];
    }

    vDSP_DFT_Execute(impl_->complexFwd, rp, ip, rp, ip);

    // Interleave back
    for (int k = 0; k < n; ++k)
    {
        data[2 * k] = rp[k];
        data[2 * k + 1] = ip[k];
    }
#else
    pffft_transform_ordered(impl_->complexSetup, data, data, impl_->workBuf.data(), PFFFT_FORWARD);
#endif
}

void BwsFFT::performComplexInverseTransform(float* data) const
{
    [[maybe_unused]] const int n = size();

#if BWS_FFT_USE_VDSP
    // Deinterleave into split-complex
    float* rp = impl_->realBuf2.data();
    float* ip = impl_->imagBuf2.data();
    for (int k = 0; k < n; ++k)
    {
        rp[k] = data[2 * k];
        ip[k] = data[2 * k + 1];
    }

    vDSP_DFT_Execute(impl_->complexInv, rp, ip, rp, ip);

    // Interleave back
    for (int k = 0; k < n; ++k)
    {
        data[2 * k] = rp[k];
        data[2 * k + 1] = ip[k];
    }
#else
    pffft_transform_ordered(impl_->complexSetup, data, data, impl_->workBuf.data(), PFFFT_BACKWARD);
#endif
}

} // namespace bws::fft

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
//
// TruePeakMeter spec-conformance + contract tests.
//
// SPEC CITATIONS:
//   ITU-R BS.1770-5 §3.5 (true peak measurement at 4× minimum polyphase)
//   Nielsen 1998 (inter-sample peak overshoot analytic bound = +3.0103 dB
//                 for sine at fs/4 with maximally-displaced phase = π/4)

#include <bw_dsp_metering/TruePeakMeter.h>

#include "bws/test/AllocCounter.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <numbers>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef BW_TECH3341_FIXTURES_DIR
#error "BW_TECH3341_FIXTURES_DIR must be defined by CMake"
#endif

using Catch::Matchers::WithinAbs;

namespace
{
constexpr double kPi = std::numbers::pi_v<double>;

// Native-first stdlib-only buffer adapter for TruePeakMeter::process() duck-typed
// buffer. The test target exercises the framework-neutral meter directly.
struct PlanarBufferView
{
    const float* const* channelData {nullptr};
    int numChannels {0};
    int numSamples {0};

    int getNumSamples() const noexcept { return numSamples; }
    int getNumChannels() const noexcept { return numChannels; }
    const float* getReadPointer(int ch) const noexcept { return channelData[ch]; }
};

// Push N seconds of stereo sine. Default phase = 0 means samples land on the
// analytic peaks, so the ISP overshoot is zero - adequate for non-ISP tests.
// The Nielsen/Lund ISP test below uses an inline phase-controlled loop.
template <typename Meter>
void pushSineSeconds(Meter& meter, double sampleRate, double seconds, float dBFS_peak, double frequencyHz = 1000.0)
{
    constexpr int kBlockSamples = 480;
    const float amplitude = std::pow(10.0f, dBFS_peak / 20.0f);
    double phase = 0.0;
    const double phaseInc = 2.0 * kPi * frequencyHz / sampleRate;

    std::array<float, kBlockSamples> bufL {}, bufR {};
    const float* channels[2] = {bufL.data(), bufR.data()};

    PlanarBufferView view;
    view.channelData = channels;
    view.numChannels = 2;
    view.numSamples = kBlockSamples;

    const long long totalSamples = static_cast<long long>(seconds * sampleRate);
    long long pushed = 0;
    while (pushed + kBlockSamples <= totalSamples)
    {
        for (int i = 0; i < kBlockSamples; ++i)
        {
            const float s = amplitude * static_cast<float>(std::sin(phase));
            bufL[i] = s;
            bufR[i] = s;
            phase += phaseInc;
            if (phase > 2.0 * kPi)
                phase -= 2.0 * kPi;
        }
        meter.process(view);
        pushed += kBlockSamples;
    }
}

template <typename Meter>
void pushSilenceSeconds(Meter& meter, double sampleRate, double seconds)
{
    constexpr int kBlockSamples = 480;
    std::array<float, kBlockSamples> bufL {}, bufR {};
    const float* channels[2] = {bufL.data(), bufR.data()};

    PlanarBufferView view;
    view.channelData = channels;
    view.numChannels = 2;
    view.numSamples = kBlockSamples;

    const long long totalSamples = static_cast<long long>(seconds * sampleRate);
    long long pushed = 0;
    while (pushed + kBlockSamples <= totalSamples)
    {
        meter.process(view);
        pushed += kBlockSamples;
    }
}

// Interleaved stereo (L=R) sine of given amplitude/frequency/start-phase.
std::vector<float> makeStereoSineInterleaved(double sampleRate, double seconds, float amplitude, double frequencyHz,
                                             double phase0) noexcept
{
    const long long frames = static_cast<long long>(seconds * sampleRate);
    std::vector<float> out(static_cast<std::size_t>(frames) * 2u, 0.0f);
    double phase = phase0;
    const double phaseInc = 2.0 * kPi * frequencyHz / sampleRate;
    for (long long i = 0; i < frames; ++i)
    {
        const float s = amplitude * static_cast<float>(std::sin(phase));
        const auto base = static_cast<std::size_t>(i) * 2u;
        out[base] = s;
        out[base + 1u] = s;
        phase += phaseInc;
        if (phase > 2.0 * kPi)
        {
            phase -= 2.0 * kPi;
        }
    }
    return out;
}

// Feed an interleaved buffer through a TruePeakMeter in 480-sample planar blocks.
template <typename Meter>
void feedInterleaved(Meter& meter, const std::vector<float>& interleaved, int numChannels)
{
    constexpr int kBlockSamples = 480;
    const long long frames = static_cast<long long>(interleaved.size()) / numChannels;
    std::array<float, kBlockSamples> bufL {}, bufR {};
    const float* channels[2] = {bufL.data(), bufR.data()};
    PlanarBufferView view;
    view.channelData = channels;
    view.numChannels = 2;
    view.numSamples = kBlockSamples;

    long long pos = 0;
    while (pos + kBlockSamples <= frames)
    {
        for (int i = 0; i < kBlockSamples; ++i)
        {
            const auto frame = static_cast<std::size_t>((pos + i) * numChannels);
            bufL[i] = interleaved[frame];
            bufR[i] = interleaved[frame + 1u];
        }
        meter.process(view);
        pos += kBlockSamples;
    }
}

// Maximum true-peak (dBTP) over the whole signal - getTruePeakDb() reports the
// current block's peak, so the cumulative max across blocks is the EBU "Max
// true-peak level" metric (the whole-signal maximum true-peak).
template <typename Meter>
double maxTruePeakDbOverSignal(Meter& meter, const std::vector<float>& interleaved, int numChannels)
{
    constexpr int kBlockSamples = 480;
    const long long frames = static_cast<long long>(interleaved.size()) / numChannels;
    std::array<float, kBlockSamples> bufL {}, bufR {};
    const float* channels[2] = {bufL.data(), bufR.data()};
    PlanarBufferView view;
    view.channelData = channels;
    view.numChannels = 2;
    view.numSamples = kBlockSamples;

    double maxDb = -200.0;
    long long pos = 0;
    while (pos + kBlockSamples <= frames)
    {
        for (int i = 0; i < kBlockSamples; ++i)
        {
            const auto frame = static_cast<std::size_t>((pos + i) * numChannels);
            bufL[i] = interleaved[frame];
            bufR[i] = interleaved[frame + 1u];
        }
        meter.process(view);
        maxDb = std::max(maxDb, static_cast<double>(meter.getTruePeakDb()));
        pos += kBlockSamples;
    }
    return maxDb;
}


// Minimal 32-bit float stereo WAV reader → interleaved samples. Validates
// IEEE_FLOAT (code 3), 32-bit, 48 kHz, 2 channels (the synth output spec).
std::vector<float> readFloat32WavInterleaved(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
    {
        throw std::runtime_error("cannot open WAV: " + path);
    }
    auto readU32 = [&]() -> std::uint32_t {
        std::uint32_t v = 0;
        in.read(reinterpret_cast<char*>(&v), 4);
        return v;
    };
    auto readU16 = [&]() -> std::uint16_t {
        std::uint16_t v = 0;
        in.read(reinterpret_cast<char*>(&v), 2);
        return v;
    };
    auto readTag = [&]() -> std::string {
        char buf[5] = {};
        in.read(buf, 4);
        return std::string(buf, 4);
    };

    if (readTag() != "RIFF" || (static_cast<void>(readU32()), readTag()) != "WAVE")
    {
        throw std::runtime_error("not a WAVE file: " + path);
    }

    std::uint16_t format = 0, channels = 0, bits = 0;
    std::uint32_t sampleRate = 0;
    std::vector<float> samples;
    while (in)
    {
        const std::string tag = readTag();
        if (in.eof() || tag.size() < 4)
        {
            break;
        }
        const std::uint32_t size = readU32();
        if (tag == "fmt ")
        {
            format = readU16();
            channels = readU16();
            sampleRate = readU32();
            static_cast<void>(readU32());
            static_cast<void>(readU16());
            bits = readU16();
            if (size > 16)
            {
                in.seekg(size - 16, std::ios::cur);
            }
        }
        else if (tag == "data")
        {
            samples.resize(size / 4);
            in.read(reinterpret_cast<char*>(samples.data()), size);
            break;
        }
        else
        {
            in.seekg(size, std::ios::cur);
        }
    }
    if (format != 3 || bits != 32 || channels != 2 || sampleRate != 48000)
    {
        throw std::runtime_error("unexpected WAV format in " + path);
    }
    return samples;
}

std::string fixturePath(const char* filename)
{
    return std::string(BW_TECH3341_FIXTURES_DIR) + "/" + filename;
}

// Drive the Nielsen/Lund fs/4 maximally-displaced-phase sine for 0.5 s through
// a TruePeakMeter at the given oversample factor. Returns measured dBTP - the
// caller asserts the bound. Signal amplitude = sqrt(2) so sample peak hits
// 0 dBFS while analytic peak is at +3.0103 dBFS.
template <int Factor>
float runNielsenLundIspAt() noexcept
{
    constexpr double kSampleRate = 48000.0;
    constexpr double kFrequency = 12000.0;
    constexpr double kPhase = kPi / 4.0;
    constexpr float kAmplitude = std::numbers::sqrt2_v<float>;
    constexpr int kBlockSamples = 480;
    constexpr double kSeconds = 0.5;

    bws::audio::TruePeakMeterImpl<Factor> meter;
    meter.prepare(kSampleRate, 2);

    std::array<float, kBlockSamples> bufL {}, bufR {};
    const float* channels[2] = {bufL.data(), bufR.data()};
    PlanarBufferView view;
    view.channelData = channels;
    view.numChannels = 2;
    view.numSamples = kBlockSamples;

    double phase = kPhase;
    const double phaseInc = 2.0 * kPi * kFrequency / kSampleRate;

    const long long totalSamples = static_cast<long long>(kSeconds * kSampleRate);
    long long pushed = 0;
    while (pushed + kBlockSamples <= totalSamples)
    {
        for (int i = 0; i < kBlockSamples; ++i)
        {
            const float s = kAmplitude * static_cast<float>(std::sin(phase));
            bufL[i] = s;
            bufR[i] = s;
            phase += phaseInc;
            if (phase > 2.0 * kPi)
                phase -= 2.0 * kPi;
        }
        meter.process(view);
        pushed += kBlockSamples;
    }
    return meter.getTruePeakDb();
}

} // namespace

//=============================================================================
// 1. Post-prepare init contract
//=============================================================================
TEST_CASE("TruePeakMeter: post-prepare readouts at -100 dBTP sentinel", "[true-peak][init]")
{
    bws::audio::TruePeakMeter meter;
    meter.prepare(48000.0, 2);

    REQUIRE_THAT(meter.getTruePeakDb(), WithinAbs(-100.0f, 0.01f));
    REQUIRE_THAT(meter.getHeldPeakDb(), WithinAbs(-100.0f, 0.01f));
    REQUIRE_THAT(meter.getTruePeakLinear(), WithinAbs(0.0f, 1e-6f));
}

//=============================================================================
// 2. Silence keeps peaks at sentinel (proves no spurious peak detection)
//=============================================================================
TEST_CASE("TruePeakMeter: silence keeps peaks at -100 dBTP", "[true-peak][silence]")
{
    bws::audio::TruePeakMeter meter;
    meter.prepare(48000.0, 2);

    pushSilenceSeconds(meter, 48000.0, 2.0);

    REQUIRE_THAT(meter.getTruePeakDb(), WithinAbs(-100.0f, 0.01f));
}

//=============================================================================
// 3. Absolute calibration: low-frequency sine - ISP overshoot negligible
//
//    For a 1 kHz stereo sine at -23 dBFS_peak, the analytic between-samples
//    peak is essentially equal to the sample peak (LF signals are densely
//    sampled at 48 kHz: 48 samples per cycle). True peak reading should
//    match sample peak within filter-implementation tolerance.
//=============================================================================
TEST_CASE("TruePeakMeter: 1 kHz -23 dBFS_peak stereo sine reads -23 dBTP (LF absolute calibration)",
          "[true-peak][k-weighting][regression]")
{
    bws::audio::TruePeakMeter meter;
    meter.prepare(48000.0, 2);

    pushSineSeconds(meter, 48000.0, 2.0, -23.0f, 1000.0);

    REQUIRE_THAT(meter.getTruePeakDb(), WithinAbs(-23.0f, 0.2f));
}

//=============================================================================
// 4. Nielsen/Lund maximally-displaced-phase sine - ISP analytic bound
//
//    A sine at f = fs/4 sampled with phase offset φ = π/4 produces samples at
//    phases {π/4, 3π/4, 5π/4, 7π/4}, all at sin-magnitude √2/2 ≈ 0.7071. The
//    analytic peak between samples is at the next half-cycle = ±1.0 (signal
//    amplitude). ISP overshoot ratio = 1.0 / 0.7071 = √2 → analytic asymptote
//    of +3.0103 dB above sample peak.
//
//    Test construction: amplitude = √2 places sample peak at exactly 0 dBFS,
//    analytic peak at +3.0103 dBFS. At 4× polyphase (BS.1770-5 minimum), the
//    achievable bound is slightly below the asymptote (~+2.93-2.98 dB).
//    Bounds: > +2.9 catches under-detection; < +3.1 catches over-shoot.
//=============================================================================
TEST_CASE("TruePeakMeter: fs/4 maximally-displaced-phase sine reads ~+3.0103 dBTP "
          "(Nielsen/Lund ISP analytic bound at 4x polyphase)",
          "[true-peak][isp][nielsen-lund][regression][calibration]")
{
    constexpr double kSampleRate = 48000.0;
    constexpr double kFrequency = 12000.0;                     // = fs/4
    constexpr double kPhase = kPi / 4.0;                       // maximally-displaced
    constexpr float kAmplitude = std::numbers::sqrt2_v<float>; // ≈ 1.4142 (sample peak hits 0 dBFS)
    constexpr int kBlockSamples = 480;
    constexpr double kSeconds = 0.5;

    bws::audio::TruePeakMeter meter;
    meter.prepare(kSampleRate, 2);

    std::array<float, kBlockSamples> bufL {}, bufR {};
    const float* channels[2] = {bufL.data(), bufR.data()};
    PlanarBufferView view;
    view.channelData = channels;
    view.numChannels = 2;
    view.numSamples = kBlockSamples;

    double phase = kPhase;
    const double phaseInc = 2.0 * kPi * kFrequency / kSampleRate;

    const long long totalSamples = static_cast<long long>(kSeconds * kSampleRate);
    long long pushed = 0;
    while (pushed + kBlockSamples <= totalSamples)
    {
        for (int i = 0; i < kBlockSamples; ++i)
        {
            const float s = kAmplitude * static_cast<float>(std::sin(phase));
            bufL[i] = s;
            bufR[i] = s;
            phase += phaseInc;
            if (phase > 2.0 * kPi)
                phase -= 2.0 * kPi;
        }
        meter.process(view);
        pushed += kBlockSamples;
    }

    const float tp = meter.getTruePeakDb();

    // Spec-compliance gate: EBU Tech 3341 case 20 acceptance band, ±0.2 dB
    // of the +3.0103 dB analytic asymptote.
    REQUIRE(tp > 2.8103f);
    REQUIRE(tp < 3.2103f);

    // Snapshot-IR gate: ±0.01 dB around empirical Hann-windowed-sinc impl
    // baseline. Re-baseline deliberately if the FIR family changes (e.g. to
    // BS.1770 Annex 2 normative coefficients). Tolerance set above the
    // ~0.001 dB cross-platform floating-point drift floor.
    REQUIRE_THAT(tp, WithinAbs(2.9771f, 0.01f));
}

// EBU Tech 3341 signal 19: fs/4 sine, amplitude sqrt(2), 45° displacement → +3.0103 dBTP
// (Nielsen/Lund analytic). The +3.0103 analytic value is pinned tightly in the
// dedicated fs/4 case above; here the EBU spec band is the conformance bar.
TEST_CASE("TruePeakMeter: EBU Tech 3341 signal 19 reads +3.0 dBTP in band", "[true-peak][isp][tech3341][calibration]")
{
    constexpr double kSampleRate = 48000.0;
    constexpr double kFrequency = 12000.0; // fs/4
    constexpr double kPhase = kPi / 4.0;
    constexpr float kAmplitude = std::numbers::sqrt2_v<float>;
    constexpr double kSeconds = 0.5;

    const auto signal = makeStereoSineInterleaved(kSampleRate, kSeconds, kAmplitude, kFrequency, kPhase);

    bws::audio::TruePeakMeter meter;
    meter.prepare(kSampleRate, 2);
    feedInterleaved(meter, signal, 2);
    const auto ours = static_cast<double>(meter.getTruePeakDb());

    // EBU Tech 3341 spec band +3.0 +0.2/−0.4 dBTP.
    REQUIRE(ours > 2.6);
    REQUIRE(ours < 3.2);
}

// EBU Tech 3341 signals 15-18: amplitude-0.50 sines at fs/4, fs/6, fs/8 with the
// spec displacements → −6.0 dBTP. #15 (0° displacement) lands on the analytic
// peaks (no inter-sample overshoot); #16-18 carry real ISP a 1× meter misses.
TEST_CASE("TruePeakMeter: EBU Tech 3341 signals 15-18 read -6.0 dBTP in band",
          "[true-peak][isp][tech3341][calibration]")
{
    constexpr double kSampleRate = 48000.0;
    constexpr float kAmplitude = 0.5f;
    constexpr double kSeconds = 0.5;

    const std::array<double, 4> freqs {12000.0, 12000.0, 8000.0, 6000.0};
    const std::array<double, 4> displacements {0.0, kPi / 4.0, kPi / 3.0, 67.5 * kPi / 180.0};
    const std::array<const char*, 4> labels {"15: fs/4 0deg", "16: fs/4 45deg", "17: fs/6 60deg", "18: fs/8 67.5deg"};

    for (std::size_t k = 0; k < freqs.size(); ++k)
    {
        INFO("EBU Tech 3341 " << labels[k]);
        const auto signal = makeStereoSineInterleaved(kSampleRate, kSeconds, kAmplitude, freqs[k], displacements[k]);
        bws::audio::TruePeakMeter meter;
        meter.prepare(kSampleRate, 2);
        feedInterleaved(meter, signal, 2);
        const auto ours = static_cast<double>(meter.getTruePeakDb());

        // EBU Tech 3341 spec band -6.0 +0.2/-0.4 dBTP - analytic truth (a sine's
        // continuous peak equals its amplitude), the conformance bar.
        REQUIRE(ours > -6.4);
        REQUIRE(ours < -5.8);
    }
}

// EBU Tech 3341 signals 20-23: a single fs/4 period (amp 1.0) embedded in an
// fs/6 sine, synthesized at 4*fs and anti-alias-decimated to 48 kHz at sample
// offsets 0/1/2/3 → 0.0 dBTP. The offsets land the 48 kHz grid at different
// points relative to the inter-sample peak (sample peak drops to ~-2.5 dBFS on
// one offset), so the meter must recover the ISP. Fixtures are independently
// verified at synth time (synth_tech3341.py --verify).
TEST_CASE("TruePeakMeter: EBU Tech 3341 signals 20-23 read 0.0 dBTP in band", "[true-peak][isp][tech3341][calibration]")
{
    constexpr int kSr = 48000;
    const std::array<const char*, 4> fixtures {"case_20.wav", "case_21.wav", "case_22.wav", "case_23.wav"};

    for (const auto* name : fixtures)
    {
        INFO("EBU Tech 3341 " << name);
        const auto signal = readFloat32WavInterleaved(fixturePath(name));
        bws::audio::TruePeakMeter meter;
        meter.prepare(static_cast<double>(kSr), 2);
        const auto ours = maxTruePeakDbOverSignal(meter, signal, 2);

        // EBU Tech 3341 spec band 0.0 +0.2/-0.4 dBTP.
        REQUIRE(ours > -0.4);
        REQUIRE(ours < 0.2);
    }
}

//=============================================================================
// 5. RT-safety: process() does not allocate on the audio thread
//=============================================================================
TEST_CASE("TruePeakMeter::process() does not allocate on the audio thread", "[true-peak][rt-safety]")
{
    bws::audio::TruePeakMeter meter;
    meter.prepare(48000.0, 2);

    // Pre-warm so first-time codepaths run before the counter opens.
    pushSineSeconds(meter, 48000.0, 0.5, -12.0f, 1000.0);

    bws::test::AllocCounterScope scope;
    pushSineSeconds(meter, 48000.0, 1.0, -12.0f, 1000.0);
    REQUIRE(scope.delta() == 0);
}

//=============================================================================
// 6. Per-factor spec compliance on the Nielsen worst-case signal
//
//    EBU Tech 3341 case 20 publishes ±0.2 dB as the acceptance band for
//    true-peak detection. The Nielsen 1999 worst-case signal (sine at fs/4,
//    phase π/4, amplitude √2) places the analytic peak at exactly +3.0103
//    dBFS. Each available oversample factor must detect within ±0.2 dB of
//    that analytic value to satisfy the spec.
//
//    Empirically (Apple Silicon, Hann-windowed sinc FIR), all three factors
//    detect ~+2.977 dB on this signal - a 0.033 dB gap, well inside the
//    ±0.2 dB band. The polyphase factor itself is not a convergence lever
//    here: for sinusoidal inputs, every factor ≥ 4 samples the half-sample-
//    offset analytic peak at the same grid point. Higher factors are
//    available for users whose signal class genuinely benefits (broadband
//    transients, real-program material) where the polyphase resolution does
//    deploy. The test below gates per-factor spec compliance, not relative
//    ordering.
//=============================================================================
TEST_CASE("TruePeakMeter<8>: Nielsen worst-case detected within EBU Tech 3341 +/-0.2 dB band",
          "[true-peak][isp][nielsen-lund][regression][multi-factor][calibration]")
{
    const float tp = runNielsenLundIspAt<8>();

    REQUIRE(tp > 2.8103f);
    REQUIRE(tp < 3.2103f);
    REQUIRE_THAT(tp, WithinAbs(2.9751f, 0.01f));
}

TEST_CASE("TruePeakMeter<16>: Nielsen worst-case detected within EBU Tech 3341 +/-0.2 dB band",
          "[true-peak][isp][nielsen-lund][regression][multi-factor][calibration]")
{
    const float tp = runNielsenLundIspAt<16>();

    REQUIRE(tp > 2.8103f);
    REQUIRE(tp < 3.2103f);
    REQUIRE_THAT(tp, WithinAbs(2.9766f, 0.01f));
}

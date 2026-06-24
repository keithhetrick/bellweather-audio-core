// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
//
// Bs1770Meter conformance + behavior tests:
//   1. Integrated LUFS hits the EBU Tech 3341 PUBLISHED absolute targets on the
//      synthesized Tech 3341 cases at 48 kHz.
//   2. Analytics-lifecycle + LRA-stability contracts for the off-thread path.
//   3. K-weighting filter-shape regression (HF shelf, LF rolloff).
//   4. process() does not allocate on the audio thread (incl. small blocks).
//
// SPEC CITATIONS:
//   ITU-R BS.1770-5 (algorithm), EBU Tech 3341/3342 v2.0 (signals + bands).
//
// FIXTURE PATH: BW_TECH3341_FIXTURES_DIR is defined by CMake (see
// tools/audiotest/synth_tech3341.py). Fixtures are deterministic-from-spec.

#include <bw_dsp_metering/Bs1770Meter.h>

#include "bws/test/AllocCounter.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <random>
#include <numbers>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using Catch::Matchers::WithinAbs;

namespace
{

constexpr int kSampleRate = 48000;
constexpr int kNumChannels = 2;
constexpr double kToleranceLu = 0.1; // EBU Tech 3341 acceptance band
constexpr double kPi = std::numbers::pi_v<double>;

#ifndef BW_TECH3341_FIXTURES_DIR
#error "BW_TECH3341_FIXTURES_DIR must be defined by CMake"
#endif

// Native-first stdlib-only buffer adapter for Bs1770Meter::process().
struct PlanarBufferView
{
    const float* const* channelData {nullptr};
    int numChannels {0};
    int numSamples {0};

    int getNumSamples() const noexcept { return numSamples; }
    int getNumChannels() const noexcept { return numChannels; }
    const float* getReadPointer(int ch) const noexcept { return channelData[ch]; }
};

// Minimal 32-bit float stereo WAV reader (returns interleaved). Validates the
// synth output spec: IEEE_FLOAT, 32-bit, 48 kHz, 2 channels.
std::vector<float> readFloat32WavInterleaved(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
        throw std::runtime_error("cannot open WAV: " + path);

    auto read_u32 = [&]() -> uint32_t {
        uint32_t v = 0;
        in.read(reinterpret_cast<char*>(&v), 4);
        return v;
    };
    auto read_u16 = [&]() -> uint16_t {
        uint16_t v = 0;
        in.read(reinterpret_cast<char*>(&v), 2);
        return v;
    };
    auto read_tag = [&]() -> std::string {
        char buf[5] = {};
        in.read(buf, 4);
        return std::string(buf, 4);
    };

    if (read_tag() != "RIFF")
        throw std::runtime_error("not a RIFF file: " + path);
    (void)read_u32();
    if (read_tag() != "WAVE")
        throw std::runtime_error("not a WAVE file: " + path);

    uint16_t format = 0, channels = 0, bits = 0;
    uint32_t sampleRate = 0;
    std::vector<float> samples;

    while (in)
    {
        std::string tag = read_tag();
        if (in.eof() || tag.size() < 4)
            break;
        uint32_t size = read_u32();
        if (tag == "fmt ")
        {
            format = read_u16();
            channels = read_u16();
            sampleRate = read_u32();
            (void)read_u32(); // byte rate
            (void)read_u16(); // block align
            bits = read_u16();
            if (size > 16)
                in.seekg(size - 16, std::ios::cur);
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

    if (format != 3 || bits != 32 || channels != kNumChannels || sampleRate != kSampleRate)
        throw std::runtime_error("unexpected WAV format in " + path);
    return samples;
}

// Feed interleaved float through Bs1770Meter in small process() chunks.
double measureIntegratedWithMeter(const std::vector<float>& interleaved, int sampleRate)
{
    bws::audio::Bs1770Meter meter;
    meter.prepare(static_cast<double>(sampleRate), kNumChannels);

    constexpr int kBlockSamples = 480;
    std::vector<float> bufL(kBlockSamples);
    std::vector<float> bufR(kBlockSamples);
    const float* channels[kNumChannels] = {bufL.data(), bufR.data()};

    PlanarBufferView view;
    view.channelData = channels;
    view.numChannels = kNumChannels;
    view.numSamples = kBlockSamples;

    const std::size_t totalFrames = interleaved.size() / kNumChannels;
    std::size_t frame = 0;
    while (frame + kBlockSamples <= totalFrames)
    {
        for (int i = 0; i < kBlockSamples; ++i)
        {
            bufL[i] = interleaved[(frame + i) * 2 + 0];
            bufR[i] = interleaved[(frame + i) * 2 + 1];
        }
        meter.process(view);
        frame += kBlockSamples;
    }
    while (meter.serviceAnalytics())
    {}
    return static_cast<double>(meter.getIntegratedLufs());
}

// Stereo sine, interleaved.
std::vector<float> synthSineStereo(double freqHz, double amplitude, double seconds, int sampleRate)
{
    const auto frames = static_cast<std::size_t>(seconds * sampleRate);
    std::vector<float> out(frames * 2);
    constexpr double pi = 3.14159265358979323846;
    for (std::size_t n = 0; n < frames; ++n)
    {
        const auto s =
            static_cast<float>(amplitude * std::sin(2.0 * pi * freqHz * static_cast<double>(n) / sampleRate));
        out[n * 2 + 0] = s;
        out[n * 2 + 1] = s;
    }
    return out;
}

const char* fixturePath(const char* filename, std::string& storage)
{
    storage = std::string(BW_TECH3341_FIXTURES_DIR) + "/" + filename;
    return storage.c_str();
}

// --- Streaming feed helpers ---------------------------------------------------
// Push N seconds of stereo sine / silence through the meter; drain analytics.
struct SineGenerator
{
    double sampleRate {48000.0};
    double frequencyHz {1000.0};
    double phase {0.0};
    float amplitude {1.0f};

    void setLevel(float dBFS) noexcept { amplitude = std::pow(10.0f, dBFS / 20.0f); }

    float next() noexcept
    {
        const float s = amplitude * static_cast<float>(std::sin(phase));
        phase += 2.0 * kPi * frequencyHz / sampleRate;
        if (phase > 2.0 * kPi)
            phase -= 2.0 * kPi;
        return s;
    }
};

void pushSineSeconds(bws::audio::Bs1770Meter& meter, double sampleRate, double seconds, float dBFS,
                     double frequencyHz = 1000.0)
{
    constexpr int kBlock = 480;
    SineGenerator genL, genR;
    genL.sampleRate = genR.sampleRate = sampleRate;
    genL.frequencyHz = genR.frequencyHz = frequencyHz;
    genL.setLevel(dBFS);
    genR.setLevel(dBFS);

    std::array<float, kBlock> bufL {}, bufR {};
    const float* channels[2] = {bufL.data(), bufR.data()};
    PlanarBufferView view {channels, 2, kBlock};

    const long long total = static_cast<long long>(seconds * sampleRate);
    long long pushed = 0;
    while (pushed + kBlock <= total)
    {
        for (int i = 0; i < kBlock; ++i)
        {
            bufL[i] = genL.next();
            bufR[i] = genR.next();
        }
        meter.process(view);
        pushed += kBlock;
    }
}

void pushSilenceSeconds(bws::audio::Bs1770Meter& meter, double sampleRate, double seconds)
{
    constexpr int kBlock = 480;
    std::array<float, kBlock> bufL {}, bufR {};
    const float* channels[2] = {bufL.data(), bufR.data()};
    PlanarBufferView view {channels, 2, kBlock};

    const long long total = static_cast<long long>(seconds * sampleRate);
    long long pushed = 0;
    while (pushed + kBlock <= total)
    {
        meter.process(view);
        pushed += kBlock;
    }
}

void serviceAllAnalytics(bws::audio::Bs1770Meter& meter)
{
    while (meter.serviceAnalytics())
    {}
}

// Deterministic white noise ~-23 dBFS RMS (above the -70 absolute gate).
struct WhiteNoise
{
    std::mt19937 rng;
    std::normal_distribution<float> dist;
    explicit WhiteNoise(std::uint32_t seed) noexcept
        : rng(seed)
        , dist(0.0f, 0.0708f)
    {}
    float next() noexcept { return dist(rng); }
};

} // namespace

// The synth fixtures are calibrated to TRUE loudness (the K-weighting gain at
// 1 kHz is divided out - see synth_tech3341.py), so the meter must hit the EBU
// Tech 3341 PUBLISHED absolute targets. case_4 validates the -70 absolute gate
// (its -72 block must be dropped). Expected values mirror fixtures/tech3341/META.json.
TEST_CASE("Bs1770Meter integrated LUFS hits the EBU published absolute targets (cases 1-4)",
          "[bs1770][lufs][ebu-absolute][calibration]")
{
    struct Case
    {
        const char* file;
        double expected;
    };
    for (const auto c : {Case {"case_1.wav", -23.0}, Case {"case_2.wav", -33.0}, Case {"case_3.wav", -23.0},
                         Case {"case_4.wav", -23.0}})
    {
        DYNAMIC_SECTION(c.file)
        {
            std::string storage;
            const auto samples = readFloat32WavInterleaved(fixturePath(c.file, storage));
            const double ours = measureIntegratedWithMeter(samples, kSampleRate);
            INFO("ours=" << ours << " EBU expected=" << c.expected);
            REQUIRE_THAT(ours, WithinAbs(c.expected, 0.1));
        }
    }
}

TEST_CASE("Bs1770Meter::process() does not allocate on the audio thread", "[bs1770][rt-safety]")
{
    bws::audio::Bs1770Meter meter;
    meter.prepare(48000.0, 2);

    const auto warm = synthSineStereo(1000.0, 0.25, 3.5, 48000);

    constexpr int kBlockSamples = 480;
    std::vector<float> bufL(kBlockSamples);
    std::vector<float> bufR(kBlockSamples);
    const float* channels[2] = {bufL.data(), bufR.data()};
    PlanarBufferView view;
    view.channelData = channels;
    view.numChannels = 2;
    view.numSamples = kBlockSamples;

    auto pump = [&](const std::vector<float>& interleaved) {
        const std::size_t totalFrames = interleaved.size() / 2;
        std::size_t frame = 0;
        while (frame + kBlockSamples <= totalFrames)
        {
            for (int i = 0; i < kBlockSamples; ++i)
            {
                bufL[i] = interleaved[(frame + i) * 2 + 0];
                bufR[i] = interleaved[(frame + i) * 2 + 1];
            }
            meter.process(view);
            frame += kBlockSamples;
        }
    };

    // Pre-warm first-time codepaths before measuring.
    pump(warm);

    const auto run = synthSineStereo(1000.0, 0.25, 5.0, 48000);
    bws::test::AllocCounterScope scope;
    pump(run);
    REQUIRE(scope.delta() == 0);
}

// Off-thread analytics: live momentary/short-term stay valid; integrated/LRA
// are gated by epoch advance and ingress health.

TEST_CASE("Bs1770Meter: logical reset rejects stale queued analytics records", "[bs1770][analytics][stale-epoch]")
{
    bws::audio::Bs1770Meter meter;
    meter.prepare(48000.0, 2);
    pushSineSeconds(meter, 48000.0, 10.0, -23.0f);
    REQUIRE(meter.getPendingAnalyticsRecords() > 0);

    meter.resetIntegrated();
    serviceAllAnalytics(meter);

    REQUIRE_FALSE(meter.areAnalyticsValid());
    REQUIRE_THAT(meter.getIntegratedLufs(), WithinAbs(-100.0f, 0.01f));
    REQUIRE(meter.getMomentaryLufs() > -70.0f);
    REQUIRE(meter.getShortTermLufs() > -70.0f);
}

TEST_CASE("Bs1770Meter: ingress overflow invalidates analytics while M/S remain live", "[bs1770][analytics][overflow]")
{
    bws::audio::Bs1770Meter meter;
    meter.prepare(48000.0, 2);
    pushSineSeconds(meter, 48000.0, 500.0, -23.0f);

    REQUIRE(meter.getDroppedIngressRecords() > 0);
    serviceAllAnalytics(meter);
    REQUIRE_FALSE(meter.areAnalyticsValid());
    REQUIRE_THAT(meter.getIntegratedLufs(), WithinAbs(-100.0f, 0.01f));
    REQUIRE(meter.getMomentaryLufs() > -70.0f);
    REQUIRE(meter.getShortTermLufs() > -70.0f);
}

TEST_CASE("Bs1770Meter: LiveOnly updates M/S without analytics ingress", "[bs1770][analytics][live-only]")
{
    bws::audio::Bs1770Meter meter(bws::audio::Bs1770Meter::AnalyticsMode::LiveOnly);
    meter.prepare(48000.0, 2);
    pushSineSeconds(meter, 48000.0, 10.0, -23.0f);

    REQUIRE(meter.getMomentaryLufs() > -70.0f);
    REQUIRE(meter.getShortTermLufs() > -70.0f);
    REQUIRE(meter.getPendingAnalyticsRecords() == 0);
    REQUIRE_FALSE(meter.serviceAnalytics());
    REQUIRE_FALSE(meter.areAnalyticsValid());
}

TEST_CASE("Bs1770Meter: external service remains coherent during reset churn",
          "[bs1770][analytics][worker-stress][tsan]")
{
    bws::audio::Bs1770Meter meter(bws::audio::Bs1770Meter::AnalyticsMode::ExternalService);
    meter.prepare(48000.0, 2);

    std::atomic<bool> producerDone {false};
    std::thread serviceOwner([&] {
        while (!producerDone.load(std::memory_order_acquire) || meter.getPendingAnalyticsRecords() > 0)
        {
            (void)meter.serviceAnalytics();
            std::this_thread::yield();
        }
    });

    for (int cycle = 0; cycle < 40; ++cycle)
    {
        pushSineSeconds(meter, 48000.0, 0.5, cycle % 2 == 0 ? -23.0f : -33.0f);
        if (cycle % 7 == 0)
            meter.resetIntegrated();
    }

    producerDone.store(true, std::memory_order_release);
    serviceOwner.join();

    REQUIRE(meter.getDroppedIngressRecords() == 0);
    REQUIRE(meter.getRejectedConcurrentServiceCalls() == 0);
    REQUIRE(meter.getMomentaryLufs() > -70.0f);
    REQUIRE(meter.getShortTermLufs() > -70.0f);
}

// LRA stability (EBU Tech 3342): < 3 s of data → unstable.
TEST_CASE("Bs1770Meter: < 3 s of audio leaves isLoudnessRangeStable() == false", "[bs1770][analytics][lra-stability]")
{
    bws::audio::Bs1770Meter meter;
    meter.prepare(48000.0, 2);
    pushSineSeconds(meter, 48000.0, 2.0, -23.0f);
    REQUIRE(meter.serviceLoudnessRange());
    REQUIRE_FALSE(meter.isLoudnessRangeStable());
}

// LRA stability negative pair: ≥ 60 s of valid audio → stable.
TEST_CASE("Bs1770Meter: >= 60 s of valid audio sets isLoudnessRangeStable() == true",
          "[bs1770][analytics][lra-stability]")
{
    bws::audio::Bs1770Meter meter;
    meter.prepare(48000.0, 2);
    pushSineSeconds(meter, 48000.0, 65.0, -23.0f);
    REQUIRE(meter.serviceLoudnessRange());
    REQUIRE(meter.isLoudnessRangeStable());
}

// LRA stability: 60 s of silence keeps unstable (no gated data passes the gate).
TEST_CASE("Bs1770Meter: 60 s of silence keeps isLoudnessRangeStable() == false", "[bs1770][analytics][lra-stability]")
{
    bws::audio::Bs1770Meter meter;
    meter.prepare(48000.0, 2);
    pushSilenceSeconds(meter, 48000.0, 65.0);
    REQUIRE(meter.serviceLoudnessRange());
    REQUIRE_FALSE(meter.isLoudnessRangeStable());
}

// process() must not allocate on the audio thread at any block size, including
// sub-480 blocks where the 5 ms sub-step accumulator straddles block boundaries.
TEST_CASE("Bs1770Meter: process() allocates zero across small block sizes", "[bs1770][analytics][rt-safety]")
{
    for (const int blockSamples : {32, 64, 128, 512})
    {
        bws::audio::Bs1770Meter meter;
        meter.prepare(48000.0, 2);

        WhiteNoise nL(0xC0FFEE), nR(0xDEADBEEF);
        std::array<float, 512> bufL {}, bufR {};
        const float* channels[2] = {bufL.data(), bufR.data()};
        PlanarBufferView view {channels, 2, blockSamples};

        auto drive = [&](int blocks) {
            for (int b = 0; b < blocks; ++b)
            {
                for (int i = 0; i < blockSamples; ++i)
                {
                    bufL[i] = nL.next();
                    bufR[i] = nR.next();
                }
                meter.process(view);
            }
        };

        drive(200); // pre-warm first-time codepaths
        long long allocations = 0;
        {
            bws::test::AllocCounterScope scope;
            drive(2000);
            allocations = scope.delta();
        }
        INFO("blockSamples=" << blockSamples); // INFO/REQUIRE allocate; keep them out of the scope
        REQUIRE(allocations == 0);
    }
}

//=============================================================================
// K-weighting filter-shape regression: pins the observable consequence of the
// high-shelf (HF gain) and RLB high-pass (LF rolloff). Absolute 1 kHz
// calibration is covered by the EBU published-absolute-target case above.
//=============================================================================

// HF shelf: 10 kHz reads materially higher than 1 kHz at the same amplitude -
// the K-weighting high-shelf (+4 dB asymptote above the ~1.7 kHz shelf centre).
TEST_CASE("Bs1770Meter: 10 kHz vs 1 kHz at -30 dBFS_peak - HF shelf gives the expected delta",
          "[bs1770][k-weighting][regression][calibration]")
{
    bws::audio::Bs1770Meter meter1k, meter10k;
    meter1k.prepare(48000.0, 2);
    meter10k.prepare(48000.0, 2);

    pushSineSeconds(meter1k, 48000.0, 30.0, -30.0f, 1000.0);
    pushSineSeconds(meter10k, 48000.0, 30.0, -30.0f, 10000.0);
    serviceAllAnalytics(meter1k);
    serviceAllAnalytics(meter10k);

    const float delta = meter10k.getIntegratedLufs() - meter1k.getIntegratedLufs();
    INFO("HF-shelf delta (10k - 1k) = " << delta << " LU");
    // +4 dB shelf asymptote minus the partial 1 kHz shelf gain; band tight enough
    // to fail if the shelf design breaks.
    REQUIRE_THAT(delta, WithinAbs(3.34f, 0.1f));
}

// LF rolloff: 20 Hz reads << 1 kHz at the same amplitude (RLB high-pass,
// 2nd-order ~38 Hz). Loose lower bound - catches an HPF-broken design without
// over-pinning the exact rolloff (varies with bilinear prewarp at low fc/fs).
TEST_CASE("Bs1770Meter: 20 Hz vs 1 kHz at -23 dBFS_peak - LF HPF attenuates by > 10 LU",
          "[bs1770][k-weighting][regression][calibration]")
{
    bws::audio::Bs1770Meter meter1k, meter20Hz;
    meter1k.prepare(48000.0, 2);
    meter20Hz.prepare(48000.0, 2);

    pushSineSeconds(meter1k, 48000.0, 30.0, -23.0f, 1000.0);
    pushSineSeconds(meter20Hz, 48000.0, 30.0, -23.0f, 20.0);
    serviceAllAnalytics(meter1k);
    serviceAllAnalytics(meter20Hz);

    const float attenuation = meter1k.getIntegratedLufs() - meter20Hz.getIntegratedLufs();
    INFO("LF-HPF attenuation (1k - 20Hz) = " << attenuation << " LU");
    REQUIRE(attenuation > 10.0f);
}

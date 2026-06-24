// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_dsp_metering/MeterBallistics.h>

#include <algorithm>
#include <vector>

namespace bws::ui
{

// Framework-neutral correlation meter ballistics. advance() runs once per UI tick;
// newData marks ticks carrying a fresh correlation sample. The smoother, peak
// capture, histogram and mono-safe latch advance on data (using the elapsed
// time since the previous sample); the peak release and hold countdown advance
// every tick. The histogram is a per-sample value histogram and is only touched
// when histogramEnabled.
struct CorrelationMeterModel
{
    struct Config
    {
        float smoothingTauMs {205.0f};
        float peakHoldSeconds {2.0f};
        float peakReleaseTauMs {325.0f};
        float monoSafeSeconds {1.0f};
        float monoSafeThreshold {0.5f};
        float histogramDecay {0.995f};
        int binCount {41};
        bool histogramEnabled {false};
    };

    float smoothed {1.0f};
    float peak {1.0f};
    float peakHoldRemaining {0.0f};
    float monoSafeAccum {0.0f};
    float timeSinceData {0.0f};
    std::vector<float> histogram;

    void reset(int binCount)
    {
        smoothed = 1.0f;
        peak = 1.0f;
        peakHoldRemaining = 0.0f;
        monoSafeAccum = 0.0f;
        timeSinceData = 0.0f;
        histogram.assign(static_cast<size_t>(binCount), 0.0f);
    }

    void advance(float raw, bool newData, float dtSeconds, const Config& cfg)
    {
        timeSinceData += dtSeconds;

        if (newData)
        {
            const float dData = timeSinceData;
            timeSinceData = 0.0f;

            smoothed = audio::MeterBallistics::easeToward(smoothed, raw, cfg.smoothingTauMs, dData);

            if (raw < peak)
            {
                peak = raw;
                peakHoldRemaining = cfg.peakHoldSeconds;
            }

            if (cfg.histogramEnabled)
            {
                const int bin = correlationToBin(smoothed, cfg.binCount);
                histogram[static_cast<size_t>(bin)] += 1.0f;
                normalizeHistogram(cfg.histogramDecay);
            }

            monoSafeAccum = (smoothed > cfg.monoSafeThreshold) ? monoSafeAccum + dData : 0.0f;
        }

        if (peakHoldRemaining > 0.0f)
            peakHoldRemaining -= dtSeconds;
        else if (peak < smoothed)
            peak = audio::MeterBallistics::easeToward(peak, smoothed, cfg.peakReleaseTauMs, dtSeconds);
    }

    [[nodiscard]] bool monoSafe(const Config& cfg) const { return monoSafeAccum > cfg.monoSafeSeconds; }

    [[nodiscard]] static int correlationToBin(float correlation, int binCount)
    {
        const float normalized = (correlation + 1.0f) * 0.5f;
        const int bin = static_cast<int>(normalized * static_cast<float>(binCount - 1));
        return std::clamp(bin, 0, binCount - 1);
    }

private:
    void normalizeHistogram(float decay)
    {
        const float maxVal = *std::max_element(histogram.begin(), histogram.end());
        if (maxVal > 1.0f)
        {
            for (auto& bin : histogram)
                bin /= maxVal;
        }
        for (auto& bin : histogram)
            bin *= decay;
    }
};

} // namespace bws::ui

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include <bw_dsp_metering/Bs1770Meter.h>
#include <bw_dsp_metering/EnvelopeFollower.h>
#include <bw_dsp_metering/LoudnessMeter.h>
#include <bw_dsp_metering/LoudnessRange.h>
#include <bw_dsp_metering/MeterBallistics.h>
#include <bw_dsp_metering/MidSideProcessor.h>
#include <bw_dsp_metering/TruePeakMeter.h>
#include <bw_dsp_metering/detail/LoudnessAnalyticsBridge.h>

int main()
{
    bws::audio::Bs1770Meter meter;
    meter.prepare(48000.0, 2);

    const float eased = bws::audio::MeterBallistics::easeToward(0.0f, 1.0f, 100.0f, 0.01f);
    return eased > 0.0f ? 0 : 1;
}

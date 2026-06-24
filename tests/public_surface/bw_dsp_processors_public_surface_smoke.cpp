// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include <bw_dsp_processors/ChannelSolo.h>
#include <bw_dsp_processors/ChannelSwap.h>
#include <bw_dsp_processors/MonoSum.h>
#include <bw_dsp_processors/StereoBalancer.h>
#include <bw_dsp_processors/StereoLevelMeter.h>
#include <bw_dsp_processors/StereoWidth.h>

int main()
{
    bws::dsp::MonoSum mono;
    mono.prepare(48000.0, 64);
    mono.setEnabled(true);
    return mono.isEnabled() ? 0 : 1;
}

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include <bw_dsp_core/ChainSpec.h>
#include <bw_dsp_core/DspConstants.h>
#include <bw_dsp_core/PhaseInverter.h>
#include <bw_dsp_core/ThreadMap.h>
#include <bw_dsp_core/TptOnePole.h>
#include <bw_dsp_core/concepts/DspConcepts.h>

int main()
{
    bws::dsp::PhaseInverter inverter;
    inverter.prepare(48000.0, 64);
    inverter.setInvertLeft(true);
    return inverter.isLeftInverted() ? 0 : 1;
}

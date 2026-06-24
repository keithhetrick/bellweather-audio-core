// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include <bw_audio_types/BufferView.h>
#include <bw_audio_types/BwsBiquadFilter.h>
#include <bw_audio_types/BwsLinearSmoothedValue.h>
#include <bw_audio_types/ConstBufferView.h>
#include <bw_audio_types/OwnedAudioBuffer.h>
#include <bw_audio_types/ParamSnapshot.h>
#include <bw_audio_types/ProcessContext.h>
#include <bw_audio_types/RtSmoothedParam.h>
#include <bw_audio_types/TransportState.h>

int main()
{
    bws::domain::OwnedAudioBuffer buffer;
    buffer.setSize(2, 8);
    buffer.setSample(0, 0, 1.0f);

    bws::domain::BufferView view {buffer.getArrayOfWritePointers(), buffer.getNumChannels(), buffer.getNumSamples()};

    bws::domain::ProcessContext context {};
    context.buffer = view;

    return view.isValid() && buffer.getSample(0, 0) == 1.0f ? 0 : 1;
}

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_audio_types/TransportState.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace bws::adapters
{

/**
 * Reads host transport info into a domain TransportState.
 *
 * Called once per processBlock() to populate ProcessContext.transport.
 * This is the last JUCE-touching translation for transport data.
 * Plugins that don't use transport can skip this call entirely.
 */
class JuceTransportReader
{
public:
    static bws::domain::TransportState read(juce::AudioProcessor& processor, int numSamples) noexcept
    {
        bws::domain::TransportState state;
        state.sampleRate = processor.getSampleRate();
        state.samplesPerBlock = numSamples;

        if (auto* ph = processor.getPlayHead())
        {
            if (auto pos = ph->getPosition())
            {
                if (auto bpm = pos->getBpm())
                    state.bpm = *bpm;
                if (auto ppq = pos->getPpqPosition())
                    state.ppqPosition = *ppq;
                state.isPlaying = pos->getIsPlaying();
                state.samplePosition = pos->getTimeInSamples().orFallback(0);
            }
        }

        return state;
    }
};

} // namespace bws::adapters

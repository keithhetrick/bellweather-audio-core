// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <bw_ports/IPluginParameter.h>
#include <bw_audio_types/BufferView.h>
#include <bw_audio_types/ProcessContext.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace bws::domain
{

/**
 * Driven port: an audio plugin processor.
 *
 * Replaces juce::Processor& in framework-agnostic test/validation
 * code paths. The JUCE adapter (JucePluginAdapter in bw_juce_adapters)
 * wraps juce::Processor&. To target another host format, write a single
 * new adapter - code consuming IPlugin needs no changes.
 *
 * Contract:
 * - prepare(sampleRate, maxBlockSize): configure processing before audio begins.
 * - reset(): clear internal state (envelope smoothers, filter memories).
 * - process(BufferView): run the plugin on the supplied non-owning buffer.
 *   MIDI is not modeled here. Adapters may discard it or expose a separate
 *   domain MIDI type; juce::MidiBuffer does not cross this boundary.
 * - getState/setState: opaque byte-level serialization round-trip.
 * - getParameters: stable set, populated by the adapter constructor.
 *   Pointers are owned by the adapter; valid for its lifetime.
 * - findParameter(id): O(n) lookup by stable parameter ID.
 * - Channel queries reflect the configured bus layout. Sidechain count
 *   = getTotalNumInputChannels() - getMainInputChannels().
 */
struct IPlugin
{
    virtual ~IPlugin() = default;

    virtual void prepare(double sampleRate, int maxBlockSize) = 0;
    virtual void reset() = 0;

    /// Process audio with full context (params + transport + buffer).
    /// Preferred path for context-aware processing.
    virtual void process(const ProcessContext& ctx) = 0;

    /// Legacy: buffer-only processing. Use process(ProcessContext) for new code.
    [[deprecated("Use process(const ProcessContext&)")]]
    virtual void process(BufferView buffer) = 0;

    virtual void getState(std::vector<uint8_t>& dest) const = 0;
    virtual void setState(const std::uint8_t* data, std::size_t numBytes) = 0;

    virtual std::vector<IPluginParameter*> getParameters() = 0;
    virtual IPluginParameter* findParameter(const std::string& paramId) = 0;

    virtual int getTotalNumInputChannels() const = 0;
    virtual int getTotalNumOutputChannels() const = 0;
    virtual int getMainInputChannels() const = 0;
    virtual int getMainOutputChannels() const = 0;
};

} // namespace bws::domain

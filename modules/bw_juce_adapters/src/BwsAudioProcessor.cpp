// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_juce_adapters/BwsAudioProcessor.h"
#include "bw_juce_adapters/JuceBufferAdapter.h"
#include "JuceTransportReader.h"
#include "ValueTreeRetainer.h"

namespace bws
{

BwsAudioProcessor::BusesProperties BwsAudioProcessor::makeBusesPropertiesForPolicy(
    adapters::PluginChannelLayoutPolicy policy)
{
    using juce::AudioChannelSet;

    switch (policy)
    {
    case adapters::PluginChannelLayoutPolicy::MatchedMonoStereoSidechain:
        return BusesProperties()
            .withInput("Input", AudioChannelSet::stereo(), true)
            .withInput("Sidechain", AudioChannelSet::mono(), false)
            .withOutput("Output", AudioChannelSet::stereo(), true);

    case adapters::PluginChannelLayoutPolicy::MatchedMonoStereo:
    case adapters::PluginChannelLayoutPolicy::StereoOnly:
        return BusesProperties()
            .withInput("Input", AudioChannelSet::stereo(), true)
            .withOutput("Output", AudioChannelSet::stereo(), true);

    case adapters::PluginChannelLayoutPolicy::MonoToStereo:
        return BusesProperties()
            .withInput("Input", AudioChannelSet::mono(), true)
            .withOutput("Output", AudioChannelSet::stereo(), true);
    }

    return BusesProperties()
        .withInput("Input", AudioChannelSet::stereo(), true)
        .withOutput("Output", AudioChannelSet::stereo(), true);
}

BwsAudioProcessor::BwsAudioProcessor()
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{}

BwsAudioProcessor::BwsAudioProcessor(const BusesProperties& ioLayouts)
    : juce::AudioProcessor(ioLayouts)
{}

void BwsAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Ensure JUCE's internal sample rate is set so getSampleRate() works correctly.
    // Host wrappers call this automatically, but direct calls (tests, standalone)
    // bypass the host wrapper, leaving currentPlaybackSampleRate at 0.
    setRateAndBufferSizeDetails(sampleRate, samplesPerBlock);

    // Cache channel counts (avoids virtual calls in processBlock - 0.3% CPU saving)
    cachedInCh.store(getTotalNumInputChannels());
    cachedOutCh.store(getTotalNumOutputChannels());
    cachedSampleRate.store(static_cast<float>(sampleRate));
    cachedBlockSize.store(samplesPerBlock);
    cachedMainBusCh_.store(getMainBusNumInputChannels());

    // Call plugin-specific preparation
    prepareToPlayImpl(sampleRate, samplesPerBlock);
}

void BwsAudioProcessor::releaseResources()
{
    releaseResourcesImpl();
}

void BwsAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    // RT-safety guard (prevents logging, allocations, syscalls in audio thread)
    bws::rt::AudioThreadScope audioScope;

    // Denormal handling (0.4% CPU saving)
    juce::ScopedNoDenormals noDenormals;

    // Base class owns ProcessContext assembly.
    // Plugin owns snapshot via buildParamSnapshot(). Wrong path is a compile error.
    bws::domain::ProcessContext domainCtx;
    domainCtx.buffer = bws::adapters::toBufferView(buffer);
    domainCtx.transport = bws::adapters::JuceTransportReader::read(*this, buffer.getNumSamples());
    domainCtx.params = buildParamSnapshot();

    processBlockImpl(domainCtx);
}

//==============================================================================
// CENTRALIZED STATE MANAGEMENT
//==============================================================================

void BwsAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Get plugin's APVTS
    auto* apvts = getApvtsPtr();
    if (!apvts)
    {
        // Plugin must implement getApvtsPtr() - fall back to empty state
        jassertfalse;
        return;
    }

    // Create XML from APVTS state
    auto xml = apvts->copyState().createXml();
    if (!xml)
        return;

    // Version stamp for parameter migrations.
    xml->setAttribute("bwsStateVersion", 1);

    // 1. Save preset metadata (if plugin has preset manager)
    if (auto* presetMgr = getPresetManagerPtr())
    {
        auto meta = presetMgr->getPresetMetadata();
        if (meta.has_value())
        {
            xml->setAttribute("currentPresetName", juce::String(meta->name.data(), meta->name.size()));
            xml->setAttribute("currentPresetCategory", juce::String(meta->category.data(), meta->category.size()));
            xml->setAttribute("presetWasModified", meta->modified);
            if (!meta->stableId.empty())
                xml->setAttribute("currentPresetId", juce::String(meta->stableId.data(), meta->stableId.size()));
        }
    }

    // A/B slots persist via the substrate's per-parameter `valueB` and
    // root `abActiveSlot` attributes; no nested-blob attribute is emitted.

    // 3. Save plugin-specific custom state (if any)
    saveCustomState(*xml);

    // Serialize to binary
    copyXmlToBinary(*xml, destData);
}

void BwsAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0)
    {
        return;
    }

    // Trial mode: reset to defaults on session restore.
    if (isTrialMode() && !presetLoadInProgress_)
    {
        resetToDefaultState();
        return;
    }

    // Get plugin's APVTS
    auto* apvts = getApvtsPtr();
    if (!apvts)
    {
        // Plugin must implement getApvtsPtr() - cannot restore state
        jassertfalse;
        return;
    }

    // Parse XML from binary
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (!xml)
        return;
    if (!xml->hasTagName(apvts->state.getType().toString()))
        return;

    // Read version (default to 1 for pre-versioned states)
    const int stateVersion = xml->getIntAttribute("bwsStateVersion", 1);
    lastLoadedStateVersion_ = stateVersion;

    // Convert to ValueTree for potential migration
    juce::ValueTree state = juce::ValueTree::fromXml(*xml);
    if (!state.isValid() || state.getType() != apvts->state.getType())
        return;
    // Strip invalid children gracefully instead of rejecting ALL state.
    // One corrupt PARAM should not nuke the other 41 valid parameters.
    // Iterate in reverse so removeChild doesn't invalidate indices.
    for (int i = state.getNumChildren() - 1; i >= 0; --i)
    {
        const auto child = state.getChild(i);
        if (child.getType() != juce::Identifier("PARAM") || !child.hasProperty("id") || !child.hasProperty("value"))
        {
            state.removeChild(i, nullptr);
        }
    }

    // Allow plugins to migrate old state formats (e.g., bool->float conversions)
    migrateValueTreeState(state, *xml);

    // Strip non-APVTS properties from ValueTree before replacing state.
    // ValueTree::fromXml() converts ALL XML attributes into properties, including
    // A/B comparison data, preset metadata, and version stamps. These must not
    // persist in the APVTS or they will be re-serialized on every
    // getStateInformation() call, contaminating the root with non-parameter
    // properties. The legacy strips below stay as defense-in-depth for older
    // saved sessions that may still carry those attributes.
    state.removeProperty("abComparisonState", nullptr); // legacy; kept for back-compat
    state.removeProperty("abCurrentSlot", nullptr);     // legacy; kept for back-compat
    state.removeProperty("abActiveSlot", nullptr);      // root attr; not an APVTS param
    state.removeProperty("bwsStateVersion", nullptr);
    state.removeProperty("presetName", nullptr);      // legacy name
    state.removeProperty("presetIsFactory", nullptr); // legacy name
    state.removeProperty("currentPresetName", nullptr);
    state.removeProperty("currentPresetCategory", nullptr);
    state.removeProperty("currentPresetId", nullptr);
    state.removeProperty("presetWasModified", nullptr);

    // 1. Restore preset state (if plugin has preset manager)
    //    Serialize full XML to a neutral blob before crossing the seam.
    //    WeatherPresetManager parses it internally using JUCE - JUCE XML
    //    stays on the implementation side, never crosses the bw_ports boundary.
    auto* presetMgr = getPresetManagerPtr();
    struct PresetRestoreScope
    {
        explicit PresetRestoreScope(bws::IPresetStatePersistence* manager)
            : manager_(manager)
        {
            if (manager_ != nullptr)
                manager_->beginPresetStateRestore();
        }

        ~PresetRestoreScope()
        {
            if (manager_ != nullptr)
                manager_->endPresetStateRestore();
        }

        bws::IPresetStatePersistence* manager_ = nullptr;
    } presetRestoreScope(presetMgr);

    // Keep the old state alive to prevent bad-free on JUCE's constexpr
    // emptyString when the old ValueTree SharedObject is destroyed.
    bws::adapters::ValueTreeRetainer::instance().retain(apvts->state);

    // Restore APVTS state
    apvts->replaceState(state);

    if (presetMgr != nullptr)
    {
        juce::MemoryBlock presetBytes;
        copyXmlToBinary(*xml, presetBytes);
        bws::domain::BwStateBlob presetBlob {std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(presetBytes.getData()), presetBytes.getSize())};
        const auto presetRestoreResult = presetMgr->restorePresetState(presetBlob);
        if (!presetRestoreResult)
        {
            jassertfalse; // non-fatal: parameter state already applied
        }
    }

    // Legacy saved sessions carrying `abComparisonState` are migrated into
    // the substrate slot pair by the plugin's state-restore path.

    // 3. Restore plugin-specific custom state (if any)
    restoreCustomState(*xml);
}

//==============================================================================
// TRIAL MODE SUPPORT
//==============================================================================

void BwsAudioProcessor::resetToDefaultState()
{
    auto* apvts = getApvtsPtr();
    if (!apvts)
    {
        return;
    }

    for (auto* param : getParameters())
    {
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(param))
        {
            ranged->setValueNotifyingHost(ranged->getDefaultValue());
        }
    }
}

} // namespace bws

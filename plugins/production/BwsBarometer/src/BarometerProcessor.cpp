// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "BarometerProcessor.h"
#include "BarometerOptionalHooks.h" // opaque optional-hook handle (complete type here)
#include "ParamId.h"
#include "BarometerEditor.h"
#include <bw_audio_types/ProcessContext.h>
#include <bw_juce_adapters/JuceBufferAdapter.h>
#include <bw_juce_adapters/PluginBusLayoutPolicy.h>
#include <bw_rt/RtSafety.h> // BWS_NONBLOCKING_UNSAFE - vendor-call suppression at adapter layer
#include <chrono>
#include <cmath>

namespace bws::barometer
{
namespace
{
// WARNING: this guard is silently eliminated by -ffast-math /
// -ffinite-math-only - those flags let the compiler treat NaN/Inf as
// impossible and fold the isfinite() check away. Never enable them globally.
void sanitizeNonFiniteSamples(juce::AudioBuffer<float>& buffer) noexcept BWS_NONBLOCKING
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    for (int channel = 0; channel < numChannels; ++channel)
    {
        float* data = buffer.getWritePointer(channel);
        for (int sample = 0; sample < numSamples; ++sample)
        {
            if (!std::isfinite(data[sample]))
                data[sample] = 0.0f;
        }
    }
}
} // namespace

// =============================================================================
// Processor Implementation
// =============================================================================

// The optional-hooks handle is a single opaque pointer regardless of build
// configuration, so this processor's layout is stable. The assert codifies that
// the handle is exactly one pointer.
static_assert(sizeof(std::unique_ptr<BarometerOptionalHooks>) == sizeof(void*),
              "BarometerOptionalHooks must be held by a single plain pointer so it "
              "contributes a stable one-pointer footprint");

BarometerProcessor::BarometerProcessor()
    : bws::BwsAudioProcessor(
          bws::BwsAudioProcessor::makeBusesPropertiesForPolicy(bws::adapters::currentPluginChannelLayoutPolicy()))
    , apvts_(*this, &undoManager_, "Parameters", createParameterLayout())
{
    // Create Weather preset manager (initializes + creates factory presets)
    presetManager_ = std::make_unique<BarometerWeatherPresetManager>(*this);

    // Migrate legacy user presets if they exist
    presetManager_->migrateLegacyUserPresets();

    // Composition root for the optional commercial hooks, behind an opaque handle
    // so this processor's layout does not depend on build configuration. The
    // open-source build gets an empty handle. The factory reads only wrapperType +
    // JucePlugin_* constants.
    optionalHooks_ = makeBarometerOptionalHooks(wrapperType);
}

BarometerProcessor::~BarometerProcessor()
{
    stopLoudnessAnalyticsWorker();
}

bool BarometerProcessor::hasOptionalHooks() const noexcept
{
    return optionalHooks_ && optionalHooks_->hasOptionalHooks();
}


void BarometerProcessor::startLoudnessAnalyticsWorker()
{
    stopLoudnessAnalyticsWorker();
    loudnessAnalyticsWorker_ = std::jthread([this](std::stop_token stopToken) {
        while (!stopToken.stop_requested())
        {
            lufsMeter_.serviceAnalytics();
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        while (lufsMeter_.serviceAnalytics())
        {}
    });
}

void BarometerProcessor::stopLoudnessAnalyticsWorker() noexcept
{
    if (loudnessAnalyticsWorker_.joinable())
    {
        loudnessAnalyticsWorker_.request_stop();
        loudnessAnalyticsWorker_.join();
    }
}

bool BarometerProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return bws::adapters::isBusesLayoutSupported(bws::adapters::currentPluginChannelLayoutPolicy(), layouts);
}

juce::AudioProcessorValueTreeState::ParameterLayout BarometerProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Gain parameter: -96dB to +24dB, default 0dB
    // Using skew factor to give more resolution around unity (0 dB)
    auto gainRange = juce::NormalisableRange<float>(kGainMinDb, kGainMaxDb, 0.1f);
    gainRange.setSkewForCentre(0.0f); // Center the knob at 0 dB

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {kGainParamId, 1}, "Gain", gainRange, kGainDefaultDb,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    // nvert Left: boolean (0 = normal, 1 = inverted)
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID {kPhaseInvertLParamId, 1}, "Phase L",
                                                                false // Default: normal phase
                                                                ));

    // nvert Right: boolean (0 = normal, 1 = inverted)
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID {kPhaseInvertRParamId, 1}, "Phase R",
                                                                false // Default: normal phase
                                                                ));

    // ink: boolean (0 = unlinked, 1 = linked)
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID {kPhaseLinkParamId, 1}, "Phase Link",
                                                                true // Default: linked
                                                                ));

    // Balance parameter: -100 (full left) to +100 (full right), default 0 (center)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {kBalanceParamId, 1}, "Balance", juce::NormalisableRange<float>(-100.0f, 100.0f, 1.0f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // DC Filter: boolean toggle for 5Hz high-pass filter (removes DC offset)
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID {kDcFilterParamId, 1}, "DC Filter",
                                                                false // Default: off
                                                                ));

    // Mono: boolean toggle to sum stereo to mono
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID {kMonoParamId, 1}, "Mono",
                                                                false // Default: off
                                                                ));

    // Stereo Width: 0.0 (mono) to 2.0 (wide), default 1.0 (unchanged)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {kWidthParamId, 1}, "Width",
                                                                 juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f),
                                                                 1.0f, // Default: 100% (unchanged)
                                                                 juce::AudioParameterFloatAttributes().withLabel("%")));

    // Swap L/R: boolean toggle to swap left and right channels
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID {kSwapLRParamId, 1}, "Swap L/R",
                                                                false // Default: off
                                                                ));

    // Solo Left: boolean toggle to solo left channel
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID {kSoloLParamId, 1}, "Solo L",
                                                                false // Default: off
                                                                ));

    // Solo Right: boolean toggle to solo right channel
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID {kSoloRParamId, 1}, "Solo R",
                                                                false // Default: off
                                                                ));

    // Output Trim: -12 dB to +12 dB, default 0 dB (final gain stage)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {kOutputTrimParamId, 1}, "Output Trim", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f, // Default: 0 dB (unity)
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    return {params.begin(), params.end()};
}

// Bind cached param pointers.
void BarometerProcessor::BarometerParamPtrs::bind(juce::AudioProcessorValueTreeState& apvts) noexcept
{
    pGain = apvts.getRawParameterValue("gain");
    pPhaseL = apvts.getRawParameterValue("phaseL");
    pPhaseR = apvts.getRawParameterValue("phaseR");
    pPhaseLink = apvts.getRawParameterValue("phaseLink");
    pBalance = apvts.getRawParameterValue("balance");
    pDcFilter = apvts.getRawParameterValue("dc_filter");
    pMono = apvts.getRawParameterValue("mono");
    pWidth = apvts.getRawParameterValue("width");
    pSwapLR = apvts.getRawParameterValue("swapLR");
    pSoloL = apvts.getRawParameterValue("soloL");
    pSoloR = apvts.getRawParameterValue("soloR");
    pOutTrim = apvts.getRawParameterValue("outTrim");

    jassert(pGain && pPhaseL && pPhaseR && pPhaseLink && pBalance && pDcFilter && pMono && pWidth && pSwapLR &&
            pSoloL && pSoloR && pOutTrim);
}

// Build plain-value snapshot from cached pointers.
bws::domain::ParamSnapshot BarometerProcessor::buildParamSnapshot() const noexcept BWS_NONBLOCKING
{
    using P = bws::barometer::ParamId;
    bws::domain::ParamSnapshot snap;

    snap.set(static_cast<uint32_t>(P::Gain), BarometerParamPtrs::load(paramPtrs_.pGain));
    snap.set(static_cast<uint32_t>(P::PhaseL), BarometerParamPtrs::load(paramPtrs_.pPhaseL));
    snap.set(static_cast<uint32_t>(P::PhaseR), BarometerParamPtrs::load(paramPtrs_.pPhaseR));
    snap.set(static_cast<uint32_t>(P::PhaseLink), BarometerParamPtrs::load(paramPtrs_.pPhaseLink));
    snap.set(static_cast<uint32_t>(P::Balance), BarometerParamPtrs::load(paramPtrs_.pBalance));
    snap.set(static_cast<uint32_t>(P::DcFilter), BarometerParamPtrs::load(paramPtrs_.pDcFilter));
    snap.set(static_cast<uint32_t>(P::Mono), BarometerParamPtrs::load(paramPtrs_.pMono));
    snap.set(static_cast<uint32_t>(P::Width), BarometerParamPtrs::load(paramPtrs_.pWidth));
    snap.set(static_cast<uint32_t>(P::SwapLR), BarometerParamPtrs::load(paramPtrs_.pSwapLR));
    snap.set(static_cast<uint32_t>(P::SoloL), BarometerParamPtrs::load(paramPtrs_.pSoloL));
    snap.set(static_cast<uint32_t>(P::SoloR), BarometerParamPtrs::load(paramPtrs_.pSoloR));
    snap.set(static_cast<uint32_t>(P::OutTrim), BarometerParamPtrs::load(paramPtrs_.pOutTrim));

    return snap;
}

void BarometerProcessor::prepareToPlayImpl(double sampleRate, int samplesPerBlock)
{
    stopLoudnessAnalyticsWorker();
    paramPtrs_.bind(apvts_); // cache param pointers once

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    // Prepare JUCE DSP processors
    gainProcessor_.prepare(spec);
    gainProcessor_.setRampDurationSeconds(bws::tokens::barometer::animation::GAIN_RAMP_SEC);

    // Prepare DC filter (5Hz high-pass)
    dcFilter_.prepare(spec);
    *dcFilter_.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 5.0f);

    // Prepare modular DSP processors
    phaseInverter_.prepare(sampleRate, samplesPerBlock);
    stereoBalancer_.prepare(sampleRate, samplesPerBlock);
    stereoWidth_.prepare(sampleRate, samplesPerBlock);
    monoSum_.prepare(sampleRate, samplesPerBlock);
    channelSwap_.prepare(sampleRate, samplesPerBlock);
    channelSolo_.prepare(sampleRate, samplesPerBlock);

    // Prepare meters
    inputMeter_.prepare(sampleRate);
    outputMeter_.prepare(sampleRate);
    const int numCh = getTotalNumOutputChannels();
    lufsMeter_.prepare(sampleRate, numCh);
    truePeakMeter_.prepare(sampleRate, numCh);
    startLoudnessAnalyticsWorker();

    // RMS integration coefficient (~300ms window)
    rmsCoeff_ = std::exp(-1.0f / (bws::tokens::barometer::animation::RMS_WINDOW_SEC * static_cast<float>(sampleRate)));
}

void BarometerProcessor::releaseResourcesImpl()
{
    stopLoudnessAnalyticsWorker();
    // Reset JUCE DSP processors
    gainProcessor_.reset();
    dcFilter_.reset();

    // Reset modular DSP processors
    phaseInverter_.reset();
    stereoBalancer_.reset();
    stereoWidth_.reset();
    monoSum_.reset();
    channelSwap_.reset();
    channelSolo_.reset();

    // Reset meters
    inputMeter_.reset();
    outputMeter_.reset();
    lufsMeter_.reset();
    truePeakMeter_.reset();

    // Reset RMS + correlation state
    smoothedInputRmsL_ = 0.0f;
    smoothedInputRmsR_ = 0.0f;
    smoothedOutputRmsL_ = 0.0f;
    smoothedOutputRmsR_ = 0.0f;
    inputRmsLeftDb_.store(-100.0f, std::memory_order_relaxed);
    inputRmsRightDb_.store(-100.0f, std::memory_order_relaxed);
    outputRmsLeftDb_.store(-100.0f, std::memory_order_relaxed);
    outputRmsRightDb_.store(-100.0f, std::memory_order_relaxed);
    stereoCorrelation_.store(1.0f, std::memory_order_relaxed);
}

void BarometerProcessor::processBlockImpl(const bws::domain::ProcessContext& ctx) noexcept BWS_NONBLOCKING
{
    // ProcessContext assembled by the base class. Wrap covers
    // toJuceBufferView() + the local's implicit ~AudioBuffer dtor at
    // scope-end (FEA dtor warning fires at declaration line; thin
    // non-owning view, trivial dtor).
    BWS_NONBLOCKING_UNSAFE(auto buffer = bws::adapters::toJuceBufferView(ctx.buffer);)
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    if (numSamples <= 0 || numChannels <= 0)
    {
        return;
    }
    const auto& domainCtx = ctx; // alias for existing domainCtx.params references

    // Host-fault hardening: recover immediately from hostile NaN/Inf input.
    sanitizeNonFiniteSamples(buffer);

    // Capture input levels BEFORE any processing
    const float* inputLeft = buffer.getReadPointer(0);
    const float* inputRight = (numChannels >= 2) ? buffer.getReadPointer(1) : nullptr;
    inputMeter_.processSamples(inputLeft, inputRight, numSamples);

    // Input RMS (RT-safe: just arithmetic)
    {
        float sumL = 0.0f, sumR = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            sumL += inputLeft[i] * inputLeft[i];
            const float r = inputRight ? inputRight[i] : inputLeft[i];
            sumR += r * r;
        }
        const float blockRmsL = std::sqrt(sumL / static_cast<float>(numSamples));
        const float blockRmsR = std::sqrt(sumR / static_cast<float>(numSamples));
        const float decay = std::pow(rmsCoeff_, static_cast<float>(numSamples));
        smoothedInputRmsL_ = blockRmsL + (smoothedInputRmsL_ - blockRmsL) * decay;
        smoothedInputRmsR_ = blockRmsR + (smoothedInputRmsR_ - blockRmsR) * decay;
        inputRmsLeftDb_.store(smoothedInputRmsL_ > 0.0f ? 20.0f * std::log10(smoothedInputRmsL_) : -100.0f,
                              std::memory_order_relaxed);
        inputRmsRightDb_.store(smoothedInputRmsR_ > 0.0f ? 20.0f * std::log10(smoothedInputRmsR_) : -100.0f,
                               std::memory_order_relaxed);
    }

    // Read params from snapshot (no APVTS string lookups on audio thread).
    using P = bws::barometer::ParamId;
    const bool dcFilterEnabled = domainCtx.params.get(static_cast<uint32_t>(P::DcFilter)) > 0.5f;

    if (dcFilterEnabled)
    {
        // JUCE ProcessorDuplicator<IIR::Filter> - RT-safe per JUCE production usage;
        // IIR coefficient state updates via cached structures, no heap, no lock.
        BWS_NONBLOCKING_UNSAFE(juce::dsp::AudioBlock<float> dcBlock(buffer);
                               juce::dsp::ProcessContextReplacing<float> dcContext(dcBlock);
                               dcFilter_.process(dcContext);)
    }

    const float gainDb = domainCtx.params.get(static_cast<uint32_t>(P::Gain));
    const bool invertL = domainCtx.params.get(static_cast<uint32_t>(P::PhaseL)) > 0.5f;
    const bool invertR = domainCtx.params.get(static_cast<uint32_t>(P::PhaseR)) > 0.5f;
    const float balance = domainCtx.params.get(static_cast<uint32_t>(P::Balance));
    const float width = domainCtx.params.get(static_cast<uint32_t>(P::Width));
    const bool monoEnabled = domainCtx.params.get(static_cast<uint32_t>(P::Mono)) > 0.5f;
    const bool swapLR = domainCtx.params.get(static_cast<uint32_t>(P::SwapLR)) > 0.5f;
    const bool soloL = domainCtx.params.get(static_cast<uint32_t>(P::SoloL)) > 0.5f;
    const bool soloR = domainCtx.params.get(static_cast<uint32_t>(P::SoloR)) > 0.5f;

    // Convert dB to linear gain (treat -96 dB as silence)
    const float linearGain = (gainDb <= kGainMinDb) ? 0.0f : juce::Decibels::decibelsToGain(gainDb);

    // Apply gain with smoothing
    gainProcessor_.setGainLinear(linearGain);

    // juce::dsp::Gain - RT-safe per JUCE production usage; in-place gain ramp
    // with cached smoother state, no heap, no lock.
    BWS_NONBLOCKING_UNSAFE(juce::dsp::AudioBlock<float> block(buffer);
                           juce::dsp::ProcessContextReplacing<float> context(block); gainProcessor_.process(context);)

    // Alias for downstream DSP modules - single toBufferView() via domainCtx
    auto& bv = domainCtx.buffer;

    // Apply per-channel phase inversion (modular)
    phaseInverter_.setInvertLeft(invertL);
    phaseInverter_.setInvertRight(invertR);
    phaseInverter_.process(bv);

    // Apply balance/pan (modular: -100 to +100 maps to -1.0 to +1.0)
    stereoBalancer_.setBalance(balance / 100.0f);
    stereoBalancer_.process(bv);

    // Apply stereo width (modular)
    stereoWidth_.setWidth(width);
    stereoWidth_.process(bv);

    // Apply mono summing (modular)
    monoSum_.setEnabled(monoEnabled);
    monoSum_.process(bv);

    // Swap L/R channels (modular)
    channelSwap_.setEnabled(swapLR);
    channelSwap_.process(bv);

    // Solo L/R (modular)
    channelSolo_.setSoloLeft(soloL);
    channelSolo_.setSoloRight(soloR);
    channelSolo_.process(bv);

    // Output Trim - LAST in signal chain (after all processing, before meter)
    const float trimDb = domainCtx.params.get(static_cast<uint32_t>(P::OutTrim));
    if (std::abs(trimDb) > 0.05f) // Only apply if not near unity
    {
        const float trimGain = juce::Decibels::decibelsToGain(trimDb);
        // juce::AudioBuffer::applyGain - in-place scalar multiply per channel; RT-safe.
        BWS_NONBLOCKING_UNSAFE(buffer.applyGain(trimGain);)
    }

    // Ensure a fault in any processing stage cannot leak non-finite output.
    sanitizeNonFiniteSamples(buffer);

    // Update output meter (after all processing)
    const float* leftChannel = buffer.getReadPointer(0);
    const float* rightChannel = (numChannels >= 2) ? buffer.getReadPointer(1) : nullptr;
    outputMeter_.processSamples(leftChannel, rightChannel, numSamples);

    // Output RMS + Stereo Correlation (RT-safe: just arithmetic)
    {
        float sumL = 0.0f, sumR = 0.0f;
        double corrLR = 0.0, corrL2 = 0.0, corrR2 = 0.0;
        for (int i = 0; i < numSamples; ++i)
        {
            const float l = leftChannel[i];
            const float r = rightChannel ? rightChannel[i] : l;
            sumL += l * l;
            sumR += r * r;
            corrLR += static_cast<double>(l) * static_cast<double>(r);
            corrL2 += static_cast<double>(l) * static_cast<double>(l);
            corrR2 += static_cast<double>(r) * static_cast<double>(r);
        }

        // Output RMS
        const float blockRmsL = std::sqrt(sumL / static_cast<float>(numSamples));
        const float blockRmsR = std::sqrt(sumR / static_cast<float>(numSamples));
        const float decay = std::pow(rmsCoeff_, static_cast<float>(numSamples));
        smoothedOutputRmsL_ = blockRmsL + (smoothedOutputRmsL_ - blockRmsL) * decay;
        smoothedOutputRmsR_ = blockRmsR + (smoothedOutputRmsR_ - blockRmsR) * decay;
        outputRmsLeftDb_.store(smoothedOutputRmsL_ > 0.0f ? 20.0f * std::log10(smoothedOutputRmsL_) : -100.0f,
                               std::memory_order_relaxed);
        outputRmsRightDb_.store(smoothedOutputRmsR_ > 0.0f ? 20.0f * std::log10(smoothedOutputRmsR_) : -100.0f,
                                std::memory_order_relaxed);

        // Stereo Correlation
        const double denom = std::sqrt(corrL2 * corrR2);
        const float corr = (denom > 1e-10) ? static_cast<float>(corrLR / denom) : 1.0f;
        stereoCorrelation_.store(juce::jlimit(-1.0f, 1.0f, corr), std::memory_order_relaxed);
    }

    // LUFS and True Peak metering (post all processing, measures final output)
    lufsMeter_.process(buffer);
    truePeakMeter_.process(buffer);
}

juce::AudioProcessorEditor* BarometerProcessor::createEditor()
{
    return new BarometerEditor(*this);
}

void BarometerProcessor::migrateValueTreeState(juce::ValueTree& state, const juce::XmlElement& /*xml*/)
{
    // Migrate legacy single "phase" parameter to per-channel phaseL/phaseR + phaseLink
    auto legacyChild = state.getChildWithProperty("id", kLegacyPhaseParamId);
    auto hasNewParams = state.getChildWithProperty("id", kPhaseInvertLParamId).isValid();

    if (legacyChild.isValid() && !hasNewParams)
    {
        bool legacyPhase = static_cast<float>(legacyChild.getProperty("value")) > 0.5f;

        juce::ValueTree phaseLChild("PARAM");
        phaseLChild.setProperty("id", kPhaseInvertLParamId, nullptr);
        phaseLChild.setProperty("value", legacyPhase ? 1.0f : 0.0f, nullptr);
        state.appendChild(phaseLChild, nullptr);

        juce::ValueTree phaseRChild("PARAM");
        phaseRChild.setProperty("id", kPhaseInvertRParamId, nullptr);
        phaseRChild.setProperty("value", legacyPhase ? 1.0f : 0.0f, nullptr);
        state.appendChild(phaseRChild, nullptr);

        juce::ValueTree linkChild("PARAM");
        linkChild.setProperty("id", kPhaseLinkParamId, nullptr);
        linkChild.setProperty("value", 1.0f, nullptr);
        state.appendChild(linkChild, nullptr);
    }
}

// =============================================================================
// A/B Compare Implementation
// =============================================================================

ParameterState BarometerProcessor::getCurrentState() const
{
    ParameterState state;
    state.gain = apvts_.getRawParameterValue(kGainParamId)->load();
    state.phaseInvertL = apvts_.getRawParameterValue(kPhaseInvertLParamId)->load() > 0.5f;
    state.phaseInvertR = apvts_.getRawParameterValue(kPhaseInvertRParamId)->load() > 0.5f;
    state.phaseLinkEnabled = apvts_.getRawParameterValue(kPhaseLinkParamId)->load() > 0.5f;
    state.balance = apvts_.getRawParameterValue(kBalanceParamId)->load();
    state.dcFilterEnabled = apvts_.getRawParameterValue(kDcFilterParamId)->load() > 0.5f;
    state.monoEnabled = apvts_.getRawParameterValue(kMonoParamId)->load() > 0.5f;
    state.width = apvts_.getRawParameterValue(kWidthParamId)->load();
    state.swapLR = apvts_.getRawParameterValue(kSwapLRParamId)->load() > 0.5f;
    state.soloL = apvts_.getRawParameterValue(kSoloLParamId)->load() > 0.5f;
    state.soloR = apvts_.getRawParameterValue(kSoloRParamId)->load() > 0.5f;
    state.outputTrim = apvts_.getRawParameterValue(kOutputTrimParamId)->load();
    return state;
}

void BarometerProcessor::saveCurrentToState(ParameterState& state)
{
    state = getCurrentState();
}

void BarometerProcessor::loadStateToParameters(const ParameterState& state)
{
    if (auto* gainParam = apvts_.getParameter(kGainParamId))
        gainParam->setValueNotifyingHost(gainParam->convertTo0to1(state.gain));

    if (auto* phaseLParam = apvts_.getParameter(kPhaseInvertLParamId))
        phaseLParam->setValueNotifyingHost(state.phaseInvertL ? 1.0f : 0.0f);

    if (auto* phaseRParam = apvts_.getParameter(kPhaseInvertRParamId))
        phaseRParam->setValueNotifyingHost(state.phaseInvertR ? 1.0f : 0.0f);

    if (auto* phaseLinkParam = apvts_.getParameter(kPhaseLinkParamId))
        phaseLinkParam->setValueNotifyingHost(state.phaseLinkEnabled ? 1.0f : 0.0f);

    if (auto* balanceParam = apvts_.getParameter(kBalanceParamId))
        balanceParam->setValueNotifyingHost(balanceParam->convertTo0to1(state.balance));

    if (auto* dcFilterParam = apvts_.getParameter(kDcFilterParamId))
        dcFilterParam->setValueNotifyingHost(state.dcFilterEnabled ? 1.0f : 0.0f);

    if (auto* monoParam = apvts_.getParameter(kMonoParamId))
        monoParam->setValueNotifyingHost(state.monoEnabled ? 1.0f : 0.0f);

    if (auto* widthParam = apvts_.getParameter(kWidthParamId))
        widthParam->setValueNotifyingHost(widthParam->convertTo0to1(state.width));

    if (auto* swapParam = apvts_.getParameter(kSwapLRParamId))
        swapParam->setValueNotifyingHost(state.swapLR ? 1.0f : 0.0f);

    if (auto* soloLParam = apvts_.getParameter(kSoloLParamId))
        soloLParam->setValueNotifyingHost(state.soloL ? 1.0f : 0.0f);

    if (auto* soloRParam = apvts_.getParameter(kSoloRParamId))
        soloRParam->setValueNotifyingHost(state.soloR ? 1.0f : 0.0f);

    if (auto* trimParam = apvts_.getParameter(kOutputTrimParamId))
        trimParam->setValueNotifyingHost(trimParam->convertTo0to1(state.outputTrim));
}

void BarometerProcessor::toggleAB()
{
    if (currentIsA_)
    {
        // Save current to A, switch to B
        saveCurrentToState(stateA_);
        loadStateToParameters(stateB_);
        currentIsA_ = false;
    }
    else
    {
        // Save current to B, switch to A
        saveCurrentToState(stateB_);
        loadStateToParameters(stateA_);
        currentIsA_ = true;
    }
}

void BarometerProcessor::copyAToB()
{
    // First make sure A has the latest values
    if (currentIsA_)
    {
        saveCurrentToState(stateA_);
    }

    // Copy A to B
    stateB_ = stateA_;

    // If currently on B, apply the copied state
    if (!currentIsA_)
    {
        loadStateToParameters(stateB_);
    }
}

void BarometerProcessor::copyBToA()
{
    // First make sure B has the latest values
    if (!currentIsA_)
    {
        saveCurrentToState(stateB_);
    }

    // Copy B to A
    stateA_ = stateB_;

    // If currently on A, apply the copied state
    if (currentIsA_)
    {
        loadStateToParameters(stateA_);
    }
}

} // namespace bws::barometer

// Factory function for JUCE plugin hosting
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new bws::barometer::BarometerProcessor();
}

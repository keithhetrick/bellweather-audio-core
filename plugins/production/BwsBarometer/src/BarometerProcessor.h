// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * =============================================================================
 * Processor.h - Barometer Utility Plugin Processor
 * =============================================================================
 *
 * Bellweather Studios - gain and metering utility plugin
 *
 * Parameters:
 *   - 12 APVTS parameters: gain, phase L/R/link, balance, DC filter, mono,
 *     width, swap L/R, solo L/R, and output trim.
 *
 * Features:
 *   - A/B Compare: Toggle between two parameter states
 *   - Metering: input/output levels, LUFS/LRA, true peak, PSR, correlation
 *   - Factory Presets: 10 common configurations
 *   - User Presets: CRUD with custom directory support
 *   - Signal Lock: hidden oscilloscope-style stereo stability view
 *
 * Layout: token-driven ResizableEditor base size (408 px wide by default)
 *
 * =============================================================================
 */

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "bw_juce_adapters/BwsAudioProcessor.h"
#include <bw_audio_types/ProcessContext.h>
#include <bw_audio_types/ParamSnapshot.h>
#include "BarometerWeatherPresetManager.h"
#include <bw_dsp_core/PhaseInverter.h>
#include <bw_dsp_processors/ChannelSwap.h>
#include <bw_dsp_processors/ChannelSolo.h>
#include <bw_dsp_processors/MonoSum.h>
#include <bw_dsp_processors/StereoWidth.h>
#include <bw_dsp_processors/StereoBalancer.h>
#include <bw_dsp_processors/StereoLevelMeter.h>
#include <bw_dsp_metering/LoudnessMeter.h>
#include <bw_dsp_metering/TruePeakMeter.h>
#include <bw_dsp_core/ChainSpec.h>
// Optional commercial hooks live behind an opaque handle (BarometerOptionalHooks) so the
// processor's object layout does not depend on build configuration - see
// BarometerOptionalHooks.h. Only a reference return type is named here; full types stay
// in the .cpp TUs.
#include <bw_dsp_core/ThreadMap.h>
#include <atomic>
#include <thread>
#include <vector>
#include <memory>

namespace bws::barometer
{

// Type tag for the DC filter (juce::dsp::ProcessorDuplicator<IIR::Filter, IIR::Coefficients>).
// Used only as a template parameter for ChainSpec - never instantiated.
struct IirHighPass
{};

// =============================================================================
// A/B Compare: Parameter State Storage
// =============================================================================
struct ParameterState
{
    float gain = 0.0f;
    bool phaseInvertL = false;    // Per-channel phase (left)
    bool phaseInvertR = false;    // Per-channel phase (right)
    bool phaseLinkEnabled = true; // Link L/R phase controls
    float balance = 0.0f;
    bool dcFilterEnabled = false; // 5 Hz DC high-pass
    bool monoEnabled = false;     // Sum to mono
    float width = 1.0f;           // Stereo width: 0.0 (mono) to 2.0 (wide)
    bool swapLR = false;          // Swap L/R channels
    bool soloL = false;           // Solo left channel
    bool soloR = false;           // Solo right channel
    float outputTrim = 0.0f;      // Output trim (-12 to +12 dB)

    /** Convert to juce::var for legacy user-preset migration. */
    juce::var toVar() const
    {
        juce::DynamicObject::Ptr obj = new juce::DynamicObject();
        obj->setProperty("gain", gain);
        obj->setProperty("phaseL", phaseInvertL);
        obj->setProperty("phaseR", phaseInvertR);
        obj->setProperty("phaseLink", phaseLinkEnabled);
        obj->setProperty("balance", balance);
        obj->setProperty("dc_filter", dcFilterEnabled);
        obj->setProperty("mono", monoEnabled);
        obj->setProperty("width", width);
        obj->setProperty("swapLR", swapLR);
        obj->setProperty("soloL", soloL);
        obj->setProperty("soloR", soloR);
        obj->setProperty("outTrim", outputTrim);
        return juce::var(obj.get());
    }

    /** Create from juce::var during legacy user-preset migration. */
    static ParameterState fromVar(const juce::var& v)
    {
        ParameterState state;
        if (v.isObject())
        {
            auto* obj = v.getDynamicObject();
            if (obj != nullptr)
            {
                state.gain = static_cast<float>(obj->getProperty("gain"));
                state.phaseInvertL = static_cast<bool>(obj->getProperty("phaseL"));
                state.phaseInvertR = static_cast<bool>(obj->getProperty("phaseR"));
                state.phaseLinkEnabled = static_cast<bool>(obj->getProperty("phaseLink"));
                state.balance = static_cast<float>(obj->getProperty("balance"));
                state.dcFilterEnabled =
                    obj->hasProperty("dc_filter") ? static_cast<bool>(obj->getProperty("dc_filter")) : false;
                state.monoEnabled = static_cast<bool>(obj->getProperty("mono"));
                state.width = obj->hasProperty("width") ? static_cast<float>(obj->getProperty("width")) : 1.0f;
                state.swapLR = obj->hasProperty("swapLR") ? static_cast<bool>(obj->getProperty("swapLR")) : false;
                state.soloL = obj->hasProperty("soloL") ? static_cast<bool>(obj->getProperty("soloL")) : false;
                state.soloR = obj->hasProperty("soloR") ? static_cast<bool>(obj->getProperty("soloR")) : false;
                state.outputTrim = obj->hasProperty("outTrim") ? static_cast<float>(obj->getProperty("outTrim")) : 0.0f;
            }
        }
        return state;
    }
};

// =============================================================================
// Factory Preset Definition (kept for backward compatibility)
// =============================================================================
struct Preset
{
    juce::String name;
    float gain;
    bool phaseInvertL;     // Per-channel phase (left)
    bool phaseInvertR;     // Per-channel phase (right)
    bool phaseLinkEnabled; // Link L/R phase controls
    float balance;
    bool monoEnabled = false; // Sum to mono
    float width = 1.0f;       // Stereo width
    bool swapLR = false;      // Swap L/R channels
    bool soloL = false;       // Solo left channel
    bool soloR = false;       // Solo right channel
    float outputTrim = 0.0f;  // Output trim
};

// =============================================================================
// StereoLevelMeter - now using bws::dsp::StereoLevelMeter from module
// =============================================================================

// Forward declarations
class BarometerProcessor;
struct BarometerOptionalHooks; // opaque optional-hook handle - see BarometerOptionalHooks.h

// =============================================================================
// Main Processor
// =============================================================================
class BarometerProcessor : public bws::BwsAudioProcessor
{
public:
    BarometerProcessor();
    ~BarometerProcessor() override;

    // =========================================================================
    // Processor Identity
    // =========================================================================

    const juce::String getName() const override { return "Barometer"; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    // =========================================================================
    // Audio Processing
    // =========================================================================

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void prepareToPlayImpl(double sampleRate, int samplesPerBlock) override;
    void releaseResourcesImpl() override;
    void processBlockImpl(const bws::domain::ProcessContext& ctx) noexcept BWS_NONBLOCKING override;

    // =========================================================================
    // Editor
    // =========================================================================

    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    // =========================================================================
    // Parameters
    // =========================================================================

    juce::AudioProcessorValueTreeState& getApvts() { return apvts_; }
    juce::UndoManager& getUndoManager() { return undoManager_; }

    // Parameter IDs
    static constexpr const char* kGainParamId = "gain";
    static constexpr const char* kPhaseInvertLParamId = "phaseL"; // Per-channel phase (left)
    static constexpr const char* kPhaseInvertRParamId = "phaseR"; // Per-channel phase (right)
    static constexpr const char* kPhaseLinkParamId = "phaseLink"; // Link L/R phase controls
    static constexpr const char* kBalanceParamId = "balance";
    static constexpr const char* kDcFilterParamId = "dc_filter"; // DC offset removal (5Hz HPF)
    static constexpr const char* kMonoParamId = "mono";          // Sum to mono
    static constexpr const char* kWidthParamId = "width";        // Stereo width
    static constexpr const char* kSwapLRParamId = "swapLR";      // Swap L/R channels
    static constexpr const char* kSoloLParamId = "soloL";        // Solo left channel
    static constexpr const char* kSoloRParamId = "soloR";        // Solo right channel
    static constexpr const char* kOutputTrimParamId = "outTrim"; // Output trim (-12 to +12 dB)

    // Legacy parameter ID (for backward compatibility migration)
    static constexpr const char* kLegacyPhaseParamId = "phase";

    // Gain range constants
    static constexpr float kGainMinDb = -96.0f;
    static constexpr float kGainMaxDb = 24.0f;
    static constexpr float kGainDefaultDb = 0.0f;

    // =========================================================================
    // Self-Describing Topology
    // =========================================================================
    // Compile-time verified signal chain declaration.
    // If processBlockImpl() changes, update this and the static_assert.

    static constexpr auto kSignalChain =
        bws::dsp::Serial {bws::dsp::Node<bws::dsp::StereoLevelMeter> {"input-meter"},
                          bws::dsp::Conditional<IirHighPass> {"dc-filter", "dc_filter"},
                          bws::dsp::Node<juce::dsp::Gain<float>> {"gain"},
                          bws::dsp::Node<bws::dsp::PhaseInverter> {"phase-invert"},
                          bws::dsp::Node<bws::dsp::StereoBalancer> {"stereo-balance"},
                          bws::dsp::Node<bws::dsp::StereoWidth> {"stereo-width"},
                          bws::dsp::Node<bws::dsp::MonoSum> {"mono-sum"},
                          bws::dsp::Node<bws::dsp::ChannelSwap> {"channel-swap"},
                          bws::dsp::Node<bws::dsp::ChannelSolo> {"channel-solo"},
                          bws::dsp::Inline {"output-trim", "Output Trim",
                                            "Final gain stage (\xC2\xB1"
                                            "12 dB)"},
                          bws::dsp::Inline {"sanitizer", "Safety Sanitizer", "NaN/Inf guard"},
                          bws::dsp::Node<bws::dsp::StereoLevelMeter> {"output-meter"},
                          bws::dsp::Node<bws::audio::LoudnessMeter> {"lufs-meter"},
                          bws::dsp::Node<bws::audio::TruePeakMeter> {"true-peak-meter"}};
    static_assert(
        kSignalChain.nodeCount() == 14,
        "kSignalChain node count doesn't match processBlockImpl - update declaration (correlation is inline)");

    static constexpr auto kThreadCrossings = bws::dsp::ThreadMap {
        bws::dsp::AtomicCrossing {"apvts-params", bws::dsp::Thread::Message >> bws::dsp::Thread::Audio,
                                  bws::dsp::Ordering::Relaxed, "Parameter sync via JUCE APVTS", "std::atomic<float>"},
        bws::dsp::AtomicCrossing {"meter-values", bws::dsp::Thread::Audio >> bws::dsp::Thread::UI,
                                  bws::dsp::Ordering::Relaxed, "Peak/RMS via StereoLevelMeter", "std::atomic<float>"},
        bws::dsp::AtomicCrossing {"loudness-values", bws::dsp::Thread::Audio >> bws::dsp::Thread::UI,
                                  bws::dsp::Ordering::Relaxed, "LUFS/TruePeak via LoudnessMeter + TruePeakMeter",
                                  "std::atomic<float>"},
        bws::dsp::AtomicCrossing {"rms-values", bws::dsp::Thread::Audio >> bws::dsp::Thread::UI,
                                  bws::dsp::Ordering::Relaxed, "RMS levels (300ms integration, inline computation)",
                                  "std::atomic<float>"},
        bws::dsp::AtomicCrossing {"correlation-value", bws::dsp::Thread::Audio >> bws::dsp::Thread::UI,
                                  bws::dsp::Ordering::Relaxed, "Stereo phase correlation (inline computation)",
                                  "std::atomic<float>"}};
    static_assert(kThreadCrossings.size() == 5, "kThreadCrossings count doesn't match actual atomic declarations");

    // =========================================================================
    // A/B Compare
    // =========================================================================
    void toggleAB();
    bool isStateA() const { return currentIsA_; }
    ParameterState getCurrentState() const;

    /** Copy current A state to B (B becomes a copy of A) */
    void copyAToB();

    /** Copy current B state to A (A becomes a copy of B) */
    void copyBToA();

    // =========================================================================
    // Output Meter (Stereo)
    // =========================================================================
    float getOutputPeakDb() const { return outputMeter_.getPeakLevelDb(); }
    float getLeftPeakDb() const { return outputMeter_.getLeftPeakDb(); }
    float getRightPeakDb() const { return outputMeter_.getRightPeakDb(); }

    // =========================================================================
    // Input Meter (Stereo) - measures signal BEFORE processing
    // =========================================================================
    float getInputPeakDb() const { return inputMeter_.getPeakLevelDb(); }
    float getInputLeftPeakDb() const { return inputMeter_.getLeftPeakDb(); }
    float getInputRightPeakDb() const { return inputMeter_.getRightPeakDb(); }

    // =========================================================================
    // RMS Metering (inline computation, 300ms integration)
    // =========================================================================
    float getOutputLeftRmsDb() const { return outputRmsLeftDb_.load(std::memory_order_relaxed); }
    float getOutputRightRmsDb() const { return outputRmsRightDb_.load(std::memory_order_relaxed); }
    float getInputLeftRmsDb() const { return inputRmsLeftDb_.load(std::memory_order_relaxed); }
    float getInputRightRmsDb() const { return inputRmsRightDb_.load(std::memory_order_relaxed); }

    // =========================================================================
    // Stereo Correlation (post-processing)
    // =========================================================================
    float getStereoCorrelation() const { return stereoCorrelation_.load(std::memory_order_relaxed); }

    // =========================================================================
    // LUFS Metering (post-processing, ITU-R BS.1770-5)
    // =========================================================================
    float getMomentaryLufs() const { return lufsMeter_.getMomentaryLufs(); }
    float getShortTermLufs() const { return lufsMeter_.getShortTermLufs(); }
    float getIntegratedLufs() const { return lufsMeter_.getIntegratedLufs(); }
    float getLoudnessRange() const { return lufsMeter_.getLoudnessRange(); }
    // stability flag for the LRA hero readout.
    // PluginEditor consults this and renders "-" when false (first 60 s of
    // measurement; matches EBU Tech 3342 / FLUX MiRA / Pleasurize convention).
    bool isLoudnessRangeStable() const { return lufsMeter_.isLoudnessRangeStable(); }
    void resetIntegratedLufs() { lufsMeter_.resetIntegrated(); }

    // =========================================================================
    // True Peak (4x oversampled, post-processing)
    // =========================================================================
    float getTruePeakDb() const { return truePeakMeter_.getTruePeakDb(); }
    float getHeldTruePeakDb() const { return truePeakMeter_.getHeldPeakDb(); }
    float getTruePeakDbLeft() const { return truePeakMeter_.getTruePeakDb(0); }
    float getTruePeakDbRight() const { return truePeakMeter_.getTruePeakDb(1); }
    void resetTruePeakHold() { truePeakMeter_.resetHeldPeaks(); }

    // =========================================================================
    // Presets - delegated to WeatherPresetManager
    // =========================================================================

    /** Get the preset manager for direct access */
    BarometerWeatherPresetManager& getPresetManager() { return *presetManager_; }
    const BarometerWeatherPresetManager& getPresetManager() const { return *presetManager_; }

    // Factory presets
    int getFactoryPresetCount() const { return presetManager_->getFactoryPresetCount(); }
    int getCurrentPresetIndex() const { return presetManager_->getCurrentPresetIndex(); }
    int getCurrentPresetComboId(int userPresetIdOffset = 100) const
    {
        const int currentIndex = presetManager_->getCurrentPresetIndex();
        const auto& presets = presetManager_->getAllPresets();
        if (!juce::isPositiveAndBelow(currentIndex, static_cast<int>(presets.size())))
            return 0;

        int factoryIndex = 0;
        int userIndex = 0;
        for (int i = 0; i <= currentIndex; ++i)
        {
            const auto& preset = presets[static_cast<size_t>(i)];
            if (preset.isFactory)
            {
                if (i == currentIndex)
                    return factoryIndex + 1;
                ++factoryIndex;
            }
            else
            {
                if (i == currentIndex)
                    return userPresetIdOffset + userIndex;
                ++userIndex;
            }
        }

        return 0;
    }
    void loadPreset(int index) { presetManager_->loadPresetByIndex(index); }
    juce::StringArray getPresetNames() const { return presetManager_->getFactoryPresetNames(); }

    // User presets - wrappers delegate to the shared facade for tests and host
    // integration; editor paths read preset data directly via
    // getPresetManager().getAllPresets() (see BarometerEditor::rebuildPresetCombo).
    // No local cache - facade is the single source of truth.
    int getNumUserPresets() const { return presetManager_->getUserPresetCount(); }
    bool saveUserPreset(const juce::String& name) { return presetManager_->saveUserPreset(name); }
    bool loadUserPreset(int index) { return presetManager_->loadUserPreset(index); }
    int findUserPresetByName(const juce::String& name) const { return presetManager_->findUserPresetByName(name); }
    bool updateUserPreset(int index) { return presetManager_->updateUserPreset(index); }
    bool deleteUserPreset(int index) { return presetManager_->deleteUserPreset(index); }
    bool renameUserPreset(int index, const juce::String& newName)
    {
        return presetManager_->renameUserPreset(index, newName);
    }
    void loadUserPresets() { presetManager_->loadUserPresets(); }

    // Preset directory
    juce::File getPresetDirectory() const { return presetManager_->getPresetDirectory(); }
    juce::File getDefaultPresetDirectory() const { return presetManager_->getDefaultPresetDirectory(); }
    bool hasCustomPresetDirectory() const { return presetManager_->hasCustomPresetDirectory(); }
    juce::File getCustomPresetDirectory() const { return presetManager_->getCustomPresetDirectory(); }
    void setCustomPresetDirectory(const juce::File& directory) { presetManager_->setCustomPresetDirectory(directory); }
    void clearCustomPresetDirectory() { presetManager_->clearCustomPresetDirectory(); }

protected:
    // =========================================================================
    // BwsAudioProcessor state management hooks
    // =========================================================================
    juce::AudioProcessorValueTreeState* getApvtsPtr() override { return &apvts_; }
    bws::IPresetStatePersistence* getPresetManagerPtr() override { return presetManager_.get(); }
    void migrateValueTreeState(juce::ValueTree& state, const juce::XmlElement& xml) override;

private:
    // Undo/Redo support
    juce::UndoManager undoManager_;

    // Parameter management
    juce::AudioProcessorValueTreeState apvts_;

    // Cached APVTS parameter pointers - populated once in prepareToPlayImpl.
    struct BarometerParamPtrs
    {
        std::atomic<float>* pGain {};
        std::atomic<float>* pPhaseL {};
        std::atomic<float>* pPhaseR {};
        std::atomic<float>* pPhaseLink {};
        std::atomic<float>* pBalance {};
        std::atomic<float>* pDcFilter {};
        std::atomic<float>* pMono {};
        std::atomic<float>* pWidth {};
        std::atomic<float>* pSwapLR {};
        std::atomic<float>* pSoloL {};
        std::atomic<float>* pSoloR {};
        std::atomic<float>* pOutTrim {};

        void bind(juce::AudioProcessorValueTreeState& apvts) noexcept;

        [[nodiscard]] static float load(std::atomic<float>* ptr, float fallback = 0.0f) noexcept
        {
            if (ptr)
                return ptr->load(std::memory_order_relaxed);
            jassertfalse;
            return fallback;
        }
    };
    BarometerParamPtrs paramPtrs_;

public:
    /// Builds a plain-value ParamSnapshot from cached atomic pointers. RT-safe.
    [[nodiscard]] bws::domain::ParamSnapshot buildParamSnapshot() const noexcept BWS_NONBLOCKING override;

    // Optional commercial hooks behind an opaque handle. ALWAYS one pointer (empty in
    // the open-source build) so the object layout does not depend on build
    // configuration. See BarometerOptionalHooks.h.
    std::unique_ptr<BarometerOptionalHooks> optionalHooks_;

public:
    /// True iff the optional commercial hooks are active. Use this - NOT a
    /// `optionalHooks_ != nullptr` check - since the handle itself is non-null even in
    /// the open-source build.
    [[nodiscard]] bool hasOptionalHooks() const noexcept;


private:
    // DSP Processors
    juce::dsp::Gain<float> gainProcessor_;
    bws::dsp::PhaseInverter phaseInverter_;
    bws::dsp::StereoBalancer stereoBalancer_;
    bws::dsp::StereoWidth stereoWidth_;
    bws::dsp::MonoSum monoSum_;
    bws::dsp::ChannelSwap channelSwap_;
    bws::dsp::ChannelSolo channelSolo_;

    // DC Filter (5Hz high-pass filter to remove DC offset)
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> dcFilter_;

    // Create parameter layout
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // =========================================================================
    // A/B Compare State
    // =========================================================================
    ParameterState stateA_;
    ParameterState stateB_;
    bool currentIsA_ = true;

    void saveCurrentToState(ParameterState& state);
    void loadStateToParameters(const ParameterState& state);

    // =========================================================================
    // Output Meter (Stereo) - using module
    // =========================================================================
    bws::dsp::StereoLevelMeter outputMeter_;

    // =========================================================================
    // Input Meter (Stereo) - measures signal BEFORE processing
    // =========================================================================
    bws::dsp::StereoLevelMeter inputMeter_;

    // =========================================================================
    // LUFS & True Peak Meters (post-processing)
    // =========================================================================
    bws::audio::LoudnessMeter lufsMeter_ {bws::audio::LoudnessMeter::AnalyticsMode::ExternalService};
    bws::audio::TruePeakMeter truePeakMeter_;
    std::jthread loudnessAnalyticsWorker_;

    void startLoudnessAnalyticsWorker();
    void stopLoudnessAnalyticsWorker() noexcept;

    // =========================================================================
    // RMS Tracking (inline, 300ms integration)
    // =========================================================================
    std::atomic<float> outputRmsLeftDb_ {-100.0f};
    std::atomic<float> outputRmsRightDb_ {-100.0f};
    std::atomic<float> inputRmsLeftDb_ {-100.0f};
    std::atomic<float> inputRmsRightDb_ {-100.0f};
    float smoothedOutputRmsL_ = 0.0f;
    float smoothedOutputRmsR_ = 0.0f;
    float smoothedInputRmsL_ = 0.0f;
    float smoothedInputRmsR_ = 0.0f;
    float rmsCoeff_ = 0.0f;

    // =========================================================================
    // Stereo Correlation (inline, post-processing)
    // =========================================================================
    std::atomic<float> stereoCorrelation_ {1.0f};

    // =========================================================================
    // Preset Manager (WeatherPresetManager - shared preset system)
    // =========================================================================
    std::unique_ptr<BarometerWeatherPresetManager> presetManager_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BarometerProcessor)
};

} // namespace bws::barometer

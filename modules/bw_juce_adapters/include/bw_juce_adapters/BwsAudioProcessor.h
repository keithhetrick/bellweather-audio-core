// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

//==============================================================================
// h
//==============================================================================
// PURPOSE: Foundation base class for ALL Bellweather Studios audio plugins
//
// WHY THIS EXISTS:
// Instead of inheriting from juce::Processor directly, ALL Bellweather
// plugins inherit from to get automatic RT-safety guarantees
// and performance optimizations for free.
//
// WHAT IT PROVIDES:
// 1. RT-SAFETY ENFORCEMENT (AudioThreadScope guard)
//    - Prevents accidental heap allocations in processBlock
//    - Catches RT violations during development/testing
//    - See: modules/bw_rt/include/bw_rt/AudioThreadScope.h
//
// 2. DENORMAL HANDLING (Automatic FTZ/DAZ)
//    - Prevents CPU spikes from very small floating point numbers
//    - Automatically enabled in processBlock (ScopedNoDenormals)
//    - 0.5-2% CPU reduction depending on DSP algorithm
//
// 3. PERFORMANCE OPTIMIZATIONS
//    - Cached channel counts (avoids virtual calls per-sample)
//    - Cached sample rate/block size (reduces function call overhead)
// - Measured 0.7% CPU reduction from optimizations
//
// 4. CONSISTENT ARCHITECTURE
//    - All plugins follow same lifecycle (prepareToPlay → processBlock → release)
//    - Framework handles common setup/teardown automatically
//    - Plugin code focuses on DSP, not boilerplate
//
// HOW TO USE:
// 1. Inherit from bws:: (NOT juce::Processor!)
// 2. Override prepareToPlayImpl(), releaseResourcesImpl(), processBlockImpl()
// 3. Framework calls your Impl methods with RT-safety already applied
//
// EXAMPLE:
// class Plugin: public bws:: {
//     void prepareToPlayImpl(double sr, int blockSize) override { ... }
//     void processBlockImpl(const bws::domain::ProcessContext& ctx) override { ... }
//   };
//
// RT-SAFETY GUARANTEE:
// When processBlockImpl() is called, you are GUARANTEED:
// - AudioThreadScope is active (heap allocations will be caught)
// - Denormals are disabled (FTZ/DAZ flags set)
// - cachedInCh, cachedOutCh, cachedSampleRate are up-to-date
//
//==============================================================================

#pragma once

#include <bw_juce_adapters/juce_overrides.h> // FEA-suppressed JUCE surface (audio-thread); juce::Processor
#include "bw_juce_adapters/PluginBusLayoutPolicy.h"
#include "bw_rt/AudioThreadScope.h" // RT-safety guard
#include <atomic>                   // std::atomic for thread-safe caching

// Interfaces from bw_ports (replaces bw_ui forward declarations).
// The substrate is the single source of truth for A/B state.
#include <bw_ports/IPresetStatePersistence.h>
#include <bw_audio_types/ProcessContext.h> // processBlockImpl signature
#include <bw_audio_types/ParamSnapshot.h>  // buildParamSnapshot pure virtual

namespace bws
{

//==============================================================================
// BASE CLASS FOR ALL BELLWEATHER AUDIO PROCESSORS
//==============================================================================
/**
 * - Foundation for all Bellweather Studios plugins
 *
 * INHERITANCE CHAIN:
 * - Plugin → → juce::Processor → (JUCE internals)
 *
 * WHY INHERIT FROM THIS INSTEAD OF juce::Processor?
 * - Automatic RT-safety enforcement (catches allocations in audio thread)
 * - Automatic denormal handling (prevents CPU spikes)
 * - Automatic performance optimizations (cached values, reduced overhead)
 * - Consistent architecture across all Bellweather plugins
 *
 * WHAT'S DIFFERENT FROM juce::Processor?
 * - prepareToPlay/processBlock/releaseResources are FINAL (you can't override)
 * - Instead, override prepareToPlayImpl/processBlockImpl/releaseResourcesImpl
 * - Framework handles RT-safety setup, then calls your Impl methods
 *
 * PERFORMANCE BENEFITS:
 * - 0.7% CPU reduction measured in optimizations
 * - Denormal handling prevents occasional CPU spikes (0.5-2% reduction)
 * - Cached atomics avoid virtual function calls in hot loops
 *
 * RT-SAFETY RULES (ENFORCED IN PROCESSBLOCK):
 * ❌ NO heap allocations (new, malloc, std::vector::push_back, std::string, etc.)
 * ❌ NO locks/mutexes (std::lock_guard, std::mutex::lock, etc.)
 * ❌ NO blocking I/O (file read/write, network, sleep, etc.)
 * ❌ NO unbounded loops (while(true), recursion, std::find on large containers)
 * ❌ NO exceptions (can trigger allocations in unwinding)
 * ❌ NO logging (juce::Logger, std::cout, printf - use RtErrorQueue instead)
 * ✅ OK: Atomics, lock-free queues, pre-allocated buffers, math, SIMD
 *
 */
class BwsAudioProcessor : public juce::AudioProcessor
{
public:
    //==========================================================================
    // CONSTRUCTORS
    //==========================================================================
    static BusesProperties makeBusesPropertiesForPolicy(adapters::PluginChannelLayoutPolicy policy);

    /**
     * Default constructor for standard stereo plugin.
     * Creates default I/O bus layout (stereo in, stereo out).
     */
    BwsAudioProcessor();

    /**
     * Constructor with custom I/O bus layout.
     *
     * USE CASES:
     * - Mono plugins: BusesProperties().withInput("Input", AudioChannelSet::mono())
     * - Surround: Custom channel layouts (5.1, 7.1, Atmos, etc.)
     * - Sidechain: Multiple input buses
     * - Generators/Synths: No input bus (output only)
     *
     * EXAMPLE (Mono in, stereo out):
     * (BusesProperties
     *       .withInput("Input", AudioChannelSet::mono())
     *       .withOutput("Output", AudioChannelSet::stereo()))
     */
    explicit BwsAudioProcessor(const BusesProperties& ioLayouts);

    /**
     * Destructor (nothing special to clean up).
     */
    ~BwsAudioProcessor() override = default;

    //==========================================================================
    // BUS LAYOUT VALIDATION (/ )
    //==========================================================================
    // Default: stereo in/out only. Closes gap - no plugin falls through
    // to JUCE's permissive default. Plugins that support mono should override
    // and call isMainBusMonoOrStereo(). Plugins with sidechains should also
    // call isSidechainValid().
    //==========================================================================

    bool isBusesLayoutSupported(const juce::AudioProcessor::BusesLayout& layouts) const override
    {
        return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo() &&
               layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
    }

    //==========================================================================
    // FRAMEWORK-MANAGED METHODS (FINAL - Plugin cannot override!)
    //==========================================================================
    // These methods are marked 'final' because the framework needs to control
    // them to apply RT-safety guards and performance optimizations.
    //
    // YOUR PLUGIN MUST OVERRIDE THE *Impl VERSIONS BELOW, NOT THESE!
    //
    // LIFECYCLE:
    // 1. DAW calls prepareToPlay() → Framework sets up → calls prepareToPlayImpl()
    // 2. DAW calls processBlock() → Framework applies RT guards → calls processBlockImpl()
    // 3. DAW calls releaseResources() → Framework calls releaseResourcesImpl()
    //==========================================================================

    /**
     * Framework-managed prepareToPlay (FINAL - do not override).
     *
     * WHAT THIS DOES:
     * 1. Updates cached atomics (sample rate, channel counts, block size)
     * 2. Calls your prepareToPlayImpl() with same parameters
     *
     * CALLED ON: AUDIO THREAD (but not real-time yet - allocations OK)
     * CALLED WHEN:
     * - Plugin loaded into DAW
     * - Sample rate changed (user changes audio interface settings)
     * - Buffer size changed (user changes DAW buffer size)
     * - Audio device restarted
     *
     * PARAMETERS:
     * - sampleRate: Current sample rate (44100, 48000, 96000, etc.)
     * - samplesPerBlock: Maximum samples per processBlock call (typically 64-512)
     *
     * ⚠️ OVERRIDE prepareToPlayImpl() INSTEAD OF THIS!
     */
    void prepareToPlay(double sampleRate, int samplesPerBlock) final;

    /**
     * Framework-managed releaseResources (FINAL - do not override).
     *
     * WHAT THIS DOES:
     * 1. Calls your releaseResourcesImpl()
     * 2. Resets cached atomics
     *
     * CALLED ON: AUDIO THREAD (but not real-time - allocations OK)
     * CALLED WHEN:
     * - Plugin unloaded from DAW
     * - Audio device stopped
     * - Before prepareToPlay() if already prepared
     *
     * USE FOR:
     * - Free audio buffers (delay lines, convolution IRs, etc.)
     * - Release external resources
     *
     * ⚠️ OVERRIDE releaseResourcesImpl() INSTEAD OF THIS!
     */
    void releaseResources() final;

    /**
     * Framework-managed processBlock (FINAL - do not override).
     *
     * WHAT THIS DOES (automatically, before calling your code):
     * 1. Activates AudioThreadScope (RT-safety guard - catches allocations)
     * 2. Enables ScopedNoDenormals (prevents CPU spikes from tiny floats)
     * 3. Calls your processBlockImpl() with RT-safety active
     * 4. Deactivates guards when done
     *
     * CALLED ON: AUDIO THREAD (REAL-TIME CRITICAL!)
     * CALLED WHEN: Every audio buffer (thousands of times per second)
     *
     * FREQUENCY:
     * - 44.1kHz, 512 samples → ~86 calls/second
     * - 48kHz, 128 samples → ~375 calls/second
     * - 96kHz, 64 samples → ~1500 calls/second
     *
     * PERFORMANCE:
     * - Must complete in < buffer duration or you get dropouts
     * - Example: 128 samples @ 48kHz = 2.7ms maximum execution time
     *
     * RT-SAFETY ENFORCEMENT:
     * When processBlockImpl() is called:
     * - AudioThreadScope is active → heap allocations will ASSERT/LOG
     * - Denormals disabled → FTZ (flush-to-zero) and DAZ (denormals-are-zero)
     * - Cached values valid → use cachedSampleRate instead of getSampleRate()
     *
     * ⚠️ OVERRIDE processBlockImpl() INSTEAD OF THIS!
     * ⚠️ FOLLOW RT-SAFETY RULES (see class comment above)!
     */
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) final;

    //==========================================================================
    // CENTRALIZED STATE MANAGEMENT
    //==========================================================================
    // These methods handle APVTS + presets + A/B comparison state automatically.
    // Plugins should NOT override these. Instead, implement the protected hooks
    // below (getApvts, getPresetManager, saveCustomState, restoreCustomState).
    //
    // DESIGN PHILOSOPHY: "Write once, benefit everywhere"
    // - One bug fix in base class fixes ALL plugins
    // - One feature addition benefits entire suite
    // - Plugins get correct state persistence with zero boilerplate
    //==========================================================================

    /**
     * Save plugin state (APVTS + presets + A/B comparison).
     *
     * This implementation automatically handles:
     * 1. APVTS state (via getApvts())
     * 2. Preset metadata (via getPresetManager(), if present)
     * 3. A/B comparison state (if plugin uses ABComparisonMixin)
     * 4. Plugin-specific state (via saveCustomState(), if overridden)
     *
     * ⚠️ DO NOT OVERRIDE THIS METHOD
     * Instead, implement getApvts() and optionally getPresetManager()/saveCustomState()
     */
    void getStateInformation(juce::MemoryBlock& destData) override;

    /**
     * Restore plugin state (APVTS + presets + A/B comparison).
     *
     * This implementation automatically handles:
     * 1. APVTS state restoration (via getApvts())
     * 2. Preset selection restoration (via getPresetManager(), if present)
     * 3. A/B comparison state restoration (if plugin uses ABComparisonMixin)
     * 4. Plugin-specific state restoration (via restoreCustomState(), if overridden)
     *
     * ⚠️ DO NOT OVERRIDE THIS METHOD
     * Instead, implement getApvts() and optionally getPresetManager()/restoreCustomState()
     */
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==========================================================================
    // PLUGIN IMPLEMENTATION POINTS (Override these in your plugin!)
    //==========================================================================
    // These are the methods YOUR PLUGIN must override to implement DSP.
    // The framework calls these after setting up RT-safety guards.
    //
    // GUARANTEED WHEN CALLED:
    // - prepareToPlayImpl: cachedSampleRate/cachedBlockSize are updated
    // - processBlockImpl: AudioThreadScope active, denormals disabled
    // - releaseResourcesImpl: Audio processing stopped
    //==========================================================================

    /**
     * Initialize your DSP for audio processing.
     *
     * CALLED BY: Framework from prepareToPlay() (see above)
     * CALLED ON: AUDIO THREAD (not real-time - allocations OK here)
     *
     * GUARANTEED CONDITIONS:
     * - sampleRate > 0
     * - samplesPerBlock > 0
     * - Called BEFORE first processBlockImpl()
     * - cachedSampleRate, cachedBlockSize already updated
     *
     * USE FOR:
     * - Allocate audio buffers (delay lines, reverb IRs, etc.)
     * - Reset DSP state (filters, envelopes, oscillators, etc.)
     * - Calculate sample-rate-dependent coefficients
     * - Pre-allocate any RT-unsafe resources needed later
     *
     * EXAMPLE:
     *   void prepareToPlayImpl(double sr, int blockSize) override {
     *       delayLine.resize(sr * 2.0);  // 2 second delay buffer
     *       filter.setSampleRate(sr);
     *       filter.reset();
     *   }
     *
     * ⚠️ DO NOT call the base class version (pure virtual)
     */
    virtual void prepareToPlayImpl(double sampleRate, int samplesPerBlock) = 0;

    /**
     * Clean up audio resources.
     *
     * CALLED BY: Framework from releaseResources() (see above)
     * CALLED ON: AUDIO THREAD (not real-time - allocations OK here)
     *
     * USE FOR:
     * - Free audio buffers
     * - Release external resources (file handles, GPU resources, etc.)
     * - NOT required for automatic cleanup (std::vector, unique_ptr, etc.)
     *
     * EXAMPLE:
     *   void releaseResourcesImpl() override {
     *       delayLine.clear();  // Free memory (optional - destructor does this)
     *   }
     *
     * NOTE: Most plugins can leave this empty (automatic cleanup is fine)
     * ⚠️ DO NOT call the base class version (pure virtual)
     */
    virtual void releaseResourcesImpl() = 0;

    /**
     * Process audio samples (YOUR DSP GOES HERE!).
     *
     * CALLED BY: Framework from processBlock() (see above)
     * CALLED ON: AUDIO THREAD (REAL-TIME CRITICAL - NO ALLOCATIONS!)
     *
     *The base class builds ProcessContext (buffer + transport + params)
     * and calls this method. The compiler rejects the old (AudioBuffer, MidiBuffer)
     * signature - the seam is enforced by the type system.
     *
     * GUARANTEED CONDITIONS:
     * - prepareToPlayImpl() was called first
     * - AudioThreadScope is active (allocations will be caught)
     * - Denormals are disabled (FTZ/DAZ active)
     * - ctx.buffer is valid, non-owning view into the host's audio data
     * - ctx.params was built by buildParamSnapshot() (N atomic loads, RT-safe)
     * - ctx.transport contains host playhead state
     *
     * TYPICAL PATTERN:
     *   void processBlockImpl(const bws::domain::ProcessContext& ctx) override {
     *       auto buffer = bws::adapters::toJuceBufferView(ctx.buffer);
     *       // ... process buffer using ctx.params for parameter values ...
     *   }
     *
     * ⚠️ RT-SAFETY RULES (ABSOLUTE REQUIREMENTS):
     * ❌ NO heap allocations (new, malloc, std::vector::push_back, std::string)
     * ❌ NO locks/mutexes (std::lock_guard, std::mutex)
     * ❌ NO blocking I/O (file read/write, network)
     * ❌ NO unbounded loops (while(true), recursion)
     * ❌ NO exceptions
     * ❌ NO logging (juce::Logger, std::cout, printf)
     * ✅ OK: Atomics, lock-free queues, pre-allocated buffers, math, SIMD
     *
     * ⚠️ DO NOT call the base class version (pure virtual)
     */
    virtual void processBlockImpl(const bws::domain::ProcessContext& ctx) = 0;

    /**
     * Build a plain-value ParamSnapshot from cached APVTS pointers.
     * Called once per processBlock() - N atomic loads, stack allocated, RT-safe.
     * Each plugin implements this using its own ParamId enum and cached pointers.
     * Non-pilot plugins and examples return ParamSnapshot{} until migrated.
     */
    [[nodiscard]] virtual bws::domain::ParamSnapshot buildParamSnapshot() const noexcept BWS_NONBLOCKING = 0;

    //==========================================================================
    // STATE MANAGEMENT HOOKS (Implement these in your plugin!)
    //==========================================================================
    // These hooks allow the centralized state management to access your plugin's
    // APVTS, preset manager, and any custom state. Override the ones you need.
    //==========================================================================

    /**
     * Return pointer to plugin's APVTS (REQUIRED for all plugins).
     *
     * Example implementation:
     * @code
     * juce::AudioProcessorValueTreeState* getApvtsPtr() override { return &apvts; }
     * @endcode
     *
     * @return Pointer to the plugin's AudioProcessorValueTreeState
     */
    virtual juce::AudioProcessorValueTreeState* getApvtsPtr() { return nullptr; }

    /**
     * Return pointer to plugin's preset manager (optional).
     *
     * Return nullptr if the plugin doesn't use presets.
     *
     * Example implementation:
     *   bws::IPresetStatePersistence* getPresetManagerPtr() override { return presetManager_.get(); }
     *
     * @return Pointer to the plugin's IPresetStatePersistence, or nullptr if no presets
     */
    virtual bws::IPresetStatePersistence* getPresetManagerPtr() { return nullptr; }

    /**
     * Save plugin-specific state beyond APVTS/presets/A-B (optional).
     *
     * Only override if your plugin has custom state not covered by APVTS,
     * presets, or A/B comparison.
     *
     * Example use cases:
     * - Custom internal state not exposed as parameters
     * - Cached calculation results that should persist
     * - UI-specific state (zoom level, view mode, etc.)
     *
     * @param xml The XML element to add custom state attributes to
     */
    virtual void saveCustomState(juce::XmlElement& xml) { juce::ignoreUnused(xml); }

    /**
     * Restore plugin-specific state beyond APVTS/presets/A-B (optional).
     *
     * Only override if your plugin has custom state not covered by APVTS,
     * presets, or A/B comparison.
     *
     * @param xml The XML element containing custom state attributes
     */
    virtual void restoreCustomState(const juce::XmlElement& xml) { juce::ignoreUnused(xml); }

    /**
     * Migrate old state format to current format (optional).
     *
     * Called BEFORE apvts.replaceState() to allow plugins to migrate
     * old saved state formats. This is where you handle backwards
     * compatibility for changed parameter names, types, or defaults.
     *
     * Example use cases:
     * - Converting bool parameters to float
     * - Renaming parameters (autoRelease -> auto)
     * - Setting default values for new parameters
     *
     * @param state The ValueTree to modify in-place before APVTS restore
     * @param xml The original XML element (for reading additional attributes)
     */
    virtual void migrateValueTreeState(juce::ValueTree& state, const juce::XmlElement& xml)
    {
        juce::ignoreUnused(state, xml);
    }

    //==========================================================================
    // TRIAL MODE HOOKS
    //==========================================================================
    // Override isTrialMode() in production plugins to enable trial restrictions.
    // When isTrialMode() returns true, setStateInformation() resets all
    // parameters to defaults instead of restoring saved state.
    // In-session operations (A/B, preset loading) are not affected.
    //
    // Only effective for plugins that use the base class setStateInformation().
    // Plugins that override setStateInformation() entirely
    // need their own trial guard or must migrate to the centralized pattern.
    //==========================================================================

    /** Override to return true when the plugin is running in trial mode.
     *  Default returns false (no trial restrictions). */
    virtual bool isTrialMode() const { return false; }

    /** Reset all APVTS parameters to their createParameterLayout() defaults.
     *  Override to add plugin-specific reset (e.g. preset curve, DSP state).
     *  Always call the base class version first in overrides. */
    virtual void resetToDefaultState();

    //==========================================================================
    // APVTS MIGRATION HELPERS
    //==========================================================================
    // Use these in migrateValueTreeState() overrides.  JUCE APVTS stores
    // parameters as <PARAM id="..." value="..."/> children, NOT as root-level
    // ValueTree properties.  Always use these helpers instead of
    // state.hasProperty() / state.setProperty() for parameter migrations.
    //

    /** Find a `<PARAM>` child by its id attribute. Returns invalid ValueTree if not found. */
    static juce::ValueTree findParam(const juce::ValueTree& state, const juce::Identifier& paramId)
    {
        return state.getChildWithProperty("id", paramId.toString());
    }

    /** Add a `<PARAM>` child with default value if no child with this id exists. */
    static void setParamDefaultIfMissing(juce::ValueTree& state, const juce::Identifier& paramId,
                                         const juce::var& defaultValue)
    {
        if (!findParam(state, paramId).isValid())
        {
            juce::ValueTree child("PARAM");
            child.setProperty("id", paramId.toString(), nullptr);
            child.setProperty("value", defaultValue, nullptr);
            state.appendChild(child, nullptr);
        }
    }

    /** Convert a bool-typed `<PARAM>` value to float (for bool→float parameter migrations). */
    static void migrateParamBoolToFloat(juce::ValueTree& state, const juce::Identifier& paramId, float onValue)
    {
        auto child = findParam(state, paramId);
        if (child.isValid())
        {
            auto v = child.getProperty("value");
            if (v.isBool())
            {
                child.setProperty("value", static_cast<bool>(v) ? onValue : 0.0f, nullptr);
            }
        }
    }

    /** Rename a `<PARAM>` child from oldId to newId (for parameter rename migrations). */
    static void renameParam(juce::ValueTree& state, const juce::Identifier& oldId, const juce::Identifier& newId)
    {
        if (!findParam(state, newId).isValid())
        {
            auto old = findParam(state, oldId);
            if (old.isValid())
            {
                juce::ValueTree child("PARAM");
                child.setProperty("id", newId.toString(), nullptr);
                child.setProperty("value", old.getProperty("value"), nullptr);
                state.appendChild(child, nullptr);
            }
        }
    }

    /** Clamp an existing `<PARAM>` value to [minVal, maxVal]. */
    static void clampParamValue(juce::ValueTree& state, const juce::Identifier& paramId, float minVal, float maxVal)
    {
        auto child = findParam(state, paramId);
        if (child.isValid())
        {
            float val = static_cast<float>(child.getProperty("value"));
            if (val < minVal || val > maxVal)
            {
                child.setProperty("value", std::clamp(val, minVal, maxVal), nullptr);
            }
        }
    }

protected:
    //==========================================================================
    // BUS LAYOUT HELPERS - isolate JUCE AudioChannelSet logic to adapter layer
    //==========================================================================

    /** Validate main bus is mono or stereo with matching in/out.
     *  Use in plugin overrides that opt into mono support. */
    static bool isMainBusMonoOrStereo(const juce::AudioProcessor::BusesLayout& layouts)
    {
        const auto& mainIn = layouts.getMainInputChannelSet();
        return (mainIn == juce::AudioChannelSet::mono() || mainIn == juce::AudioChannelSet::stereo()) &&
               layouts.getMainOutputChannelSet() == mainIn;
    }

    /** Validate a secondary bus (e.g., sidechain) is disabled, mono, or stereo. */
    static bool isSidechainValid(const juce::AudioProcessor::BusesLayout& layouts, int busIndex = 1)
    {
        const auto& sc = layouts.getChannelSet(true, busIndex);
        return sc.isDisabled() || sc == juce::AudioChannelSet::mono() || sc == juce::AudioChannelSet::stereo();
    }

    //==========================================================================
    // CACHED VALUES FOR RT-SAFE ACCESS
    //==========================================================================
    // Cached in prepareToPlay(), read lock-free from processBlock() to avoid
    // virtual-call overhead. Written on the main thread, read on the audio
    // thread; relaxed atomics guarantee no torn reads.
    //==========================================================================

    /** Cached input channel count (updated in prepareToPlay) */
    std::atomic<int> cachedInCh {0};

    /** Cached output channel count (updated in prepareToPlay) */
    std::atomic<int> cachedOutCh {0};

    /** Cached sample rate (updated in prepareToPlay) */
    std::atomic<float> cachedSampleRate {0.0f};

    /** Cached maximum block size (updated in prepareToPlay) */
    std::atomic<int> cachedBlockSize {0};

    /** Main bus input channel count - excludes sidechain (updated in prepareToPlay).
     *  Returns 1 for mono, 2 for stereo. RT-safe relaxed atomic read. */
    int mainBusChannels() const noexcept { return cachedMainBusCh_.load(std::memory_order_relaxed); }

private:
    std::atomic<int> cachedMainBusCh_ {0};

public:
    // Editor size persistence: stored so the editor can restore its size on reopen.

    /** Store editor size for persistence (call from editor destructor) */
    void setEditorSize(int width, int height)
    {
        editorWidth_ = width;
        editorHeight_ = height;
    }

    /** Get stored editor size (returns {0,0} if not set) */
    std::pair<int, int> getEditorSize() const { return {editorWidth_, editorHeight_}; }

    /** Get the state format version from the last loaded state */
    int getLastLoadedStateVersion() const { return lastLoadedStateVersion_; }

    // A/B toggle is a slot-pointer flip with no getStateInformation
    // round-trip, so no capture-in-progress guard is needed.

    //==========================================================================
    // PRESET LOAD FLAG (trial mode support)
    //==========================================================================
    // Set by WeatherPresetManager (via ScopedPresetLoad RAII guard) around
    // setStateInformation() calls during preset loading.  When true,
    // the trial mode check in setStateInformation() is skipped so that
    // factory presets can be browsed in trial mode.
    //==========================================================================

    /** RAII guard: prevents trial mode from blocking preset browsing.
     * Usage:::ScopedPresetLoad guard(processor);*/
    class ScopedPresetLoad
    {
    public:
        explicit ScopedPresetLoad(BwsAudioProcessor& p)
            : proc_(p)
        {
            proc_.presetLoadInProgress_ = true;
        }
        ~ScopedPresetLoad() { proc_.presetLoadInProgress_ = false; }
        ScopedPresetLoad(const ScopedPresetLoad&) = delete;
        ScopedPresetLoad& operator=(const ScopedPresetLoad&) = delete;
        ScopedPresetLoad(ScopedPresetLoad&&) = delete;
        ScopedPresetLoad& operator=(ScopedPresetLoad&&) = delete;

    private:
        BwsAudioProcessor& proc_;
    };

private:
    /** Editor width storage (for state persistence) */
    int editorWidth_ = 0;

    /** Editor height storage (for state persistence) */
    int editorHeight_ = 0;

    /** State version from last setStateInformation() call (default 1 for pre-versioned states) */
    int lastLoadedStateVersion_ = 1;

    /** True when WeatherPresetManager is loading a preset via setStateInformation().
     *  Prevents trial mode from blocking factory preset browsing. */
    bool presetLoadInProgress_ = false;
    friend class ScopedPresetLoad;

    // Prevent copying (plugins should never be copied)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BwsAudioProcessor)
};

} // namespace bws

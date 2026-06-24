# Module Catalog

This catalog defines every `modules/bw_*` directory in `bellweather-audio-core`.
The public tree is a layered DAG: a module is independently linkable when its
`bws::` target can be linked directly and pulls only the explicit dependencies
listed here, not the whole tree.

Maturity tiers:

- **Stable/reusable** - intended as reusable public library surface.
- **Reference-plugin support** - shipped because Barometer demonstrates the
  library surface in a real JUCE plugin; not positioned as the standalone
  library surface and not a prebuilt plugin release surface.

Not included: commercial product services, certification helpers,
licensing/update services, installers, prebuilt plugin binaries,
signing/notarization artifacts, and unreleased plugin code.

## Modules

### `bw_audio_types`

- Purpose: Framework-neutral audio value types: buffer views, owned buffers, process context, parameter snapshots, transport state, biquad coefficients/filters, and linear smoothing.
- Public headers: `BufferView.h`, `ConstBufferView.h`, `OwnedAudioBuffer.h`, `ProcessContext.h`, `ParamSnapshot.h`, `TransportState.h`, `BwsBiquadFilter.h`, `BwsLinearSmoothedValue.h`, `RtSmoothedParam.h`.
- Depends on: standard library only.
- JUCE: no.
- Independently linkable: yes, `bws::bw_audio_types`.
- Maturity: Stable/reusable.

### `bw_core`

- Purpose: foundational result and compiler-feature utilities shared by higher modules.
- Public headers: `BwCompilerFeatures.h`, `BwResult.h`, `BwToUnderlying.h`.
- Depends on: standard library only.
- JUCE: no.
- Independently linkable: yes, `bws::bw_core`.
- Maturity: Stable/reusable.

### `bw_dsp_core`

- Purpose: Framework-neutral DSP constants, concepts, phase inversion, thread mapping, and one-pole primitives.
- Public headers: `ChainSpec.h`, `DspConstants.h`, `PhaseInverter.h`, `ThreadMap.h`, `TptOnePole.h`, `concepts/DspConcepts.h`.
- Depends on: `bws::bw_audio_types`, `bws::bw_fft_adapters`.
- JUCE: no.
- Independently linkable: yes, `bws::bw_dsp_core`.
- Maturity: Stable/reusable.

### `bw_dsp_metering`

- Purpose: BS.1770/EBU loudness, loudness-range, true-peak, mid/side, envelope, and meter-ballistics primitives plus the public conformance suite.
- Public headers: `Bs1770Meter.h`, `EnvelopeFollower.h`, `LoudnessMeter.h`, `LoudnessRange.h`, `MeterBallistics.h`, `MidSideProcessor.h`, `TruePeakMeter.h`, `detail/LoudnessAnalyticsBridge.h`.
- Depends on: `bws::bw_dsp_core`; tests also use Catch2 and committed Tech 3341 fixtures.
- JUCE: no.
- Independently linkable: yes, `bws::bw_dsp_metering`.
- Maturity: Stable/reusable.

### `bw_dsp_processors`

- Purpose: small reusable channel and stereo processors used by Barometer and available to consumers.
- Public headers: `ChannelSolo.h`, `ChannelSwap.h`, `MonoSum.h`, `StereoBalancer.h`, `StereoLevelMeter.h`, `StereoWidth.h`.
- Depends on: `bws::bw_dsp_core`.
- JUCE: no.
- Independently linkable: yes, `bws::bw_dsp_processors`.
- Maturity: Stable/reusable.

### `bw_fft_adapters`

- Purpose: FFT wrapper with a stable Bellweather surface over platform Accelerate or bundled pffft.
- Public headers: `BwsFFT.h`.
- Depends on: Accelerate on Apple; bundled `vendor/pffft` elsewhere.
- JUCE: no.
- Independently linkable: yes, `bws::bw_fft_adapters`.
- Maturity: Stable/reusable.

### `bw_juce_adapters`

- Purpose: the JUCE host-adapter layer: audio processor base, buffer bridge, scheduler, preset persistence, bus layout policy, system info, and ValueTree retention.
- Public headers: `BwsAudioProcessor.h`, `CaptureSystemInfo.h`, `HostEnvironment.h`, `JuceBufferAdapter.h`, `JucePresetFileRepository.h`, `JucePresetJsonCodec.h`, `JucePresetStateBridge.h`, `MakeJuceScheduler.h`, `PluginBusLayoutPolicy.h`, `RetainValueTree.h`, `dsp/JuceOversamplerAdapter.h`, `juce_overrides.h`.
- Depends on: `bws::bw_core`, `bws::bw_audio_types`, `bws::bw_state_types`, `bws::bw_ports`, `bws::bw_preset_core`, `bws::bw_rt`, and JUCE audio/core/dsp/events modules.
- JUCE: yes.
- Independently linkable: yes when `BWS_BUILD_BAROMETER_PLUGIN=ON`, `bws::bw_juce_adapters`.
- Maturity: Reference-plugin support.

### `bw_ports`

- Purpose: framework-agnostic ports for plugin identity/parameters, scheduling, state serialization, and preset-state persistence.
- Public headers: `IPlugin.h`, `IPluginParameter.h`, `IPresetStatePersistence.h`, `IScheduler.h`, `IStateSerializer.h`.
- Depends on: `bws::bw_core`, `bws::bw_audio_types`, `bws::bw_state_types`.
- JUCE: no.
- Independently linkable: yes, `bws::bw_ports`.
- Maturity: Stable/reusable.

### `bw_preset_core`

- Purpose: Framework-neutral preset catalog, codec, discovery, dirty tracking, identity, navigation, repository, session, storage, state, and migration types.
- Public headers: all headers under `include/bw_preset_core/`, including the umbrella `bw_preset_core.h`.
- Depends on: `bws::bw_state_types`.
- JUCE: no.
- Independently linkable: only when `BWS_BUILD_BAROMETER_PLUGIN=ON`, `bws::bw_preset_core`.
- Maturity: Reference-plugin support. Included to support Barometer's preset path, not as a standalone OSS preset API.

### `bw_rt`

- Purpose: real-time-safety utilities: audio-thread scope, RT guard, violation buffer, lock-free ring buffer, assertions, denormal flushing, and snap-to-zero helpers.
- Public headers: `AudioThreadScope.h`, `CompositionFuzz.h`, `CrossStageInvariant.h`, `LockFreeRingBuffer.h`, `RtAssertions.h`, `RtGuard.h`, `RtSafety.h`, `RtViolationBuffer.h`, `ScopedFlushDenormals.h`, `SnapToZero.h`.
- Depends on: standard library only.
- JUCE: no.
- Independently linkable: yes, `bws::bw_rt`.
- Maturity: Stable/reusable.

### `bw_state_types`

- Purpose: Framework-neutral value types for state blobs, preset metadata, and host/system snapshots.
- Public headers: `BwStateBlob.h`, `PresetMetadata.h`, `SystemInfo.h`.
- Depends on: standard library only.
- JUCE: no.
- Independently linkable: yes, `bws::bw_state_types`.
- Maturity: Stable/reusable.

### `bw_toy_shell`

- Purpose: small JUCE UI primitives used by Barometer's reference interface.
- Public headers: `HiddenClick.h`, `HintCard.h`, `HudUtils.h`, `TimerPump.h`.
- Depends on: `juce::juce_gui_basics`.
- JUCE: yes.
- Independently linkable: yes when `BWS_BUILD_BAROMETER_PLUGIN=ON`, `bws::bw_toy_shell`.
- Maturity: Reference-plugin support.

### `bw_ui`

- Purpose: token-driven JUCE UI closure used by Barometer: controls, panels, preset manager/dialogs, rendering, localization, weather-layout components, and generated design tokens.
- Public headers: selected headers under `include/bw_ui/Components`, `Presets`, `Visualizers`, `adapters`, `ambient`, `foundation`, `generated`, `interaction`, `interactions`, `internal`, `kernel`, `localization`, `preset_system`, `rendering`, `tokens`, `weather`, and `windowing`.
- Depends on: `bws::bw_juce_adapters`, `bws::bw_preset_core`, `bws::bw_toy_shell`, shipped token JSON, shipped font/localization assets, and JUCE GUI modules.
- JUCE: yes.
- Independently linkable: yes when `BWS_BUILD_BAROMETER_PLUGIN=ON`, `bws::bw_ui` and its shipped sub-library targets.
- Maturity: Reference-plugin support.

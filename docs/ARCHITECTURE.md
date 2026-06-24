# Architecture

Bellweather Audio Core has one architectural rule:

**JUCE is the plugin framework adapter, not the DSP substrate.**

The reusable library surface is plain C++ below the adapter line. Barometer is
included to exercise that surface inside a real JUCE plugin, not to turn this
repository into a prebuilt plugin distribution.

## Build surfaces

The root CMake option `BWS_BUILD_BAROMETER_PLUGIN` selects the public build
surface:

| Surface                    | Option | Purpose                                                                                                                  | JUCE dependency |
| -------------------------- | ------ | ------------------------------------------------------------------------------------------------------------------------ | --------------- |
| Library surface            | `OFF`  | Build reusable `bws::` library targets, public-surface smokes, framework-boundary check, and metering conformance tests. | no              |
| Barometer reference plugin | `ON`   | Add the JUCE adapter, preset support, UI closure, and Barometer source plugin.                                           | yes             |

The default is the library surface. That path is the public reusable API.

## Layer map

```text
Barometer reference plugin
  plugins/production/BwsBarometer
    |
    v
Reference-plugin support, built only when BWS_BUILD_BAROMETER_PLUGIN=ON
  bw_ui
  bw_toy_shell
  bw_juce_adapters       JUCE host-adapter layer
  bw_preset_core         framework-neutral preset support for Barometer
    |
    v
Stable reusable library surface, built with BWS_BUILD_BAROMETER_PLUGIN=OFF
  bw_dsp_metering        BS.1770 / EBU loudness, true-peak, LRA
  bw_dsp_processors      channel and stereo utility processors
  bw_dsp_core            DSP constants, concepts, primitive filters
  bw_fft_adapters        stable FFT surface over Accelerate or pffft
  bw_rt                  RT-safety guard, lock-free ring buffer, denormal helpers
  bw_ports               framework-agnostic plugin and state/preset ports
  bw_audio_types         buffer views, owned buffers, process context, smoothing
  bw_state_types         state blob, preset metadata, system info
  bw_core                result type and compiler-feature helpers
```

The exact module list, public headers, dependencies, JUCE use, and maturity tier
are cataloged in [`MODULES.md`](MODULES.md).

## Layer invariants

- Stable/reusable modules must not include JUCE headers or expose `juce::` types.
- `bw_juce_adapters`, `bw_ui`, and `bw_toy_shell` are the only shipped module
  areas expected to link JUCE.
- Barometer-support targets may depend on stable/reusable modules.
- Stable/reusable modules must not depend on Barometer-support targets.
- Consumers should link namespaced `bws::` targets, not raw local target names.
- Audio-thread processing APIs are `noexcept` and must not allocate, lock, log,
  throw, or perform unbounded work.
- Setup/configuration APIs may allocate and may report failure through value
  types such as `bws::domain::BwResult<T>`.

The framework boundary is checked by:

```sh
ctest --test-dir build-library -L framework-boundary --output-on-failure
```

Public target consumption is checked by:

```sh
ctest --test-dir build-library -L public-surface --output-on-failure
```

## Maturity tiers

`Stable/reusable` means the target is intended as public library surface:

- available in the default library build;
- independently linkable by its documented `bws::` alias;
- cataloged in `docs/MODULES.md`;
- covered by public-surface compile/link smoke tests;
- not linked to JUCE unless the catalog explicitly says otherwise.

`Reference-plugin support` means the target is shipped because Barometer needs it
to build from source:

- gated behind `BWS_BUILD_BAROMETER_PLUGIN=ON` when appropriate;
- allowed to link JUCE if it is part of the adapter/UI/plugin layer;
- not presented as the standalone reusable library API;
- not a binary plugin release, installer, signing, or notarization surface.

## Metering thread model

`Bs1770Meter` separates real-time sample processing from slower gated analytics:

```text
Audio thread                         Non-RT thread
------------                         -------------
process(block)
  K-weight samples
  update live 400 ms / 3 s windows
  emit 100 ms loudness records
  push records into SPSC ring  ---->  serviceAnalytics()
                                      drain records
                                      update energy grid
                                      compute integrated LUFS and LRA
                                      publish snapshot with seqlock

getMomentaryLufs()
getShortTermLufs()
  lock-free live reads

getIntegratedLufs()
getLoudnessRange()
  seqlock snapshot reads
```

The audio thread is the single producer for the analytics queue. The servicing
thread is the single consumer. That is why the bridge uses SPSC storage instead
of a mutex-protected queue. It keeps the audio callback bounded while still
allowing integrated loudness and loudness range to be updated off the audio
thread.

## Design choices

### Framework-neutral buffer views

DSP APIs use small view/value types and duck-typed block inputs instead of a
common framework object. That keeps the reusable layer independent of JUCE and
lets tests feed plain buffers without constructing plugin framework state.

Trade-off: call sites must provide the expected shape (`getNumChannels()`,
`getNumSamples()`, `getReadPointer(int)`). The public examples and conformance
tests show that shape directly.

### CMake aliases as the public contract

Consumers link `bws::bw_*` aliases. Raw targets such as `bw_dsp_metering` are
local implementation names. The alias layer keeps the public CMake surface
namespaced and makes downstream compile/link smoke tests meaningful.

### Energy-grid gated loudness

Integrated loudness and LRA use a bounded 0.1 LU energy grid for gated analytics
instead of retaining every block forever. The grid keeps analytics memory bounded
while preserving the gated-mean behavior needed by the conformance tests.

Trade-off: the implementation pays fixed grid work during analytics service
rather than unbounded work proportional to program length.

### Fine sub-step display windows

Momentary and short-term display windows advance on a 5 ms sub-step. The
conformance suite includes a grid-misaligned 400 ms burst case to guard the
claim that display-window timing does not drift outside the published tolerance.

### SPSC plus seqlock snapshots

The audio thread publishes analytics records through a single-producer,
single-consumer queue. The non-RT thread publishes integrated/LRA snapshots with
a seqlock. That combination avoids audio-thread locks while giving readers a
stable snapshot API.

Trade-off: callers that need final integrated/LRA values must call
`serviceAnalytics()` to drain the queue before reading the snapshot.

### RT guard default

`BWS_ENABLE_RT_GUARD` defaults ON because Barometer and the public examples are
meant to expose audio-thread misuse early. Consumers may configure it OFF when a
different runtime checker owns that responsibility.

## What this release does not promise

- no prebuilt plugin binaries;
- no installers;
- no signing or notarization service;
- no consumer plugin support channel;
- no commercial Bellweather product services;
- no unreleased plugin source.

The release is source, tests, build scripts, and documentation for the reusable
audio-core library plus a source-built JUCE reference plugin.

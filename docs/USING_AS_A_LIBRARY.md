# Using Bellweather Audio Core as a library

The most direct path is `add_subdirectory` plus the `bws::` alias targets: point
at the source tree and link the targets you need. The reusable library surface
also supports a conventional CMake install/export flow for consumers that prefer
`find_package(BellweatherAudioCore)`.

```cmake
add_subdirectory(bellweather-audio-core)

target_link_libraries(my_plugin PRIVATE
    bws::bw_dsp_metering   # LUFS / true-peak / loudness-range meters
    bws::bw_dsp_core       # shared DSP primitives the meters build on
)
```

The DSP modules are header-style `INTERFACE` libraries that carry their include
paths and C++ standard as usage requirements, so linking the alias is all that
is needed - no extra `target_include_directories`.

The public CI includes tiny downstream-project smoke targets for every
Stable/reusable module listed in `docs/MODULES.md`. Each smoke target links only
the documented `bws::` alias for that module; if a module needs a missing
`PUBLIC` include path, compile definition, or link dependency, the library
preset fails.

## Target naming

Use the namespaced `bws::` targets in consumer projects:

```cmake
target_link_libraries(my_app PRIVATE bws::bw_dsp_metering)
```

Those aliases are the supported public CMake API. The raw `bw_*` target names
exist inside this source tree so the libraries can be defined and tested; they
are not the documented consumer contract. The namespace keeps Bellweather targets
separate from targets in a larger host project and matches common CMake package
practice (`Catch2::...`, `JUCE::...`, `ZLIB::...`).

## A complete example

Vendor this repo as a subdirectory (or a git submodule) and link the meters:

```text
my-meter-app/
├── CMakeLists.txt
├── main.cpp
└── bellweather-audio-core/        ← this repository
```

`CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.22)
project(my_meter_app LANGUAGES CXX)

# You only want the libraries: skip the Barometer plugin/adapter layer and
# the library's own tests. Both lines must come before add_subdirectory().
set(BWS_BUILD_BAROMETER_PLUGIN OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING              OFF CACHE BOOL "" FORCE)
add_subdirectory(bellweather-audio-core)

add_executable(my_meter_app main.cpp)
target_link_libraries(my_meter_app PRIVATE bws::bw_dsp_metering)
```

`main.cpp` - measure momentary loudness of one stereo block (this exact program
compiles and runs against the tree):

```cpp
#include <bw_dsp_metering/Bs1770Meter.h>
#include <vector>

// process() is duck-typed - any type with these three members works.
struct Block {
    const float* const* data = nullptr;
    int channels = 0, samples = 0;
    int getNumChannels() const noexcept { return channels; }
    int getNumSamples()  const noexcept { return samples; }
    const float* getReadPointer(int ch) const noexcept { return data[ch]; }
};

int main() {
    bws::audio::Bs1770Meter meter;
    meter.prepare(48000.0, /*channels*/ 2);

    std::vector<float> left(480, 0.5f), right(480, 0.5f);
    const float* chans[] = { left.data(), right.data() };
    meter.process(Block{ chans, 2, 480 });

    float momentaryLufs = meter.getMomentaryLufs();   // lock-free, RT-safe
    (void) momentaryLufs;
    return 0;
}
```

Build and run:

```sh
cmake -S . -B build
cmake --build build --target my_meter_app
./build/my_meter_app
```

## Optional install/export flow

The root project enables install/export support when Bellweather Audio Core is
configured as the top-level project. Install the reusable library surface, then
consume it from another CMake project with `find_package`:

```sh
cmake --preset library -DBWS_AUDIO_CORE_INSTALL=ON
cmake --build --preset library
cmake --install build-library --prefix /tmp/bellweather-audio-core-install
```

Consumer `CMakeLists.txt`:

```cmake
find_package(BellweatherAudioCore CONFIG REQUIRED)

target_link_libraries(my_meter_app PRIVATE
    bws::bw_dsp_metering
)
```

Point CMake at the install prefix from the consuming project:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/tmp/bellweather-audio-core-install
```

This install/export path covers the reusable library targets. It does not turn
Barometer into a binary plugin distribution channel.

The meter API - `#include <bw_dsp_metering/Bs1770Meter.h>`, namespace `bws::audio`:

- `prepare(double sampleRate, int numChannels)` - call once before processing.
- `process(view)` - feed one planar block per call. `view` is any type exposing
  `getNumChannels()`, `getNumSamples()`, and `getReadPointer(int)` (as in `Block`
  above); the test helper `PlanarBufferView` is the same shape.
- `getMomentaryLufs()` / `getShortTermLufs()` - lock-free real-time readings.
- `getIntegratedLufs()` - the gated integrated measurement; drain
  `serviceAnalytics()` first to finalize it.

The conformance tests in `modules/bw_dsp_metering/tests/` are the canonical
usage examples - `bs1770_meter_test.cpp` is the shortest starting point, and
`bs1770_conformance_test.cpp` shows the full block-feeding loop.

## Skipping the plugin

If you only want the libraries and the conformance suite, configure with the
plugin off so the adapter layer is not built:

```sh
cmake -S . -B build -DBWS_BUILD_BAROMETER_PLUGIN=OFF
```

## Requirements

- CMake >= 3.22
- A C++20-capable compiler for the reusable library surface
- Python 3, only if you build the tests
- Catch2 v3, only if you build the tests (`-DBUILD_TESTING=ON`); it is found via
  `find_package(Catch2 3)` or fetched at configure time.

## Available targets

Every enabled module exposes a `bws::bw_*` alias. Link only what you use.

With `BWS_BUILD_BAROMETER_PLUGIN=OFF`, the public tree exposes 9 library-mode
targets. These are the intended standalone OSS library surface: they do not link
JUCE and are covered by the library Docker lanes and native CI matrix.

| Target                   | Contents                                                      | JUCE? |
| ------------------------ | ------------------------------------------------------------- | ----- |
| `bws::bw_dsp_metering`   | LUFS / true-peak / loudness-range meters + conformance suite  | no    |
| `bws::bw_dsp_core`       | DSP primitives the meters build on                            | no    |
| `bws::bw_dsp_processors` | stereo / channel utility processors (width, balance, mono, …) | no    |
| `bws::bw_rt`             | real-time-safety: audio-thread guards, lock-free ring buffer  | no    |
| `bws::bw_audio_types`    | buffer / parameter value types                                | no    |
| `bws::bw_core`           | foundation types (`BwResult`, compiler-feature shims)         | no    |
| `bws::bw_ports`          | framework-agnostic plugin / parameter interfaces              | no    |
| `bws::bw_fft_adapters`   | FFT wrapper (vendored pffft)                                  | no    |
| `bws::bw_state_types`    | state blob, preset metadata, and system-info value types      | no    |

When `BWS_BUILD_BAROMETER_PLUGIN=ON`, the build also exposes the JUCE adapter,
Barometer preset support, UI, example support, and shipped UI sub-library
targets needed by the Barometer source build. These targets are present so the
reference plugin builds from source; they are not positioned as the standalone
library surface.

| Target                  | Contents                                                         | JUCE?   |
| ----------------------- | ---------------------------------------------------------------- | ------- |
| `bws::bw_preset_core`   | preset value model used by Barometer support                     | no      |
| `bws::bw_juce_adapters` | the JUCE host-adapter layer (`BwsAudioProcessor`, buffers)       | **yes** |
| `bws::bw_ui`            | token-driven UI primitives (+ `bw_ui_*` sub-libraries)           | **yes** |
| `bws::bw_toy_shell`     | small JUCE UI primitives used by Barometer's reference interface | **yes** |

The `bws::bw_ui*`, `bws::bw_toy_shell`, and `bws::bw_juce_adapters` targets link
JUCE. `bws::bw_preset_core` is JUCE-free, but it is grouped with Barometer
support because it exists here to support the reference plugin's preset path,
not as a standalone preset API. The JUCE targets are shipped as Barometer
support, not as a standalone public API.

Commercial product services, certification helpers, licensing/update services,
installers, signing/notarization artifacts, and unreleased plugin code are not
part of this public library surface.

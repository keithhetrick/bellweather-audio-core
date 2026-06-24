# Testing & verification

Bellweather Studios ships the loudness and true-peak metering with its
conformance suite. The intent is plain: the correctness claims in the README are
checkable from a clean clone, against the published specifications, without
trusting this repository's word for them. This document describes what the suite
covers and how it is built.

The default proof is the reusable library surface. Barometer is source-built
reference plugin code. The suite does not claim prebuilt plugin
binary release, installer validation, code signing, notarization, host loading,
or marketplace packaging.

## What ships

The suite lives in `modules/bw_dsp_metering/tests/`. Each file maps to a published
specification you can check it against:

| Test file                     | Specification                                                    | What it checks                                                                                                                             |
| ----------------------------- | ---------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------ |
| `bs1770_conformance_test.cpp` | ITU-R BS.1770-5 · EBU Tech 3341 (incl. cases 10, 13) · Tech 3342 | Integrated loudness, momentary/short-term convergence, absolute + relative gating, loudness range, and the 44.1-192 kHz sample-rate matrix |
| `bs1770_meter_test.cpp`       | BS.1770-5 · EBU Tech 3341 / 3342                                 | Published-target calibration cases, the off-thread analytics lifecycle, and the allocation-free audio path                                 |
| `true_peak_meter_test.cpp`    | BS.1770-5 §3.5 · EBU Tech 3341 case 20 · Nielsen/Lund bound      | Inter-sample true-peak with 4× oversampling, against the analytic bound                                                                    |
| `meter_ballistics_test.cpp`   | EBU Tech 3341 (M/S integration windows)                          | Momentary (400 ms) and short-term (3 s) time-window behaviour                                                                              |

The EBU Tech 3341 reference signals are committed under
`modules/bw_dsp_metering/tests/fixtures/tech3341/` as float WAVs, with
`META.json` recording each signal's spec case, expected value, and SHA-256. The
synthesiser that produced them (`tools/audiotest/synth_tech3341.py`) ships too,
so the fixtures can be regenerated and re-checked from the spec formulas.

## Standards and metrics

The suite uses independent public references rather than implementation-derived
expected values:

| Reference                                                                                                                       | Role in the suite                                                                                                                   |
| ------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------- |
| [ITU-R BS.1770](https://www.itu.int/rec/R-REC-BS.1770)                                                                          | Normative loudness and true-peak algorithm: K-weighting, gating, integrated loudness, and true-peak measurement.                    |
| [EBU loudness / R 128](https://tech.ebu.ch/loudness)                                                                            | Broadcast practice and terminology around EBU Mode: Momentary, Short-term, Integrated, LUFS, LU, LRA, and target-level conventions. |
| [EBU Tech 3341](https://tech.ebu.ch/publications/tech3341)                                                                      | Meter behavior and published test signals used for loudness and true-peak conformance cases.                                        |
| [EBU Tech 3342](https://tech.ebu.ch/publications/tech3342)                                                                      | Loudness Range (LRA) definition and stability behavior.                                                                             |
| [AES true-peak overview](https://aes.org/resources/audio-topics/loudness-project/learn-more/)                                   | Public background on inter-sample peaks and why true-peak meters upsample before measuring peaks.                                   |
| [Nielsen/Lund, "Overload in Signal Conversion"](https://www.semanticscholar.org/paper/ab1206497afbc620f48e893bcb6239b4653b7a8f) | Research context for inter-sample overload in digital-to-analog, sample-rate-conversion, and codec paths.                           |

The Nielsen/Lund inter-sample peak case used by the true-peak tests is research
context, not a standards-body requirement. It is useful because the signal has a
closed-form peak between samples: a sine at one quarter of the sample rate, with
the maximally displaced phase, has sample peaks 3.0103 dB below the reconstructed
continuous-time peak. That gives the implementation a hard analytic target in
addition to the EBU Tech 3341 acceptance bands.

Metric names in the test output:

| Metric             | Meaning                                                                                                  |
| ------------------ | -------------------------------------------------------------------------------------------------------- |
| `LUFS`             | Loudness Units relative to digital full scale; the absolute loudness unit used by BS.1770 and EBU R 128. |
| `LU`               | Relative loudness difference; 1 LU is equivalent to 1 dB of loudness difference.                         |
| `Integrated` / `I` | Programme loudness over the measured duration after BS.1770 absolute and relative gating.                |
| `Momentary` / `M`  | 400 ms loudness window.                                                                                  |
| `Short-term` / `S` | 3 s loudness window.                                                                                     |
| `LRA`              | Loudness Range, the gated spread of short-term loudness in LU.                                           |
| `dBTP`             | Decibels true peak, referenced to digital full scale after inter-sample peak estimation.                 |

## Running it

```sh
cmake --preset library -DBWS_WARNINGS_AS_ERRORS=ON
cmake --build --preset library
ctest --test-dir build-library --output-on-failure
```

The public CI runs the same source/library proof with a warnings-as-errors gate
(`BWS_WARNINGS_AS_ERRORS=ON`), plus three defensive lanes: clang-format
(`--dry-run --Werror`), ASan/UBSan, and TSan. `.clang-tidy` is checked in as the
public static-analysis profile for local review of bugprone and concurrency
issues.

For local ASan/UBSan runs on macOS with AppleClang, do not set
`ASAN_OPTIONS=detect_leaks=1`; AppleClang's AddressSanitizer does not support
LeakSanitizer there and will abort before running tests. Use
`ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` on macOS, or the CI-style
`ASAN_OPTIONS=halt_on_error=1` on Linux/toolchains where leak detection support is
available.

The metering allocation-counter tests install global allocation/deallocation
overrides in normal and ASan/UBSan builds. TSan builds intentionally skip those
overrides because the ThreadSanitizer C++ runtime owns the same interceptor
symbols; the TSan lane is the race detector, while the allocation-free hot-path
assertions remain covered by the normal and ASan/UBSan lanes.

The split lanes are useful while diagnosing:

```sh
ctest --test-dir build-library -L public-surface --output-on-failure      # public target consumption
ctest --test-dir build-library -R bw_dsp_metering --output-on-failure      # conformance
ctest --test-dir build-library -L framework-boundary --output-on-failure   # JUCE separation
```

**What success looks like.** The full run ends with:

```text
100% tests passed, 0 tests failed out of <platform count>
```

The exact count can move as coverage grows. The current launch suite reports 162
tests on macOS/Linux and 161 on Windows ARM64, where the Apple-only containment
probe is not registered. The public-surface lane reports one passing smoke test
per Stable/reusable module in `docs/MODULES.md`, and the framework-boundary lane
prints:

```text
PASS: core framework boundary holds (9 modules checked).
```

If you see these, the corresponding README claim is verified on your machine.

**Running a subset.** List the test entries, then run one by name:

```sh
ctest --test-dir build-library -N                                          # list every entry
ctest --test-dir build-library -R bw_dsp_metering_calibration --output-on-failure   # run one
```

`-DBWS_BUILD_BAROMETER_PLUGIN=OFF` builds the libraries and the suite with the
plugin adapter layer disabled. Catch2 is fetched at configure time, or used from
the system if already present. Python 3 is used by the fixture/conformance
support scripts during the test build.

The `framework-boundary` lane verifies the layering contract: JUCE-specific code
belongs in `bw_juce_adapters` and the Barometer plugin, not in the nine
Stable/reusable modules. It is a portable source check
(`tools/check_framework_boundary.py`) and can run directly or via
`ctest -L framework-boundary`.

## Native Windows Proof

Public CI runs the `library` preset on `windows-latest`, which is the normal
Windows x64/MSVC proof. Apple Silicon Macs can also run a local Windows ARM64
proof with Parallels. That lane is useful for catching MSVC portability issues,
but it is not a replacement for the x64 CI job.

See [`docs/WINDOWS_ARM64_PARALLELS.md`](docs/WINDOWS_ARM64_PARALLELS.md) for the
toolchain bootstrap, copy step, and repeatable proof command. The successful
local Parallels/MSVC ARM64 run uses the same public commands:

```powershell
python tools\check_manifest_coverage.py
cmake --preset library -DBWS_WARNINGS_AS_ERRORS=ON
cmake --build --preset library
ctest --test-dir build-library --output-on-failure
```

## Docker lanes

`tools/docker_lanes.sh` runs the default Linux library matrix from the shipped
`Dockerfile`: Clang with fetched Catch2, GCC with fetched Catch2, and GCC with
Ubuntu's system Catch2 package and test fetching disabled. These lanes build the
public-surface smokes and the metering conformance tests, then run the
public-surface, conformance, and JUCE-separation checks.

The Barometer VST3 source build is optional:

```sh
RUN_PLUGIN_LANES=1 tools/docker_lanes.sh
```

The script prints the current lane, elapsed time, and a heartbeat while Docker is
still building or checking an image. Docker's full build and run output is
captured in per-lane log files so successful runs stay readable. On failure, the
script prints the tail of the failing log and leaves the full path in the
terminal output.

The log directory defaults to `${TMPDIR:-/tmp}/bellweather-audio-core-docker-logs`.
Override it when you want logs in a project-local folder:

```sh
LOG_ROOT=build/docker-lane-logs tools/docker_lanes.sh
```

The default lanes are Linux clean-room library proof only. They do not prove
plugin signing, host loading, installers, notarization, marketplace packaging,
or native plugin binaries. The public CI matrix covers native library builds on
macOS and Windows/MSVC. The Barometer lane is expected to take longer because it
builds the JUCE-backed VST3 target from source.

## Failure map

Most failures fall into one of these gates:

| Gate                                                               | What it usually means                                                                                                | First checks                                                                                                                                                                   |
| ------------------------------------------------------------------ | -------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Docker cannot start                                                | Docker Desktop/daemon is not running, or the current user cannot reach it.                                           | Run `docker version`, then rerun `tools/docker_lanes.sh` from the repo root.                                                                                                   |
| Dependency fetch fails                                             | The lane needs network access for Ubuntu packages, Catch2, or JUCE.                                                  | Rerun with a working network. For test-only native builds, install Catch2 locally and configure with `-DBWS_FETCH_TEST_DEPS=OFF`.                                              |
| Configure fails before building                                    | CMake, compiler, or package discovery is incomplete for the selected lane.                                           | Re-run the printed `cmake -S ... -B ...` command directly; inspect the first CMake error above the failure.                                                                    |
| Configure fails on `Manually-specified variables were not used`    | A documented option is not wired into the current build graph.                                                       | Check the option spelling first. If it is spelled correctly, the build files and docs disagree.                                                                                |
| Configure fails on `Performing Test ... - Failed`                  | A feature probe failed and would make the public build look noisy.                                                   | Use the named Docker lane when reporting it; those probes should be quiet unless explicitly enabled.                                                                           |
| Build fails on `warning:` or `CMake Warning`                       | The public lanes treat warnings as release blockers.                                                                 | The first warning in the log is the one to fix; later lines may only be fallout.                                                                                               |
| Public-surface smoke fails                                         | A documented public module no longer works when linked from a downstream project.                                    | Run `ctest --test-dir build-library -L public-surface --output-on-failure`; check for missing `PUBLIC` include paths, compile definitions, or link dependencies.               |
| Metering tests fail                                                | A loudness, true-peak, fixture, or real-time allocation contract no longer matches the standards-backed expectation. | Run `ctest --test-dir build-library -R bw_dsp_metering --output-on-failure`, then inspect the named test case and `modules/bw_dsp_metering/tests/fixtures/tech3341/META.json`. |
| Framework-boundary fails                                           | A reusable core module now includes or links through the JUCE adapter layer.                                         | Run `ctest --test-dir build-library -L framework-boundary --output-on-failure`; the output names the module and include/link edge.                                             |
| Barometer lane fails in package discovery                          | The Linux plugin build is missing a JUCE system dependency.                                                          | Prefer the Docker lane first; for native Linux builds, mirror the packages listed in the `barometer-clang` Dockerfile stage.                                                   |
| Barometer lane reaches the final link step but no `.vst3` is found | Compilation completed, but JUCE packaging output moved or failed late.                                               | Run `find build-barometer/plugins/production/BwsBarometer -type d -name 'Barometer.vst3'` from the build tree.                                                                 |

## How the suite is written

- **Spec-anchored.** Tests cite ITU-R BS.1770-4/5 and EBU Tech 3341/3342 by
  section, and assert the spec's published target values - the EBU Tech 3341
  loudness targets and the Nielsen/Lund inter-sample bound - not numbers this
  implementation happened to produce. You can check them against the documents.

- **Positive and negative pairs.** The cases the spec names: silence, signals
  below the absolute gate, the gate boundary, and the relative-gate exclusions of
  EBU Tech 3341 cases 3 and 4 - where blocks that pass the absolute gate are still
  excluded by the relative one.

- **Measurable deltas.** Where a result is calibration-independent, it is asserted
  as one: an exact −X dB amplitude change reads as a −X LU integrated change, and
  block-level spread reads as the expected loudness range.

- **A sample-rate matrix anchored to 48 kHz.** At 48 kHz the K-weighting uses the
  published BS.1770 coefficient table verbatim, so that reading is the spec
  anchor; 44.1, 88.2, 96, 176.4, and 192 kHz are each required to match it.

- **Determinism.** The fixtures are synthesised from the spec formulas and
  SHA-pinned; the fixed-seed stimulus makes every run reproducible on any machine.

- **Real-time safety.** An allocation counter asserts that `process()` performs no
  heap allocation on the audio path, across block sizes including sub-block ones.
  ThreadSanitizer builds defer global allocation/deallocation interception to the
  sanitizer runtime instead of installing the allocation counter overrides.

## Scope

This suite demonstrates conformance, determinism, and the allocation-free audio
path - the properties `ctest` can prove on your machine. It is the metering
proof, not the whole test surface; it makes the README's metering claims
reproducible rather than asserted. The public v1.0.0 proof is source/library
proof, not prebuilt plugin binary proof.

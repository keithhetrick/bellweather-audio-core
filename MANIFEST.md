# Public Tree Manifest

This file is the path contract for `bellweather-audio-core`. Every shipped path
is here because it either builds the reusable libraries, proves their claims,
builds the Barometer source-built reference plugin, documents the public
contract, or supports the public repository workflow. It does not define a
prebuilt plugin binary, installer, signing, notarization, or marketplace
packaging release.

The coverage rules below are executable. Run:

```sh
python3 tools/check_manifest_coverage.py
```

The checker fails if a shipped file or directory is not covered, if a directory
is empty, if junk files appear, or if a `modules/bw_*` directory lacks a
`docs/MODULES.md` catalog entry.

## Root Contract

- `.clang-format` - C++ formatting standard (Allman, 4-space, 120 col); matches the upstream tree.
- `.clang-format-ignore` - Paths clang-format must not touch (vendored third-party, generated code).
- `.clang-tidy` - Sharpened bug-class lint config (bugprone/concurrency) for contributors.
- `.dockerignore` - Docker build-context filter for the zero-setup quickstart and lane images.
- `.github` - Public GitHub automation directory.
- `.github/ISSUE_TEMPLATE` - Public issue templates for bug reports and feature requests.
- `.github/ISSUE_TEMPLATE/bug_report.md` - Bug report template focused on reproducible library/plugin-source issues.
- `.github/ISSUE_TEMPLATE/feature_request.md` - Feature request template for public API and reference-plugin requests.
- `.github/PULL_REQUEST_TEMPLATE.md` - Pull request checklist for tests, formatting, DCO, and public-scope fit.
- `.github/workflows` - Workflow definitions for public CI and contribution policy.
- `.github/workflows/clang-format.yml` - Enforces the C++ formatting standard on pull requests.
- `.github/workflows/conformance.yml` - Runs the library preset on Linux, macOS, and Windows.
- `.github/workflows/dco.yml` - Enforces Signed-off-by lines on pull requests.
- `.github/workflows/sanitizers.yml` - Runs ASan/UBSan and TSan library lanes on Linux/Clang.
- `.gitattributes` - Text/binary and line-ending policy for cross-platform contributors.
- `.gitignore` - Keeps local build products out of the public source tree.
- `AUTHORS` - Human authorship and copyright ownership record.
- `CHANGELOG.md` - Public source release notes.
- `CMakeLists.txt` - Standalone root build for the public tree.
- `CMakePresets.json` - Library and local source-plugin configure/build presets.
- `CODE_OF_CONDUCT.md` - Contributor conduct policy.
- `CONTRIBUTING.md` - Contribution workflow, DCO rule, and patch expectations.
- `Dockerfile` - Zero-setup quickstart image plus named Linux build lanes.
- `Doxyfile` - Public API documentation generation configuration.
- `GOVERNANCE.md` - Maintainer, release, and compatibility governance notes.
- `LICENSE` - Apache-2.0 license text for the public source release.
- `LICENSING.md` - Plain-language license notes for Apache-2.0, JUCE, and built artifacts.
- `MANIFEST.md` - This path contract and manifest coverage source.
- `NOTICE` - Project notice for copyright and license identification.
- `README.md` - Public entry point, quickstart, architecture summary, and build guide.
- `SECURITY.md` - Vulnerability reporting policy.
- `SUPPORT.md` - Public support expectations.
- `TESTING.md` - What the shipped conformance suite proves and how to run it.
- `THIRD_PARTY_NOTICES.md` - Third-party components, licenses, and provenance.

## Build Support

- `cmake` - CMake helper modules and public registries.
- `cmake/BwsCatch2.cmake` - Fetches or locates Catch2 v3 for the metering conformance suite.
- `cmake/BwsChainExporter.cmake` - No-op-safe helper used by the Barometer CMake file.
- `cmake/BwsJuceDefaults.cmake` - Shared JUCE plugin defaults for the Barometer source build.
- `cmake/BwsJuceProvision.cmake` - Finds or fetches the pinned JUCE dependency for plugin builds.
- `cmake/BwsLinkerFlags.cmake` - Linker settings for standalone plugin/library builds.
- `cmake/BwsModuleSupport.cmake` - Optional C++20 named-module support detection.
- `cmake/BwsPIC.cmake` - Position-independent-code helper for Linux static-library links.
- `cmake/BwsPluginManifest.cmake` - Reads the public plugin manifest and defines `bws_add_juce_plugin`.
- `cmake/BwsStrictFlags.cmake` - Strict compiler flag application for first-party targets.
- `cmake/BwsVersions.cmake` - Pinned dependency versions used by the public build.
- `cmake/BwsVisibility.cmake` - Symbol visibility defaults and export helpers.
- `cmake/BwsWarnings.cmake` - Warning policy helper for first-party C++ targets.
- `cmake/BwsInstallExport.cmake` - Installs + exports the reusable library as the `find_package(BellweatherAudioCore)` package (`bws::` namespace).
- `cmake/BellweatherAudioCoreConfig.cmake.in` - Package config template consumed by `find_package(BellweatherAudioCore)`.
- `cmake/registries` - Public build registry directory.
- `cmake/registries/plugin_manifest.json` - Barometer reference-plugin identity, formats, and source-directory metadata.

## Design Tokens

- `bds` - Bellweather Design System token generator and shipped token products.
- `bds/tools` - Token code-generation tool directory.
- `bds/tools/generate_tokens.py` - Generates `bw_ui/generated/BwTokens.h` from product JSON.
- `bds/tokens` - Token data root.
- `bds/tokens/products` - Product token JSONs included in the public build closure.
- `bds/tokens/products/barometer.tokens.json` - Barometer-specific color/design tokens.
- `bds/tokens/products/rendering.tokens.json` - Rendering material and shadow tokens used by `bw_ui`.
- `bds/tokens/products/shared.tokens.json` - Shared design tokens used across the UI library.

## Documentation

- `docs` - Public documentation directory.
- `docs/MODULES.md` - Per-module catalog: purpose, public headers, deps, JUCE use, linkability, maturity.
- `docs/ARCHITECTURE.md` - Layer map, invariants, thread model, and key design choices for the public tree.
- `docs/USING_AS_A_LIBRARY.md` - How to consume the shipped modules from another CMake project.
- `docs/PUBLIC_API_CONTRACT.md` - Stability + behavior contract: fallible cold phase (`BwResult`) vs `noexcept` infallible hot path.
- `docs/WINDOWS_ARM64_PARALLELS.md` - Local Windows ARM64/MSVC proof runbook for Parallels.
- `docs/assets` - Public documentation media assets.
- `docs/assets/barometer.png` - README screenshot of the Barometer reference plugin.

## Modules

Module internals are documented at module granularity in `docs/MODULES.md`.
Each `modules/bw_*` path below is independently linkable through its `bws::`
target unless the catalog explicitly says the target is gated behind the
Barometer/JUCE build option.

- `modules` - C++ module root.
- `modules/CMakeLists.txt` - Dependency-ordered module build entry point.
- `modules/bw_audio_types/**` - Framework-neutral audio value types and smoothing primitives.
- `modules/bw_core/**` - Foundation result and compiler-feature utilities.
- `modules/bw_dsp_core/**` - Framework-neutral DSP constants, concepts, and primitive helpers.
- `modules/bw_dsp_metering/**` - BS.1770/EBU metering library plus conformance suite and fixtures.
- `modules/bw_dsp_processors/**` - Small reusable stereo/channel DSP processors.
- `modules/bw_fft_adapters/**` - FFT adapter over platform Accelerate or bundled pffft.
- `modules/bw_juce_adapters/**` - The JUCE host-adapter layer used by Barometer.
- `modules/bw_ports/**` - Framework-agnostic ports for plugin, scheduling, and preset/state persistence.
- `modules/bw_preset_core/**` - Barometer preset-support model, gated behind the plugin build option.
- `modules/bw_rt/**` - Real-time-safety guard, assertions, and lock-free utilities.
- `modules/bw_state_types/**` - Framework-neutral state blob, preset metadata, and system-info value types.
- `modules/bw_toy_shell/**` - Small JUCE UI primitives for Barometer's reference interface.
- `modules/bw_ui/**` - Token-driven JUCE UI library closure used by Barometer.

## Reference Plugin

- `plugins` - Public reference-plugin root.
- `plugins/production` - Reference-plugin directory group.
- `plugins/production/CMakeLists.txt` - Adds the Barometer source-built reference plugin.
- `plugins/production/BwsBarometer/**` - Barometer source-built reference plugin (VST3/AU), with public reference-plugin sources only.

## Public Tests

- `tests` - Public test sources for the reusable library surface.
- `tests/public_surface/**` - Tiny downstream compile/link smokes for Stable/reusable modules.

## Public Tools

- `tools` - Public helper scripts used by build, tests, and verification.
- `tools/audiotest` - Deterministic audio-test fixture tools.
- `tools/audiotest/synth_tech3341.py` - Regenerates and verifies the Tech 3341 fixture WAVs.
- `tools/check_framework_boundary.py` - Portable source check that verifies JUCE stays in the adapter/plugin layer.
- `tools/check_framework_boundary.sh` - Shell wrapper for the same framework-boundary check.
- `tools/check_manifest_coverage.py` - Verifies this manifest covers every shipped path.
- `tools/check_public_surface_smokes.py` - Verifies public-surface smokes match the module catalog.
- `tools/docker_lanes.sh` - Builds and runs every shipped Docker build lane.
- `tools/fonts` - Font-processing helpers for the Barometer UI build.
- `tools/fonts/subset_ui_fonts.py` - Subsets shipped UI fonts before JUCE binary-data embedding.

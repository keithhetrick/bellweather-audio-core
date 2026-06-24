# Third-Party Notices

Bellweather Audio Core, from Bellweather Studios, is licensed under
Apache-2.0. It uses the following third-party components, each under its own
license. Some are fetched at configure time; others are vendored source or
bundled assets in this repository.

## Linked at build time

- **JUCE** - the audio plugin framework. Used under JUCE's open-source terms
  (AGPLv3) or a commercial JUCE license. Fetched at configure time; not vendored
  in this repository. <https://juce.com> · <https://github.com/juce-framework/JUCE>

- **VST3 SDK** (Steinberg) - the VST3 plugin-format implementation, provided
  through JUCE for local Barometer VST3 source builds. Dual GPLv3 / Steinberg VST3
  license. VST is a trademark of Steinberg Media Technologies GmbH.
  <https://github.com/steinbergmedia/vst3sdk>

- **System audio frameworks** - Apple Accelerate (vDSP) on macOS. Linked from
  the platform; not vendored.

## Vendored in this repository

- **PFFFT** - a pretty fast FFT, by Julien Pommier (and FFTPACK by Paul N.
  Swarztrauber / UCAR). BSD-3-Clause-style license. See
  `modules/bw_fft_adapters/vendor/pffft/`.

## Bundled assets

- **Inter** - typeface by Rasmus Andersson. SIL Open Font License 1.1.
- **JetBrains Mono** - typeface by JetBrains. SIL Open Font License 1.1.

  Both are embedded as build-time font subsets via `tools/fonts/subset_ui_fonts.py`.

## Test tooling (not in the shipped plugin)

- **Catch2** - the C++ test framework used by the conformance suite. Boost
  Software License 1.0. Fetched at configure time when `-DBUILD_TESTING=ON`; not
  linked into any plugin binary. <https://github.com/catchorg/Catch2>

Each component remains under its respective license; Apache-2.0 applies to the
Bellweather Audio Core source.

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

## Optional circuit experiment dependencies

The diode example downloads ngspice 47 and the Python distributions pinned in
`examples/diode-detector/requirements.lock`. These dependencies are not bundled
in the source archive and are not covered by Bellweather's Apache-2.0 license.

- **ngspice** - circuit simulator. The Docker recipe verifies the release
  archive checksum. Its upstream license and copyright notices are in the
  release's `COPYING` file. <https://ngspice.sourceforge.io/>
- **NumPy, SciPy, Matplotlib** - numerical arrays, root solving and plots.
  Their releases include their respective license texts and third-party notices.
  <https://numpy.org/> · <https://scipy.org/> · <https://matplotlib.org/>
- **contourpy, cycler, fonttools, kiwisolver, packaging, pillow, pyparsing,
  python-dateutil, six** - transitive Python dependencies. Their installed
  distribution metadata and release archives supply their individual notices.

The source archive includes dependency identities and download instructions,
not a redistribution of these dependency binaries or an audio dataset.

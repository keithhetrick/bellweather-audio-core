# Changelog

## 1.0.1

Maintenance source release.

- Restores shipped framework class names in comments that the public-comment
  scrubber previously over-genericized (e.g. `BwsAudioProcessor`).
- Extraction now normalizes the tree with the pinned `clang-format` as a hard
  gate, so the published source always matches the public style.
- Syncs accumulated shared-library source and test updates from upstream.
- No public API or ABI change.

## 1.0.0

Initial public source release of `bellweather-audio-core`.

- Ships the reusable Bellweather audio-core library surface.
- Includes Barometer as source-built reference plugin code.
- Does not ship prebuilt Barometer plugin binaries, installers, signing
  artifacts, notarized packages, or redistributable plugin bundles.
- Includes BS.1770 / EBU Tech 3341 metering conformance tests and fixtures.
- Includes public-surface compile/link smoke tests for the stable reusable
  module targets.
- Publishes Bellweather source under Apache-2.0.

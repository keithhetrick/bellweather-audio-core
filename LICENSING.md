# Licensing, in plain English

_This is a plain-language summary, not legal advice. The binding terms are
[`LICENSE`](LICENSE) (Apache-2.0), [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md),
and JUCE's own license._

- **Bellweather source is Apache-2.0.** The reusable libraries, public tooling,
  and Barometer source-built reference plugin code are published under Apache-2.0
  unless a file says otherwise.

- **The libraries on their own do not link JUCE.** The DSP and conformance
  build (`-DBWS_BUILD_BAROMETER_PLUGIN=OFF`) does not build the JUCE adapter/plugin layer, so that
  path carries only Bellweather's Apache-2.0 source terms plus the listed
  third-party notices for fetched test/build dependencies.

- **A built plugin links third-party framework code.** This repository does not
  ship prebuilt Barometer plugin binaries. If you build and distribute one
  yourself, it links JUCE 8.0.12, so distributing that plugin binary requires
  satisfying JUCE's terms for the framework portion. JUCE is dual-licensed:
  AGPLv3 or a commercial JUCE license.

- **If you build the plugin yourself under JUCE 8.0.12's free terms, you are
  using JUCE under AGPLv3.** A binary you distribute under that path carries
  AGPLv3 obligations for the JUCE portion, including its network-use clause.
  Building for your own private use triggers none of this.

- **Bottom line:** source = Apache-2.0; the library-only build = Apache-2.0 plus
  third-party notices; a distributed Barometer plugin binary = Apache-2.0 for
  Bellweather source plus JUCE's AGPLv3 or a commercial JUCE license for the
  framework.

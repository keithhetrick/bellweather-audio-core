# Public API contract

This is the stability and behavior contract for the reusable `bws::` libraries.
It is what you can rely on as a consumer, and what we gate ourselves against.

## Two phases: fallible setup, infallible processing

The API is deliberately split in two, because an audio library lives on two
threads with opposite rules.

**Cold phase - configuration / setup (may fail, may allocate).**
Construction, `prepare(...)`, and preset/state load happen on a normal thread
before audio starts. Operations that can genuinely fail - chiefly the
externally-supplied surface, state deserialization - report failure explicitly
through a value type, `bws::domain::BwResult<T>` (an alias for
`std::expected<T, std::string_view>` on C++23, with a C++20 struct fallback).
They do not throw across the public boundary for expected, recoverable
conditions. (`prepare(double sampleRate, int maxBlockSize)` returns `void`: it is
configuration, not a fallible operation.)

```cpp
// bws::domain::IStateSerializer
BwResult<void> r = serializer.deserialize(blob);   // BwResult<T> = std::expected<T, std::string_view>
if (!r) { /* r.error() : std::string_view - recover; do not proceed to audio */ }
```

**Hot phase - the audio callback (cannot fail, cannot block).**
`bws::dsp::DSPModule::process(...)` and everything it calls is **`noexcept`,
allocation-free, lock-free, non-blocking, and total** - defined for every input, never
reporting an error, because there is no one on the audio thread to report to.
Invalid numeric input is scrubbed (non-finite values neutralized, denormals
flushed), not rejected. Anything that _could_ fail belongs in the cold phase.

```cpp
// bws::dsp::DSPModule
void process(bws::domain::BufferView buffer) noexcept;   // never throws, never allocates
```

## What this means for you

- Check `BwResult` once, during setup. After a successful `prepare`, the hot path
  is contractually safe to call from the audio thread every block.
- Do not call cold-phase methods from the audio thread.
- We will not change a `noexcept` hot-path method to throwing, or a `BwResult`
  setup method to throwing, within a major version.
- **Check the result before reading it.** `value()`/`operator*` are valid only on
  success. Error-access is the one place the two `BwResult` builds differ: on the
  C++23 `std::expected` path `value()` on an error throws `bad_expected_access`; on
  the C++20 fallback it returns a default. So always test `if (r)` / `r.has_value()`
  first - never call `value()` on an unchecked result.

## How we keep this from drifting

The contract is mechanically enforced, not just documented: the `bw_dsp_*`
modules are compiled with exceptions disabled, so introducing a throw on the
audio path is a build error, not a runtime surprise.

`BwResult`'s two implementations (C++23 `std::expected` and the C++20 fallback)
are both compiled and tested: the suite is built twice - once normally and once
with `-DBW_HAS_EXPECTED=0` - so the fallback path cannot silently rot on the
default C++20 library toolchain or the newer plugin/UI toolchain.

## Other surface conventions

- Public functions that return a value you must not ignore are `[[nodiscard]]`.
- Strings and buffers cross the boundary as `std::string_view` / `std::span`
  views - the caller owns the storage; we borrow for the duration of the call.
- Single-argument constructors are `explicit` unless implicit conversion is
  intended.

## Versioning, ABI, and deprecation

The library follows [Semantic Versioning](https://semver.org) on its **public API
surface** (the `bws::` headers you include and the targets you link):

- **Patch** - bug fixes, no surface change.
- **Minor** - additive, source-compatible changes (new functions/types); existing
  code keeps compiling.
- **Major** - a breaking change to the public surface.

**Distribution is source + static**, so there is no runtime `.so`/`.dylib` soname
to track: every published target is either header-only (an `INTERFACE` library)
or a static archive. "ABI" here therefore means **compile-time API
compatibility** - your translation units recompile against the new headers - not
a binary soname contract. For that reason the installed targets carry a project
`VERSION` but **no `SOVERSION`**; a soname would be meaningless for header-only
and statically-archived libraries.

**Deprecation:** a public API slated for removal is first marked `[[deprecated]]`
with a replacement named in the message, kept for the remainder of the current
major version, and removed only at the next major. The one documented behavioral
seam between builds is `BwResult` error-access (see above), which is governed by
the same within-major stability guarantee.

License terms across versions are governed by
[`LICENSE`](../LICENSE), [`LICENSING.md`](../LICENSING.md),
[`NOTICE`](../NOTICE), and [`THIRD_PARTY_NOTICES.md`](../THIRD_PARTY_NOTICES.md).
The source release is Apache-2.0; JUCE terms apply when building or distributing
JUCE-linked binaries.

See [`USING_AS_A_LIBRARY.md`](USING_AS_A_LIBRARY.md) for build/consume mechanics
and [`MODULES.md`](MODULES.md) for the per-module maturity tiers.

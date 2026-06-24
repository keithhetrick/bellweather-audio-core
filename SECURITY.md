# Security Policy

## Reporting a vulnerability

Please report security issues **privately** by email to
<keith@bellweatherstudios.com>. Do not open a public issue, pull request, or
discussion for a suspected vulnerability.

Include, where you can:

- the affected component (module, plugin, build, or the conformance suite),
- reproduction steps or a proof of concept,
- the impact you believe it has.

You can expect an acknowledgement within a few business days. Once a fix is
prepared we will coordinate disclosure timing with you.

## Scope

This repository ships framework-neutral audio DSP libraries and the source for
**Barometer**, a metering reference plugin built on them. Reports about the
DSP/library code, the Barometer source build, the build system, or the
conformance suite are in scope. Issues in third-party dependencies (JUCE, the
VST3 SDK, PFFFT, Catch2) should be reported upstream; let us know if a fix
requires a change on our side.

## Memory safety

This is a C++ library. Its threat model is **in-process and trusted-host**: the
code runs inside a DAW or host application the user already trusts, with no
network, IPC, or privilege boundary of its own. The only externally-supplied
input is **preset / plugin-state deserialization**; everything else is audio
samples and parameters from the host.

Given that model we harden C++ rather than rewrite, and we treat memory safety
as a layered, enforced property:

- **Ownership:** RAII throughout; the reusable core avoids raw owning pointers.
  Lifetimes are expressed in the type system, not by convention.
- **Bounds:** buffers cross interfaces as `std::span` / view types, not raw
  pointer + length pairs.
- **Sanitizers:** the public CI exercises the reusable library surface under
  AddressSanitizer, UndefinedBehaviorSanitizer, and ThreadSanitizer.
- **Real-time checks:** the metering tests include allocation counters around
  the documented audio-thread `process()` paths.
- **Numeric robustness:** DSP detectors scrub non-finite values (NaN/Inf) and
  flush denormals, so malformed numeric input cannot propagate undefined state.

We do **not** claim memory-safe-by-construction, and we have deliberately not
adopted a Rust rewrite or hardware CFI - both are out of proportion to an
in-process, trusted-host DSP library. Report suspected memory-safety issues via
the private channel above.

## Supported versions

Only the latest released version receives fixes.

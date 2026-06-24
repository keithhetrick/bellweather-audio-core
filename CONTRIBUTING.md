# Contributing to Bellweather Audio Core

Thanks for your interest in Bellweather Audio Core, an open-source C++ audio
library from Bellweather Studios. Bug reports, fixes, and improvements are
welcome.

## How to contribute

1. Open an issue describing the change (for anything non-trivial).
2. Fork, branch, and make your change.
3. Format your C++ with **clang-format** before committing - the style is defined in
   `.clang-format` (Allman braces, 4-space indent, 120 columns) and enforced by CI:
   `clang-format -i <your changed files>`. Use the LLVM major the project pins
   (`BWS_LLVM_PINNED_MAJOR` in `cmake/BwsVersions.cmake`) so output matches CI.
   `.clang-tidy` (bugprone/concurrency) is available for optional local linting.
4. Build and validate locally (see the README for build instructions).
5. Open a pull request with a clear description. The `clang-format` check must pass.

## C++ conventions

A few house rules keep the library portable and consistent across toolchains
(AppleClang/libc++, GCC/libstdc++, MSVC/MS-STL):

- **Naming:** `bws::` namespaces, `PascalCase` types, `camelCase` methods,
  trailing-underscore members (`cutoffHz_`), `k`-prefixed constants
  (`kMaxChannels`), `BW_`/`BWS_` SCREAMING_CASE macros.
- **Compiler attributes:** standard `[[attributes]]` (`[[nodiscard]]`, …) are
  written raw; **vendor-namespaced and legacy spellings are never raw** -
  `[[gnu::]]`/`[[clang::]]`/`[[msvc::]]`, `__attribute__`, `__forceinline` must
  go through a capability macro gated by `__has_cpp_attribute` (e.g.
  `BW_ALWAYS_INLINE`, `BW_LIFETIMEBOUND` in `bw_core/BwCompilerFeatures.h`). A
  raw vendor attribute is silent on Clang/GCC but warns (`C5030`) on MSVC.
- **Headers:** self-contained (`#include` what you use), `#pragma once`, no
  `using namespace` at file scope.
- **Standard:** the public surface targets **C++20**; capability-gate anything
  newer with `__cpp_lib_*` rather than compiler-version checks.
- **Real-time safety:** code reachable from the audio callback must not allocate,
  lock, or block; annotate it with `BWS_NONBLOCKING` and prefer non-owning views
  over `shared_ptr` on the hot path.
- **Ownership:** RAII throughout; `unique_ptr` for single ownership, `shared_ptr`
  only for genuinely shared lifetime, raw pointers/references are non-owning.

## Developer Certificate of Origin (DCO)

This project uses the [Developer Certificate of Origin](https://developercertificate.org/).
Every commit must be signed off, certifying that you wrote the change or
otherwise have the right to submit it under the project's license. Add a
sign-off line to each commit:

```
Signed-off-by: Your Name <your.email@example.com>
```

`git commit -s` adds this automatically. The name and email must be real.

## License

Contributions are accepted under the project's license, **Apache-2.0** (see
`LICENSE`). The DCO sign-off records your right to contribute; copyright in each
contribution remains with its author.

## Contribution terms

This project does not use a separate contributor license agreement. Pull
requests are accepted under Apache-2.0 through the DCO sign-off above.

## Contact

Keith Hetrick · <keith@bellweatherstudios.com>

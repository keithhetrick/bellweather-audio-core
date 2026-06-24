<!-- Thanks for contributing to Bellweather Audio Core. -->

## What & why

<!-- What does this change do, and why? Link any related issue (#123). -->

## Checklist

- [ ] Builds clean on at least one toolchain (`cmake --preset library && cmake --build --preset library`)
- [ ] Tests added/updated and passing (`ctest --test-dir build-library --output-on-failure`)
- [ ] Public API change is reflected in doc-comments + `CHANGELOG.md`
- [ ] No raw vendor compiler attributes (`[[gnu::]]`/`[[clang::]]`/`[[msvc::]]`) - use the capability macros (see `CONTRIBUTING.md`)
- [ ] Commits are signed off for the DCO (`git commit -s`)

## Notes for reviewers

<!-- Anything reviewers should focus on, trade-offs, or follow-ups. -->

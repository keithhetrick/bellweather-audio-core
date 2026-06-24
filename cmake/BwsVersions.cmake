# Bellweather Studios - pinned third-party versions
# Single source of truth for JUCE pinning; keep this in sync with documentation.
set(BWS_JUCE_PINNED_TAG "8.0.12")
set(BWS_JUCE_PINNED_SHA "29396c22c93392d6738e021b83196283d6e4d850")

# clang-format / clang-tidy: pin the LLVM MAJOR. clang-format output is
# version-sensitive, so local and CI must agree or every PR shows spurious format
# diffs. CI installs this LLVM major; bump in lockstep with the development
# toolchain.
set(BWS_LLVM_PINNED_MAJOR "22")

# Optional: print when invoked as `cmake -DBWS_PRINT_PINNED_TAG=ON -P cmake/BwsVersions.cmake`
if(BWS_PRINT_PINNED_TAG)
    message(STATUS "${BWS_JUCE_PINNED_TAG}")
endif()

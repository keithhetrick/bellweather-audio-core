// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

// Opaque handle for Processor's optional commercial hooks, held behind a
// single always-present std::unique_ptr so the object layout does not depend on
// build configuration.
//
// PRIVATE header - included only by.cpp TUs. Processor.h forward-declares
// `struct BarometerOptionalHooks`, so the public header stays free of any optional-hook
// types and the open-source build needs no extra modules.

#include <juce_audio_processors/juce_audio_processors.h>

#include <memory>


namespace bws::barometer
{


// Open-source build: empty handle. The unique_ptr member still exists in
// Processor (one pointer), so the object layout is unchanged; no
// commercial modules are linked.
struct BarometerOptionalHooks
{
    [[nodiscard]] bool hasOptionalHooks() const noexcept { return false; }
};


// Composition root for the optional commercial hooks. Reads only the wrapper type +
// JucePlugin_* constants (no processor state), so it is safe to call from the ctor
// in any order. Wires the hooks when present; returns an empty handle otherwise.
[[nodiscard]] std::unique_ptr<BarometerOptionalHooks> makeBarometerOptionalHooks(
    juce::AudioProcessor::WrapperType wrapperType);

} // namespace bws::barometer

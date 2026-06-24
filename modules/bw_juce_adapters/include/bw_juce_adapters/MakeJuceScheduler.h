// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_ports/IScheduler.h>
#include <memory>

namespace bws::adapters
{

// Factory function. JUCE-backed implementation (wrapping juce::Timer)
// buried in modules/bw_juce_adapters/src/JuceScheduler.h.
//
// LIFETIME NOTE: the returned scheduler privately inherits juce::Timer and
// must be destroyed on the JUCE message thread. Hold via std::unique_ptr in
// a class member declared BEFORE any consumer that holds it by reference,
// so destruction order (reverse of declaration) destroys the consumer
// first while the scheduler is still alive.
std::unique_ptr<bws::domain::IScheduler> makeJuceScheduler();

} // namespace bws::adapters

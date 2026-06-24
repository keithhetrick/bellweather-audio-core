// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_core/BwCompilerFeatures.h>
#include <juce_gui_basics/juce_gui_basics.h>

#if BW_HAS_SOURCE_LOCATION
#include <source_location>
#endif

namespace bws::ui
{

// Debug-only paint guard to catch heavy work accidentally placed in paint().
// Usage: drop BWS_PAINT_GUARD(); at the top of paint() overrides.
// It asserts if the current thread is not the message thread or if paint is re-entered.
struct PaintGuard
{
#if BW_HAS_SOURCE_LOCATION
    PaintGuard(std::source_location loc = std::source_location::current())
    {
#if JUCE_DEBUG
        jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
        jassert(!reentered); // basic re-entrancy check
        reentered = true;
#endif
        juce::ignoreUnused(loc);
    }
#else
    PaintGuard(const char* file, int line)
    {
#if JUCE_DEBUG
        jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
        jassert(!reentered); // basic re-entrancy check
        reentered = true;
#endif
        juce::ignoreUnused(file, line);
    }
#endif
    ~PaintGuard()
    {
#if JUCE_DEBUG
        reentered = false;
#endif
    }

private:
    static thread_local bool reentered;
};

} // namespace bws::ui

// Convenience macro for paint()
#if BW_HAS_SOURCE_LOCATION
#define BWS_PAINT_GUARD() ::bws::ui::PaintGuard paintGuardInstance
#else
#define BWS_PAINT_GUARD() ::bws::ui::PaintGuard paintGuardInstance(__FILE__, __LINE__)
#endif

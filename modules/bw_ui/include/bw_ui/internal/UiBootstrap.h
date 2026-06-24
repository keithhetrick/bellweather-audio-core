// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace bws::ui
{
// Thread-safe, idempotent bootstrap for shared UI systems (fonts, asset search roots).
class UiBootstrap
{
public:
    // Safe to call from every editor ctor; will run once per process.
    static void init();
};

} // namespace bws::ui

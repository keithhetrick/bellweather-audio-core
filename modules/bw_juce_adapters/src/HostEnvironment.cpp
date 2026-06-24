// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#include "bw_juce_adapters/HostEnvironment.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace bws::adapters
{

bool hostInterceptsPluginKeyboardInput()
{
    // Reaper on macOS is the documented canonical case (JUCE forum 2015-2024,
    // u-he FAQ, HISE forum, Surge GitHub). Reaper binds spacebar to transport
    // at the main-window level; child plugin windows don't receive the event
    // even when a focused TextEditor wants it.
    //
    // If Windows Reaper is ever confirmed NOT to hijack, tighten this to
    // `.isReaper() && SystemStats::getOperatingSystemType() == MacOSX_...`.
    // If other hosts (Logic, Pro Tools, Cubase) are confirmed to hijack,
    // add them with `||`. Call sites don't change - the seam is behavioral.
    return juce::PluginHostType().isReaper();
}

} // namespace bws::adapters

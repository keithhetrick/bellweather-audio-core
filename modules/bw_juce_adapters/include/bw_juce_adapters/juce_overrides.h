// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// =============================================================================
// juce_overrides.h - JUCE include surface with Clang Function Effect Analysis
// diagnostics suppressed.
//
// JUCE headers do not carry FEA annotations. When included inside a
// translation unit that contains [[clang::nonblocking]] code, JUCE's API
// surface generates a flood of -Wfunction-effects warnings - most at
// header-level symbol-table positions that have nothing to do with
// audio-thread hot paths.
//
// This header is THE single point of entry for JUCE includes in
// audio-thread-reachable bw_juce_adapters code. It bundles the audio-thread
// union of JUCE modules (audio_processors, audio_basics, dsp) plus their
// transitive deps, wrapping them in a pragma-suppressed region.
//
// Non-audio-thread headers (preset I/O, GUI bindings, bus layout policy)
// keep direct JUCE includes - FEA does not analyze them, so the wrapper
// adds no value at those seams.
//
// See:
//
//
//     (LMMS empirical anchor: tree-wide propagation infeasible; entry-points-
//      only + JUCE override-header is the deployed audio-domain pattern.)
// =============================================================================

#if defined(__clang__) && defined(__has_warning)
#pragma clang diagnostic push
#if __has_warning("-Wfunction-effects")
#pragma clang diagnostic ignored "-Wfunction-effects"
#endif
#if __has_warning("-Wperf-constraint-implies-noexcept")
#pragma clang diagnostic ignored "-Wperf-constraint-implies-noexcept"
#endif
#endif

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#if defined(__clang__) && defined(__has_warning)
#pragma clang diagnostic pop
#endif

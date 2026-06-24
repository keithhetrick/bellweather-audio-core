// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <optional>

namespace bws::ui::windowing
{

// Captures the native host-window state we need to reconstitute a hosted
// plugin editor after a fullscreen session. The editor owns content bounds;
// the host/backend owns native frame restoration around that content.
struct HostedWindowSessionState
{
    juce::Rectangle<int> nativeWindowFrame;
    juce::Rectangle<int> nativeViewInWindow;
    bool borderless {false};
};

#if defined(__APPLE__)
// Hosted fullscreen uses native frame-aware placement rather than assuming the
// editor view itself is the outermost fullscreen rectangle.
std::optional<HostedWindowSessionState> enterHostedFullscreenSession(const juce::Component& component);
bool exitHostedFullscreenSession(const juce::Component& component, const HostedWindowSessionState& state);

// Debug-only native geometry readout used by overlays and diagnostics.
juce::String debugHostedWindowGeometry(const juce::Component& component);

// Windowed hosted editors should remain inside the monitor's visible frame.
bool clampHostedWindowToVisibleFrame(const juce::Component& component);

// Host attach/open often needs one immediate clamp plus one async settle pass.
void settleHostedWindowPlacement(juce::Component& component);
#else
inline std::optional<HostedWindowSessionState> enterHostedFullscreenSession(const juce::Component&)
{
    return std::nullopt;
}

inline bool exitHostedFullscreenSession(const juce::Component&, const HostedWindowSessionState&)
{
    return false;
}

inline juce::String debugHostedWindowGeometry(const juce::Component&)
{
    return {};
}

inline bool clampHostedWindowToVisibleFrame(const juce::Component&)
{
    return false;
}

inline void settleHostedWindowPlacement(juce::Component&) {}
#endif

} // namespace bws::ui::windowing

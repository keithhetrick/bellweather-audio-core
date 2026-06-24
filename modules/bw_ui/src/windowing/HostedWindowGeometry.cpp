// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include <bw_ui/windowing/HostedWindowGeometry.h>

#if defined(__APPLE__)
namespace bws::ui::windowing
{

void settleHostedWindowPlacement(juce::Component& component)
{
    clampHostedWindowToVisibleFrame(component);

    juce::Component::SafePointer<juce::Component> safeComponent(&component);
    juce::MessageManager::callAsync([safeComponent]() {
        if (safeComponent != nullptr)
            clampHostedWindowToVisibleFrame(*safeComponent);
    });
}

} // namespace bws::ui::windowing
#endif

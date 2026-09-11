// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/Components/TooltipHub.h"

#include <utility>

namespace bws::ui
{

TooltipHub::TooltipHub(juce::Component* parentComp, const UiThemeResolved& theme, CopyResolver resolver, int delayMs,
                       EnabledResolver enabledResolver)
    : ThemedTooltipWindow(parentComp, theme, delayMs)
    , resolver_(std::move(resolver))
    , enabledResolver_(std::move(enabledResolver))
{}

void TooltipHub::registerControl(juce::Component& control, juce::String key)
{
    registry_[&control] = std::move(key);
}

void TooltipHub::unregisterControl(juce::Component& control) noexcept
{
    registry_.erase(&control);
}

juce::String TooltipHub::getTipFor(juce::Component& leaf)
{
    if (enabledResolver_ && !enabledResolver_())
        return {};

    for (juce::Component* c = &leaf; c != nullptr; c = c->getParentComponent())
    {
        const auto it = registry_.find(c);
        if (it != registry_.end())
            return resolver_ ? resolver_(it->second) : juce::String();
    }

    return baseTipFor(leaf);
}

juce::String TooltipHub::baseTipFor(juce::Component& leaf)
{
    return juce::TooltipWindow::getTipFor(leaf);
}

} // namespace bws::ui

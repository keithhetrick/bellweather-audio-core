// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_toy_shell/HiddenClick.h"

namespace bws::toy_shell
{

bool hasHiddenTriggerModifier(juce::ModifierKeys mods) noexcept
{
    return mods.isAltDown();
}

bool isHiddenTriggerClick(const juce::MouseEvent& event) noexcept
{
    return hasHiddenTriggerModifier(event.mods);
}

} // namespace bws::toy_shell

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace bws::ui::kernel
{

// On Windows/Linux JUCE aliases commandModifier == ctrlModifier; cmd and
// ctrl will hold the same bit after conversion. Kernel logic must not
// assume cmd != ctrl on those platforms.
struct ModSet
{
    bool cmd = false;
    bool alt = false;
    bool shift = false;
    bool ctrl = false;
};

} // namespace bws::ui::kernel

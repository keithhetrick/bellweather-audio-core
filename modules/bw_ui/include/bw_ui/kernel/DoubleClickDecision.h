// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "bw_ui/interaction/DoubleClickAction.h"

namespace bws::ui::kernel
{

enum class BareDoubleClickOutcome
{
    InvokeTextEntry,
    InvokeReset,
    InvokeCustom,
    NoOp,
    AutoDegradeTextEntry,
    AutoDegradeCustom,
    BaseFallthrough,
    DelegateToModifierPath,
};

struct WiringState
{
    bool hasTextEntryCallback = false;
    bool hasResetMechanism = false;
    bool hasCustomCallback = false;
    bool locked = false;
    bool textEntryDelegates = false;
};

[[nodiscard]] inline BareDoubleClickOutcome decideBareDoubleClick(DoubleClickAction a, WiringState w) noexcept
{
    if (w.locked)
        return BareDoubleClickOutcome::NoOp;

    switch (a)
    {
    case DoubleClickAction::TextEntry:
        if (w.textEntryDelegates)
            return BareDoubleClickOutcome::DelegateToModifierPath;
        return w.hasTextEntryCallback ? BareDoubleClickOutcome::InvokeTextEntry
                                      : BareDoubleClickOutcome::AutoDegradeTextEntry;
    case DoubleClickAction::Reset:
        return w.hasResetMechanism ? BareDoubleClickOutcome::InvokeReset : BareDoubleClickOutcome::BaseFallthrough;
    case DoubleClickAction::Custom:
        return w.hasCustomCallback ? BareDoubleClickOutcome::InvokeCustom : BareDoubleClickOutcome::AutoDegradeCustom;
    case DoubleClickAction::NoOp:
        return BareDoubleClickOutcome::NoOp;
    }
    return BareDoubleClickOutcome::NoOp;
}

} // namespace bws::ui::kernel

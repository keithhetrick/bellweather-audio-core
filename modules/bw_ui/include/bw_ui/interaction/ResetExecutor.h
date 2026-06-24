// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "bw_ui/interaction/InteractionPolicy.h"

#include <variant>

namespace bws::ui::interaction
{

inline void performReset(const ResetMechanism& m) noexcept
{
    if (auto* p = std::get_if<ParameterReset>(&m))
    {
        if (p->parameter == nullptr)
            return;
        p->parameter->beginGesture();
        p->parameter->setValueNormalized(
            p->overrideResetValueNormalized.value_or(p->parameter->getDefaultValueNormalized()));
        p->parameter->endGesture();
    }
    else if (auto* c = std::get_if<CallbackReset>(&m))
    {
        if (c->callback)
            c->callback();
    }
}

} // namespace bws::ui::interaction

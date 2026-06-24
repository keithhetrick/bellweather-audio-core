// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "bw_ui/interaction/DoubleClickAction.h"
#include "bw_ui/kernel/IParameter.h"

#include <functional>
#include <optional>
#include <string>
#include <variant>

namespace bws::ui::interaction
{

struct CallbackReset
{
    std::function<void()> callback;
};

struct ParameterReset
{
    kernel::IParameter* parameter = nullptr;
    std::optional<float> overrideResetValueNormalized {};
};

struct NoReset
{};

using ResetMechanism = std::variant<NoReset, CallbackReset, ParameterReset>;

struct DoubleClickPolicy
{
    DoubleClickAction action = DoubleClickAction::Reset;
    std::function<void()> onTextEntry;
    ResetMechanism reset {NoReset {}};
    std::function<void()> onCustom;
};

struct InteractionPolicy
{
    DoubleClickPolicy doubleClick {};
    std::optional<std::string> diagnosticName {};
};

} // namespace bws::ui::interaction

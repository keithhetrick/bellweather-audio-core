// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/interaction/ValueInteraction.h"

#include "bw_ui/kernel/DoubleClickDecision.h"

#include <algorithm>
#include <variant>

namespace bws::ui::interaction
{

ValueInteraction::ValueInteraction(kernel::IValueParameter& param, InteractionPolicy policy, kernel::FormatSpec spec,
                                   ValueInteractionConfig config)
    : param_(&param)
    , policy_(std::move(policy))
    , spec_(std::move(spec))
    , config_(config)
{}

float ValueInteraction::snap(float normalized) const
{
    if (config_.detent.enabled && std::abs(normalized - config_.detent.position) <= config_.detent.snapRadius)
    {
        return config_.detent.position;
    }
    return normalized;
}

void ValueInteraction::oneShotWrite(float normalized)
{
    kernel::setValueWithGesture(*param_, std::clamp(normalized, 0.0f, 1.0f));
}

void ValueInteraction::pointerDown(kernel::ModSet mods, bool isDoubleClick)
{
    (void)mods;
    if (isDoubleClick)
    {
        applyDoubleClick();
        return;
    }
    dragStartNorm_ = param_->getValueNormalized();
    armed_ = true; // gesture opens lazily on the first move (no empty gesture on a bare click)
}

void ValueInteraction::pointerDrag(int pixelsFromStart, kernel::ModSet mods)
{
    if (!armed_)
    {
        return;
    }
    if (!drag_)
    {
        drag_ = std::make_unique<kernel::ParameterGestureScope>(param_);
    }
    const kernel::InteractionPolicy sensPolicy {.sensitivity = config_.drag};
    const int sens = kernel::sensitivityForDrag(mods, sensPolicy);
    const float delta = static_cast<float>(pixelsFromStart) / static_cast<float>(sens > 0 ? sens : 1);
    drag_->setValueNormalized(snap(std::clamp(dragStartNorm_ + delta, 0.0f, 1.0f)));
}

void ValueInteraction::pointerUp()
{
    drag_.reset();
    armed_ = false;
}

void ValueInteraction::wheel(float deltaY, kernel::ModSet mods)
{
    const float scale = kernel::isFineGesture(mods) ? config_.wheelFine : config_.wheelNormal;
    oneShotWrite(snap(std::clamp(param_->getValueNormalized() + (deltaY * scale), 0.0f, 1.0f)));
}

void ValueInteraction::rebind(kernel::IValueParameter& param)
{
    drag_.reset(); // a gesture must not survive a rebind
    armed_ = false;
    param_ = &param;
    dragStartNorm_ = 0.0f;
}

std::string ValueInteraction::displayText() const
{
    return kernel::formatValue(param_->convertToReal(param_->getValueNormalized()) * config_.displayScale, spec_);
}

void ValueInteraction::commitText(std::string_view text)
{
    const auto parsed = kernel::parseValue(std::string(text), spec_);
    if (!parsed.has_value())
    {
        return; // garbage -> no gesture, no write
    }
    const double real = *parsed / config_.displayScale;
    oneShotWrite(param_->convertToNormalized(real));
}

void ValueInteraction::applyDoubleClick()
{
    const auto& dc = policy_.doubleClick;
    const kernel::WiringState wiring {
        .hasTextEntryCallback = static_cast<bool>(dc.onTextEntry),
        .hasResetMechanism = !std::holds_alternative<NoReset>(dc.reset),
        .hasCustomCallback = static_cast<bool>(dc.onCustom),
    };

    switch (kernel::decideBareDoubleClick(dc.action, wiring))
    {
    case kernel::BareDoubleClickOutcome::InvokeTextEntry:
        dc.onTextEntry();
        break;
    case kernel::BareDoubleClickOutcome::InvokeCustom:
        dc.onCustom();
        break;
    case kernel::BareDoubleClickOutcome::InvokeReset:
        std::visit(
            [this](const auto& r) {
                using T = std::decay_t<decltype(r)>;
                if constexpr (std::is_same_v<T, CallbackReset>)
                {
                    if (r.callback)
                    {
                        r.callback();
                    }
                }
                else if constexpr (std::is_same_v<T, ParameterReset>)
                {
                    auto* target = r.parameter != nullptr ? r.parameter : param_;
                    kernel::setValueWithGesture(
                        *target, r.overrideResetValueNormalized.value_or(target->getDefaultValueNormalized()));
                }
            },
            dc.reset);
        break;
    default:
        break; // NoOp / AutoDegrade* / BaseFallthrough / DelegateToModifierPath
    }
}

} // namespace bws::ui::interaction

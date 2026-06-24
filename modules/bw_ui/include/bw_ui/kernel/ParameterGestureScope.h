// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "bw_ui/kernel/IParameter.h"

#include <utility>

namespace bws::ui::kernel
{

// RAII gesture bracket. Holds a non-owning IParameter pointer; nullptr is
// silent no-op on both endpoints. Message-thread only.
class ParameterGestureScope
{
public:
    explicit ParameterGestureScope(IParameter* p) noexcept
        : p_(p)
    {
        if (p_ != nullptr)
        {
            p_->beginGesture();
        }
    }

    ~ParameterGestureScope() noexcept
    {
        if (p_ != nullptr)
        {
            p_->endGesture();
        }
    }

    ParameterGestureScope(const ParameterGestureScope&) = delete;
    ParameterGestureScope& operator=(const ParameterGestureScope&) = delete;

    // Defaulted move would copy the raw pointer, leaving source non-null and
    // double-closing on dtor. std::exchange nulls source explicitly.
    ParameterGestureScope(ParameterGestureScope&& other) noexcept
        : p_(std::exchange(other.p_, nullptr))
    {}

    ParameterGestureScope& operator=(ParameterGestureScope&& other) noexcept
    {
        if (this != &other)
        {
            if (p_ != nullptr)
            {
                p_->endGesture();
            }
            p_ = std::exchange(other.p_, nullptr);
        }
        return *this;
    }

    void setValueNormalized(float v) noexcept
    {
        if (p_ != nullptr)
        {
            p_->setValueNormalized(v);
        }
    }

private:
    IParameter* p_;
};

inline void setValueWithGesture(IParameter& param, float normalizedValue) noexcept
{
    ParameterGestureScope scope {&param};
    scope.setValueNormalized(normalizedValue);
}

} // namespace bws::ui::kernel

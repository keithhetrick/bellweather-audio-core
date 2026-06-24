// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace bws::ui::kernel
{

// Non-owning. All methods must be noexcept - ParameterGestureScope's dtor
// is noexcept and calls endGesture(); a throwing impl terminates.
class IParameter
{
public:
    virtual ~IParameter() = default;

    virtual void beginGesture() = 0;
    virtual void setValueNormalized(float v) = 0;
    virtual void endGesture() = 0;
    virtual float getDefaultValueNormalized() const = 0;
};

} // namespace bws::ui::kernel

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "bw_ui/kernel/IParameter.h"

namespace bws::ui::kernel
{

// Read + normalized<->real conversion atop the write/gesture seam. Gesture-only
// callers keep the minimal IParameter; an interactive value model needs reads to
// format the display and capture a drag's start value.
class IValueParameter : public IParameter
{
public:
    virtual float getValueNormalized() const = 0;
    virtual double convertToReal(float normalized) const = 0;
    virtual float convertToNormalized(double real) const = 0;
};

} // namespace bws::ui::kernel

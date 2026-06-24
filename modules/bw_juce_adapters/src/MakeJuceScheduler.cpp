// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_juce_adapters/MakeJuceScheduler.h"

#include "JuceScheduler.h"

namespace bws::adapters
{

std::unique_ptr<bws::domain::IScheduler> makeJuceScheduler()
{
    return std::make_unique<JuceScheduler>();
}

} // namespace bws::adapters

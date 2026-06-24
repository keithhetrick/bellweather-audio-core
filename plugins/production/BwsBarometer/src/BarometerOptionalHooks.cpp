// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#include "BarometerOptionalHooks.h"


namespace bws::barometer
{

std::unique_ptr<BarometerOptionalHooks> makeBarometerOptionalHooks(
    [[maybe_unused]] juce::AudioProcessor::WrapperType wrapperType)
{
    auto handle = std::make_unique<BarometerOptionalHooks>();


    return handle;
}

} // namespace bws::barometer

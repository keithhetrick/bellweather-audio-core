// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_juce_adapters/RetainValueTree.h"

#include "ValueTreeRetainer.h"

#include <juce_data_structures/juce_data_structures.h>

namespace bws::adapters
{

void retainValueTree(juce::ValueTree const& tree)
{
    ValueTreeRetainer::instance().retain(tree);
}

} // namespace bws::adapters

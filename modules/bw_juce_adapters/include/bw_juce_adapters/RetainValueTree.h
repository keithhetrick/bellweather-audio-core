// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// Forward decl avoids dragging the JUCE header into this public surface.
// Callers that pass a juce::ValueTree already include juce_data_structures.
namespace juce
{
class ValueTree;
}

namespace bws::adapters
{

// Free-function entry point for ValueTree lifetime retention. Consumers do not
// need to know about the backing storage or its threshold constants.
//
// Thread safety: message thread only (same constraint as ValueTreeRetainer).
void retainValueTree(juce::ValueTree const& tree);

} // namespace bws::adapters

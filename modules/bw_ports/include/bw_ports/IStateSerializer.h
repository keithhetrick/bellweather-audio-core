// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "bw_state_types/BwStateBlob.h"
#include "bw_core/BwResult.h"

namespace bws::domain
{

/**
 * Driven port: state persistence.
 * JUCE-backed implementations live in bw_juce_adapters.
 * Replaces JUCE XmlElement& in IPresetStatePersistence.
 *
 * Returned blob references internal storage.
 * Valid until next serialize() call or destruction.
 */
struct IStateSerializer
{
    virtual ~IStateSerializer() = default;
    [[nodiscard]] virtual BwResult<BwStateBlob> serialize() = 0;
    [[nodiscard]] virtual BwResult<void> deserialize(BwStateBlob blob) = 0;
};

} // namespace bws::domain

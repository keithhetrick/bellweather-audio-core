// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <bw_state_types/PresetMetadata.h>
#include <bw_state_types/BwStateBlob.h>
#include <bw_core/BwResult.h>

namespace bws
{

/**
 * Driven port: preset state persistence.
 *
 * Implemented by WeatherPresetManager (bw_ui).
 * Consumed by BwsAudioProcessor (bw_rt).
 *
 * Save: BwsAudioProcessor calls getPresetMetadata() to retrieve preset name,
 *       category, and modified flag, then writes them to its own XML.
 * Restore: BwsAudioProcessor serializes its XML to a blob, passes it here.
 *          Implementation parses the blob internally using JUCE - JUCE XML
 *          never crosses this seam.
 */
struct IPresetStatePersistence
{
    virtual ~IPresetStatePersistence() = default;

    /** Return current preset metadata for serialization.
     *  Returned string_views reference implementation-owned storage.
     *  Valid until next getPresetMetadata() call or destruction. */
    [[nodiscard]] virtual bws::domain::BwResult<bws::domain::PresetMetadata> getPresetMetadata() const = 0;

    /** Called before an owning processor restores its serialized parameter state. */
    virtual void beginPresetStateRestore() {}

    /** Called after an owning processor restores serialized parameter/preset state. */
    virtual void endPresetStateRestore() {}

    /** Restore preset metadata from serialized state blob.
     *  The blob must remain valid for the duration of this call. */
    [[nodiscard]] virtual bws::domain::BwResult<void> restorePresetState(bws::domain::BwStateBlob blob) = 0;
};

} // namespace bws

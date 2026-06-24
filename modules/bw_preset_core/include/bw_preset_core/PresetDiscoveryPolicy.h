// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_preset_core/PresetLibraryKey.h>
#include <bw_preset_core/PresetRecord.h>

#include <cstdint>
#include <string>

namespace bws::preset
{

struct PresetDiscoveryCandidate
{
    PresetMetadata metadata;
    std::int64_t lastModifiedEpochMillis = 0;
};

[[nodiscard]] inline std::string makePresetDiscoveryKey(const PresetMetadata& metadata)
{
    auto key = makePresetLibraryKey(metadata);
    if (!key.empty())
        return key;

    return metadata.identity.legacyKey();
}

[[nodiscard]] inline int presetFormatDiscoveryRank(PresetFormat format)
{
    switch (format)
    {
    case PresetFormat::Json:
        return 3;
    case PresetFormat::LegacyXml:
        return 2;
    case PresetFormat::Unknown:
        break;
    }

    return 0;
}

[[nodiscard]] inline bool shouldReplaceDiscoveredPreset(const PresetDiscoveryCandidate& existing,
                                                        const PresetDiscoveryCandidate& candidate)
{
    const auto existingKey = makePresetDiscoveryKey(existing.metadata);
    const auto candidateKey = makePresetDiscoveryKey(candidate.metadata);
    if (existingKey.empty() || existingKey != candidateKey)
        return false;

    const auto existingFormatRank = presetFormatDiscoveryRank(existing.metadata.format);
    const auto candidateFormatRank = presetFormatDiscoveryRank(candidate.metadata.format);
    if (candidateFormatRank != existingFormatRank)
        return candidateFormatRank > existingFormatRank;

    return candidate.lastModifiedEpochMillis > existing.lastModifiedEpochMillis;
}

} // namespace bws::preset

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_preset_core/PresetRecord.h>
#include <bw_preset_core/PresetStorage.h>

#include <string>
#include <vector>

namespace bws::preset
{

enum class PresetSaveMode
{
    CreateNew,
    OverwriteExisting
};

struct PresetRepositoryQuery
{
    std::string pluginId;
    bool includeFactory = true;
    bool includeUser = true;
};

class IPresetRepository
{
public:
    virtual ~IPresetRepository() = default;

    [[nodiscard]] virtual PresetStorageValueResult<std::vector<PresetMetadata>> listPresets(
        const PresetRepositoryQuery& query) const = 0;

    [[nodiscard]] virtual PresetStorageValueResult<PresetRecord> loadPreset(const PresetIdentity& identity) const = 0;

    [[nodiscard]] virtual PresetStorageResult savePreset(const PresetRecord& record, PresetSaveMode mode) = 0;

    [[nodiscard]] virtual PresetStorageValueResult<PresetRecord> renamePreset(const PresetIdentity& identity,
                                                                              std::string nextName,
                                                                              std::string nextCategory = {}) = 0;

    [[nodiscard]] virtual PresetStorageResult deletePreset(const PresetIdentity& identity) = 0;
};

[[nodiscard]] inline bool repositoryQueryIncludesSource(const PresetRepositoryQuery& query, PresetSource source)
{
    switch (source)
    {
    case PresetSource::Factory:
        return query.includeFactory;
    case PresetSource::User:
        return query.includeUser;
    case PresetSource::Unknown:
        break;
    }

    return false;
}

} // namespace bws::preset

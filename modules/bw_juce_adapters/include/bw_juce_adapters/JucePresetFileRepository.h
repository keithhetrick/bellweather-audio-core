// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_preset_core/PresetCodec.h>
#include <bw_preset_core/PresetRepository.h>

#include <juce_core/juce_core.h>

#include <optional>
#include <string>
#include <vector>

namespace bws::adapters
{

class JucePresetFileRepository final : public bws::preset::IPresetRepository
{
public:
    JucePresetFileRepository(std::string pluginId, juce::File userPresetRoot,
                             std::vector<juce::File> factoryPresetRoots, const bws::preset::IPresetCodec& codec);

    [[nodiscard]] bws::preset::PresetStorageValueResult<std::vector<bws::preset::PresetMetadata>> listPresets(
        const bws::preset::PresetRepositoryQuery& query) const override;

    [[nodiscard]] bws::preset::PresetStorageValueResult<bws::preset::PresetRecord> loadPreset(
        const bws::preset::PresetIdentity& identity) const override;

    [[nodiscard]] bws::preset::PresetStorageResult savePreset(const bws::preset::PresetRecord& record,
                                                              bws::preset::PresetSaveMode mode) override;

    [[nodiscard]] bws::preset::PresetStorageValueResult<bws::preset::PresetRecord> renamePreset(
        const bws::preset::PresetIdentity& identity, std::string nextName, std::string nextCategory = {}) override;

    [[nodiscard]] bws::preset::PresetStorageResult deletePreset(const bws::preset::PresetIdentity& identity) override;

private:
    struct FileRecord
    {
        juce::File file;
        bws::preset::PresetRecord record;
    };

    [[nodiscard]] std::vector<FileRecord> loadRecords(bool includeFactory, bool includeUser,
                                                      bool deduplicate = true) const;
    [[nodiscard]] std::optional<FileRecord> findRecord(const bws::preset::PresetIdentity& identity, bool includeFactory,
                                                       bool includeUser) const;
    [[nodiscard]] bws::preset::PresetStorageValueResult<bws::preset::PresetRecord> decodeFile(
        const juce::File& file, bws::preset::PresetSource source) const;
    [[nodiscard]] juce::File userFileForRecord(const bws::preset::PresetRecord& record) const;
    [[nodiscard]] static bool shouldReplaceDiscoveredRecord(const FileRecord& existing, const FileRecord& candidate);
    [[nodiscard]] bool identityBelongsToRepository(const bws::preset::PresetIdentity& identity) const;

    std::string pluginId_;
    juce::File userPresetRoot_;
    std::vector<juce::File> factoryPresetRoots_;
    const bws::preset::IPresetCodec& codec_;
};

} // namespace bws::adapters

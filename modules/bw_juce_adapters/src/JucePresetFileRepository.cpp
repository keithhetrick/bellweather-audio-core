// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_juce_adapters/JucePresetFileRepository.h"

#include <bw_preset_core/PresetCatalog.h>
#include <bw_preset_core/PresetDiscoveryPolicy.h>
#include <bw_preset_core/PresetPathPolicy.h>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <map>
#include <set>
#include <utility>

namespace bws::adapters
{
namespace
{

juce::String toJuceString(const std::string& value)
{
    return juce::String::fromUTF8(value.c_str());
}

std::vector<std::byte> bytesFromMemoryBlock(const juce::MemoryBlock& block)
{
    std::vector<std::byte> bytes(block.getSize());
    if (!bytes.empty())
        std::memcpy(bytes.data(), block.getData(), block.getSize());
    return bytes;
}

bool writeBytes(const juce::File& file, const std::vector<std::byte>& bytes)
{
    if (bytes.empty())
        return false;

    if (!file.getParentDirectory().exists() && !file.getParentDirectory().createDirectory())
        return false;

    return file.replaceWithData(bytes.data(), bytes.size());
}

bool sameFile(const juce::File& a, const juce::File& b)
{
    return a.getFullPathName() == b.getFullPathName();
}

} // namespace

JucePresetFileRepository::JucePresetFileRepository(std::string pluginId, juce::File userPresetRoot,
                                                   std::vector<juce::File> factoryPresetRoots,
                                                   const bws::preset::IPresetCodec& codec)
    : pluginId_(std::move(pluginId))
    , userPresetRoot_(std::move(userPresetRoot))
    , factoryPresetRoots_(std::move(factoryPresetRoots))
    , codec_(codec)
{}

bws::preset::PresetStorageValueResult<std::vector<bws::preset::PresetMetadata>> JucePresetFileRepository::listPresets(
    const bws::preset::PresetRepositoryQuery& query) const
{
    if (!query.pluginId.empty() &&
        bws::preset::normalizedIdentityText(query.pluginId) != bws::preset::normalizedIdentityText(pluginId_))
    {
        return bws::preset::PresetStorageValueResult<std::vector<bws::preset::PresetMetadata>>::success({});
    }

    std::vector<bws::preset::PresetMetadata> metadata;
    for (const auto& fileRecord : loadRecords(query.includeFactory, query.includeUser))
    {
        if (bws::preset::repositoryQueryIncludesSource(query, fileRecord.record.metadata.source))
            metadata.push_back(fileRecord.record.metadata);
    }

    return bws::preset::PresetStorageValueResult<std::vector<bws::preset::PresetMetadata>>::success(
        bws::preset::orderedPresetMetadata(std::move(metadata)));
}

bws::preset::PresetStorageValueResult<bws::preset::PresetRecord> JucePresetFileRepository::loadPreset(
    const bws::preset::PresetIdentity& identity) const
{
    if (!identityBelongsToRepository(identity))
        return bws::preset::PresetStorageValueResult<bws::preset::PresetRecord>::failure(
            bws::preset::PresetStorageStatus::NotFound);

    if (const auto found = findRecord(identity, true, true))
        return bws::preset::PresetStorageValueResult<bws::preset::PresetRecord>::success(found->record);

    return bws::preset::PresetStorageValueResult<bws::preset::PresetRecord>::failure(
        bws::preset::PresetStorageStatus::NotFound);
}

bws::preset::PresetStorageResult JucePresetFileRepository::savePreset(const bws::preset::PresetRecord& record,
                                                                      bws::preset::PresetSaveMode mode)
{
    if (!record.isLoadable() || !identityBelongsToRepository(record.metadata.identity))
        return bws::preset::PresetStorageResult::failure(bws::preset::PresetStorageStatus::InvalidArgument);

    const auto existing = findRecord(record.metadata.identity, false, true);
    if (mode == bws::preset::PresetSaveMode::CreateNew && existing.has_value())
        return bws::preset::PresetStorageResult::failure(bws::preset::PresetStorageStatus::AlreadyExists);

    auto userRecord = record;
    userRecord.metadata.source = bws::preset::PresetSource::User;
    userRecord.metadata.format = codec_.format();
    const auto targetFile = userFileForRecord(userRecord);
    if (mode == bws::preset::PresetSaveMode::CreateNew && targetFile.existsAsFile())
        return bws::preset::PresetStorageResult::failure(bws::preset::PresetStorageStatus::AlreadyExists);

    const auto encoded = codec_.encode(userRecord);
    if (!encoded)
        return encoded.result;

    if (!writeBytes(targetFile, encoded.value.bytes))
        return bws::preset::PresetStorageResult::failure(bws::preset::PresetStorageStatus::WriteFailed);

    for (const auto& staleRecord : loadRecords(false, true, false))
    {
        if (!bws::preset::matchesPresetLookupIdentity(staleRecord.record.metadata.identity,
                                                      userRecord.metadata.identity))
            continue;

        if (!sameFile(staleRecord.file, targetFile) && staleRecord.file.existsAsFile())
            staleRecord.file.deleteFile();
    }

    return bws::preset::PresetStorageResult::ok();
}

bws::preset::PresetStorageValueResult<bws::preset::PresetRecord> JucePresetFileRepository::renamePreset(
    const bws::preset::PresetIdentity& identity, std::string nextName, std::string nextCategory)
{
    const auto existing = findRecord(identity, false, true);
    if (!existing.has_value())
        return bws::preset::PresetStorageValueResult<bws::preset::PresetRecord>::failure(
            bws::preset::PresetStorageStatus::NotFound);

    auto renamed = existing->record.renamed(std::move(nextName), std::move(nextCategory));
    const auto saved = savePreset(renamed, bws::preset::PresetSaveMode::OverwriteExisting);
    if (!saved)
        return bws::preset::PresetStorageValueResult<bws::preset::PresetRecord>::failure(saved.status, saved.message);

    return bws::preset::PresetStorageValueResult<bws::preset::PresetRecord>::success(std::move(renamed));
}

bws::preset::PresetStorageResult JucePresetFileRepository::deletePreset(const bws::preset::PresetIdentity& identity)
{
    bool found = false;
    for (const auto& record : loadRecords(false, true, false))
    {
        if (!bws::preset::matchesPresetLookupIdentity(record.record.metadata.identity, identity))
            continue;

        found = true;
        if (!record.file.deleteFile())
            return bws::preset::PresetStorageResult::failure(bws::preset::PresetStorageStatus::DeleteFailed);
    }

    if (!found)
        return bws::preset::PresetStorageResult::failure(bws::preset::PresetStorageStatus::NotFound);

    return bws::preset::PresetStorageResult::ok();
}

std::vector<JucePresetFileRepository::FileRecord> JucePresetFileRepository::loadRecords(bool includeFactory,
                                                                                        bool includeUser,
                                                                                        bool deduplicate) const
{
    std::vector<FileRecord> records;
    const auto extension = toJuceString(bws::preset::extensionForPresetFormat(codec_.format()));

    auto addRoot = [&](const juce::File& root, bws::preset::PresetSource source) {
        if (!root.isDirectory())
            return;

        juce::Array<juce::File> files;
        std::set<juce::String> seenPaths;

        auto collect = [&](const juce::String& pattern) {
            juce::Array<juce::File> matches;
            root.findChildFiles(matches, juce::File::findFiles, true, pattern);
            for (const auto& file : matches)
            {
                if (seenPaths.insert(file.getFullPathName()).second)
                    files.add(file);
            }
        };

        collect("*" + extension);
        if (source == bws::preset::PresetSource::Factory && extension == ".bwpreset.json")
        {
            collect("*.json");
        }

        files.sort();

        for (const auto& file : files)
        {
            auto decoded = decodeFile(file, source);
            if (decoded)
                records.push_back(FileRecord {file, std::move(decoded.value)});
        }
    };

    if (includeFactory)
    {
        for (const auto& root : factoryPresetRoots_)
            addRoot(root, bws::preset::PresetSource::Factory);
    }

    if (includeUser)
        addRoot(userPresetRoot_, bws::preset::PresetSource::User);

    if (!deduplicate)
        return records;

    std::map<std::string, FileRecord> recordsByIdentity;
    for (auto& record : records)
    {
        auto key = bws::preset::makePresetDiscoveryKey(record.record.metadata);

        auto it = recordsByIdentity.find(key);
        if (it == recordsByIdentity.end())
        {
            recordsByIdentity.emplace(std::move(key), std::move(record));
            continue;
        }

        if (shouldReplaceDiscoveredRecord(it->second, record))
            it->second = std::move(record);
    }

    std::vector<FileRecord> deduplicatedRecords;
    deduplicatedRecords.reserve(recordsByIdentity.size());
    for (auto& [_, record] : recordsByIdentity)
        deduplicatedRecords.push_back(std::move(record));

    return deduplicatedRecords;
}

std::optional<JucePresetFileRepository::FileRecord> JucePresetFileRepository::findRecord(
    const bws::preset::PresetIdentity& identity, bool includeFactory, bool includeUser) const
{
    for (auto& record : loadRecords(includeFactory, includeUser))
    {
        if (bws::preset::matchesPresetLookupIdentity(record.record.metadata.identity, identity))
            return record;
    }

    return std::nullopt;
}

bws::preset::PresetStorageValueResult<bws::preset::PresetRecord> JucePresetFileRepository::decodeFile(
    const juce::File& file, bws::preset::PresetSource source) const
{
    juce::MemoryBlock bytes;
    if (!file.loadFileAsData(bytes))
        return bws::preset::PresetStorageValueResult<bws::preset::PresetRecord>::failure(
            bws::preset::PresetStorageStatus::ReadFailed);

    auto decoded = codec_.decode(bws::preset::makeEncodedDocument(codec_.format(), bytesFromMemoryBlock(bytes)));
    if (!decoded)
        return decoded;

    decoded.value.metadata.source = source;
    decoded.value.metadata.format = codec_.format();
    if (!identityBelongsToRepository(decoded.value.metadata.identity))
        return bws::preset::PresetStorageValueResult<bws::preset::PresetRecord>::failure(
            bws::preset::PresetStorageStatus::InvalidArgument);

    return decoded;
}

juce::File JucePresetFileRepository::userFileForRecord(const bws::preset::PresetRecord& record) const
{
    const auto relativePath = bws::preset::userPresetRelativePathForMetadata(record.metadata);
    auto file = userPresetRoot_;
    for (const auto& directory : relativePath.directories)
        file = file.getChildFile(toJuceString(directory));

    return file.getChildFile(toJuceString(relativePath.filename));
}

bool JucePresetFileRepository::shouldReplaceDiscoveredRecord(const FileRecord& existing, const FileRecord& candidate)
{
    return bws::preset::shouldReplaceDiscoveredPreset(
        {existing.record.metadata, existing.file.getLastModificationTime().toMilliseconds()},
        {candidate.record.metadata, candidate.file.getLastModificationTime().toMilliseconds()});
}

bool JucePresetFileRepository::identityBelongsToRepository(const bws::preset::PresetIdentity& identity) const
{
    return bws::preset::normalizedIdentityText(identity.pluginId) == bws::preset::normalizedIdentityText(pluginId_);
}

} // namespace bws::adapters

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_preset_core/PresetIdentity.h>
#include <bw_preset_core/PresetStorage.h>

#include <string>
#include <utility>
#include <vector>

namespace bws::preset
{

struct PresetMigrationReceipt
{
    std::string migrationId;
    int targetSchemaVersion = 0;
    std::vector<std::string> migratedPresetKeys;
    bool completed = false;
};

struct PresetMigrationReport
{
    std::string migrationId;
    int discoveredCount = 0;
    int migratedCount = 0;
    int skippedCount = 0;
    std::vector<std::string> diagnostics;

    [[nodiscard]] bool succeeded() const { return diagnostics.empty(); }
    [[nodiscard]] explicit operator bool() const { return succeeded(); }
};

[[nodiscard]] inline bool migrationReceiptMatches(const PresetMigrationReceipt& receipt, const std::string& migrationId,
                                                  int targetSchemaVersion)
{
    return receipt.completed && receipt.migrationId == migrationId &&
           receipt.targetSchemaVersion == targetSchemaVersion;
}

[[nodiscard]] inline bool shouldRunMigration(const PresetMigrationReceipt& receipt, const std::string& migrationId,
                                             int targetSchemaVersion)
{
    return !migrationReceiptMatches(receipt, migrationId, targetSchemaVersion);
}

[[nodiscard]] inline PresetMigrationReceipt completedMigrationReceipt(std::string migrationId, int targetSchemaVersion,
                                                                      std::vector<PresetIdentity> migratedIdentities)
{
    PresetMigrationReceipt receipt;
    receipt.migrationId = std::move(migrationId);
    receipt.targetSchemaVersion = targetSchemaVersion;
    receipt.completed = true;
    receipt.migratedPresetKeys.reserve(migratedIdentities.size());

    for (const auto& identity : migratedIdentities)
        receipt.migratedPresetKeys.push_back(identity.canonicalKey());

    return receipt;
}

[[nodiscard]] inline PresetStorageResult migrationReportToStorageResult(const PresetMigrationReport& report)
{
    if (report)
        return PresetStorageResult::ok();

    if (!report.diagnostics.empty())
        return PresetStorageResult::failure(PresetStorageStatus::MigrationFailed, report.diagnostics.front());

    return PresetStorageResult::failure(PresetStorageStatus::MigrationFailed);
}

} // namespace bws::preset

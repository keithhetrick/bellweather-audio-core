// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include <utility>

namespace bws::preset
{

enum class PresetStorageStatus
{
    Ok,
    NotFound,
    AlreadyExists,
    InvalidArgument,
    UnsupportedFormat,
    ReadFailed,
    WriteFailed,
    DeleteFailed,
    MigrationFailed
};

struct PresetStorageResult
{
    PresetStorageStatus status = PresetStorageStatus::Ok;
    std::string message;

    [[nodiscard]] static PresetStorageResult ok() { return {}; }

    [[nodiscard]] static PresetStorageResult failure(PresetStorageStatus status, std::string message = {})
    {
        return {status, std::move(message)};
    }

    [[nodiscard]] bool succeeded() const { return status == PresetStorageStatus::Ok; }
    [[nodiscard]] explicit operator bool() const { return succeeded(); }
};

template <typename Value>
struct PresetStorageValueResult
{
    PresetStorageResult result;
    Value value {};

    [[nodiscard]] static PresetStorageValueResult success(Value value)
    {
        return {PresetStorageResult::ok(), std::move(value)};
    }

    [[nodiscard]] static PresetStorageValueResult failure(PresetStorageStatus status, std::string message = {})
    {
        return {PresetStorageResult::failure(status, std::move(message)), {}};
    }

    [[nodiscard]] bool succeeded() const { return result.succeeded(); }
    [[nodiscard]] explicit operator bool() const { return succeeded(); }
};

} // namespace bws::preset

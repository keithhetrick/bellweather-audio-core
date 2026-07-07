// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <string_view>

namespace bws::domain
{

/// Preset state metadata returned by IPresetStatePersistence::getPresetMetadata().
///
/// name and category reference storage is owned by IPresetStatePersistence.
/// Valid until the next getPresetMetadata() call or owner destruction.
/// Consume immediately - do not store across calls.
struct PresetMetadata
{
    std::string_view name;
    std::string_view category;
    bool modified = false;
    std::string_view stableId;
};

} // namespace bws::domain

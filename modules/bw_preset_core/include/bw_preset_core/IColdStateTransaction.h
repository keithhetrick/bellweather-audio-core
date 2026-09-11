// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_preset_core/PresetState.h>
#include <bw_state_types/BwStateBlob.h>

#include <cstdint>
#include <string_view>

namespace bws::preset
{

enum class ColdStateIntent : std::uint8_t
{
    hostSession,
    presetFile,
    previewApply,
    previewCancel,
};

struct ColdStateIdentityContext
{
    std::string_view pluginId;
    std::string_view stableId;
    std::string_view stateSha256;
    bool isFactory {};

    [[nodiscard]] bool present() const noexcept { return !pluginId.empty(); }
};

struct PresetMetadataContext
{
    std::string_view name;
    std::string_view category;
    std::string_view stableId;
    bool modified {};
};

enum class StateCaptureStatus : std::uint8_t
{
    captured,
    capturedUnverified,
    wrongThread,
    internalMetadataInvalid,
    unsupported,
    failed,
};

struct StateCaptureResult
{
    StateCaptureStatus status {StateCaptureStatus::failed};
    PresetStateBytes bytes;

    [[nodiscard]] bool verified() const noexcept { return status == StateCaptureStatus::captured; }
};

enum class StateApplyStatus : std::uint8_t
{
    controlStateCommitted,
    appliedUnverified,
    appliedDefaultsForTrial,
    legacyIdentityAssumed,
    wrongThread,
    rejected,
    identityMismatch,
    internalMetadataInvalid,
    commitIndeterminate,
};

struct GenerationToken
{
    std::uint64_t instanceNonce {};
    std::uint64_t controlGeneration {};

    [[nodiscard]] bool valid() const noexcept { return instanceNonce != 0 && controlGeneration != 0; }
    friend bool operator==(const GenerationToken&, const GenerationToken&) = default;
};

struct StateApplyResult
{
    StateApplyStatus status {StateApplyStatus::rejected};
    std::uint64_t controlGeneration {};
    bool semanticNoOp {};
    std::uint64_t instanceNonce {};
    std::uint64_t externalAutomationEpochAtCommit {};

    [[nodiscard]] GenerationToken token() const noexcept { return {instanceNonce, controlGeneration}; }

    [[nodiscard]] bool verifiedCommit() const noexcept
    {
        return status == StateApplyStatus::controlStateCommitted ||
               status == StateApplyStatus::appliedDefaultsForTrial || status == StateApplyStatus::legacyIdentityAssumed;
    }
};

// Cold deployment seam shared by JUCE and native CLAP preset adapters. Calls
// are forbidden from the audio thread; implementations must reject before
// allocating, locking, parsing, or touching streams.
class IColdStateTransaction
{
public:
    virtual ~IColdStateTransaction() = default;
    [[nodiscard]] virtual StateCaptureResult captureColdState() const = 0;
    [[nodiscard]] virtual StateCaptureResult captureColdStateWithPresetMetadata(bws::domain::BwStateBlob,
                                                                                PresetMetadataContext) const
    {
        return {StateCaptureStatus::unsupported, {}};
    }
    [[nodiscard]] virtual StateApplyResult applyColdState(bws::domain::BwStateBlob, ColdStateIntent) = 0;
    [[nodiscard]] virtual StateApplyResult applyColdStateWithContext(bws::domain::BwStateBlob state,
                                                                     ColdStateIntent intent, ColdStateIdentityContext)
    {
        return applyColdState(state, intent);
    }
};

} // namespace bws::preset

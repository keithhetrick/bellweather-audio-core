// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
// Contract source: modules/bw_ports/include/bw_ports/IPresetStatePersistence.h
// Assertions derived from interface promises, not implementations.
// Do NOT read WeatherPresetManager or any bw_ui header when authoring these tests.

#include <catch2/catch_test_macros.hpp>
#include <bw_ports/IPresetStatePersistence.h>
#include <string>

namespace
{

// Backing storage for string_view lifetime (same pattern as WeatherPresetManager)
static const std::string kTestName = "TestPreset";
static const std::string kTestCategory = "TestCategory";

struct TestPresetPersistence : bws::IPresetStatePersistence
{
    bool modified = false;
    bool restoreCalled = false;
    size_t lastBlobSize = 0;
    std::string stableId = "7ffb61f6-af73-4fce-af71-a8eccfb3ad01";

    [[nodiscard]] bws::domain::BwResult<bws::domain::PresetMetadata> getPresetMetadata() const override
    {
        return bws::domain::BwSuccess(bws::domain::PresetMetadata {
            std::string_view(kTestName), std::string_view(kTestCategory), modified, std::string_view(stableId)});
    }

    [[nodiscard]] bws::domain::BwResult<void> restorePresetState(bws::domain::BwStateBlob blob) override
    {
        restoreCalled = true;
        lastBlobSize = blob.size();
        return bws::domain::BwSuccessVoid();
    }
};

} // namespace

// =============================================================================
// Positive: getPresetMetadata returns valid metadata
// =============================================================================

TEST_CASE("IPresetStatePersistence - getPresetMetadata returns name and category", "[core][contract][preset]")
{
    TestPresetPersistence stub;
    auto result = stub.getPresetMetadata();

    REQUIRE(result.has_value());
    REQUIRE(result->name == "TestPreset");
    REQUIRE(result->category == "TestCategory");
}

// =============================================================================
// Negative: getPresetMetadata with modified flag
// =============================================================================

TEST_CASE("IPresetStatePersistence - getPresetMetadata reports modified state", "[core][contract][preset]")
{
    TestPresetPersistence stub;

    stub.modified = false;
    auto clean = stub.getPresetMetadata();
    REQUIRE_FALSE(clean->modified);

    stub.modified = true;
    auto dirty = stub.getPresetMetadata();
    REQUIRE(dirty->modified);
}

// =============================================================================
// Positive: restorePresetState accepts a blob
// =============================================================================

TEST_CASE("IPresetStatePersistence - restorePresetState accepts blob", "[core][contract][preset]")
{
    TestPresetPersistence stub;

    std::byte data[8] = {};
    bws::domain::BwStateBlob blob {std::span<const std::byte>(data, 8)};

    auto result = stub.restorePresetState(blob);

    REQUIRE(result.has_value());
    REQUIRE(stub.restoreCalled);
    REQUIRE(stub.lastBlobSize == 8);
}

// =============================================================================
// Negative: restorePresetState with empty blob
// =============================================================================

TEST_CASE("IPresetStatePersistence - restorePresetState with empty blob is safe", "[core][contract][preset]")
{
    TestPresetPersistence stub;
    bws::domain::BwStateBlob emptyBlob;

    auto result = stub.restorePresetState(emptyBlob);

    REQUIRE(result.has_value());
    REQUIRE(stub.restoreCalled);
    REQUIRE(stub.lastBlobSize == 0);
}

// =============================================================================
// Positive: virtual dispatch through interface pointer
// =============================================================================

TEST_CASE("IPresetStatePersistence - virtual dispatch through pointer", "[core][contract][preset]")
{
    TestPresetPersistence stub;
    bws::IPresetStatePersistence* iface = &stub;

    auto meta = iface->getPresetMetadata();
    REQUIRE(meta.has_value());
    REQUIRE(meta->name == "TestPreset");
}

// =============================================================================
// Positive: metadata string_view references are valid after call
// =============================================================================

TEST_CASE("IPresetStatePersistence - metadata string_views are valid", "[core][contract][preset]")
{
    TestPresetPersistence stub;
    auto result = stub.getPresetMetadata();

    // string_views must reference valid storage (mutable member strings in impl)
    REQUIRE(result.has_value());
    REQUIRE(result->name.data() != nullptr);
    REQUIRE(result->name.size() > 0);
    REQUIRE(result->category.data() != nullptr);
    REQUIRE(result->category.size() > 0);
    REQUIRE(result->stableId.data() != nullptr);
    REQUIRE(result->stableId.size() > 0);
}

// =============================================================================
// Positive: nullptr path (BwsAudioProcessor::getPresetManagerPtr default)
// =============================================================================

TEST_CASE("IPresetStatePersistence - nullptr is the default contract", "[core][contract][preset]")
{
    bws::IPresetStatePersistence* ptr = nullptr;
    REQUIRE(ptr == nullptr);
}

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_preset_core/PresetRecord.h>
#include <bw_preset_core/PresetStorage.h>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace bws::preset
{

struct PresetEncodedDocument
{
    PresetFormat format = PresetFormat::Unknown;
    std::vector<std::byte> bytes;
};

class IPresetCodec
{
public:
    virtual ~IPresetCodec() = default;

    [[nodiscard]] virtual PresetFormat format() const = 0;
    [[nodiscard]] virtual bool canDecode(const PresetEncodedDocument& document) const = 0;

    [[nodiscard]] virtual PresetStorageValueResult<PresetRecord> decode(
        const PresetEncodedDocument& document) const = 0;

    [[nodiscard]] virtual PresetStorageValueResult<PresetEncodedDocument> encode(const PresetRecord& record) const = 0;
};

[[nodiscard]] inline bool encodedDocumentHasBytes(const PresetEncodedDocument& document)
{
    return !document.bytes.empty();
}

[[nodiscard]] inline PresetEncodedDocument makeEncodedDocument(PresetFormat format, std::vector<std::byte> bytes)
{
    return PresetEncodedDocument {format, std::move(bytes)};
}

} // namespace bws::preset

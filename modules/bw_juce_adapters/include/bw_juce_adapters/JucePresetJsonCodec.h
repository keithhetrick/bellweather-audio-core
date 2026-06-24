// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_preset_core/PresetCodec.h>

#include <juce_core/juce_core.h>

namespace bws::adapters
{

class JucePresetJsonCodec final : public bws::preset::IPresetCodec
{
public:
    [[nodiscard]] bws::preset::PresetFormat format() const override;
    [[nodiscard]] bool canDecode(const bws::preset::PresetEncodedDocument& document) const override;

    [[nodiscard]] bws::preset::PresetStorageValueResult<bws::preset::PresetRecord> decode(
        const bws::preset::PresetEncodedDocument& document) const override;

    [[nodiscard]] bws::preset::PresetStorageValueResult<bws::preset::PresetEncodedDocument> encode(
        const bws::preset::PresetRecord& record) const override;
};

} // namespace bws::adapters

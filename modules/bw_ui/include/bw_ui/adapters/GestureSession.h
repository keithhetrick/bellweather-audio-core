// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "bw_ui/adapters/JuceParameterAdapter.h"
#include "bw_ui/kernel/ParameterGestureScope.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace bws::ui::adapters
{

// Pairs a JUCE param adapter with its RAII gesture scope. scope holds an
// IParameter* to &this->adapter, so the struct is non-movable/non-copyable and
// adapter is declared before scope. Wrap in std::unique_ptr for container or
// optional storage. Message-thread only.
struct GestureSession
{
    juce::RangedAudioParameter* juceParamId;
    JuceParameterAdapter adapter;
    kernel::ParameterGestureScope scope;

    explicit GestureSession(juce::RangedAudioParameter* p) noexcept
        : juceParamId(p)
        , adapter(p)
        , scope(&adapter)
    {}

    GestureSession(const GestureSession&) = delete;
    GestureSession& operator=(const GestureSession&) = delete;
    GestureSession(GestureSession&&) = delete;
    GestureSession& operator=(GestureSession&&) = delete;
};

} // namespace bws::ui::adapters

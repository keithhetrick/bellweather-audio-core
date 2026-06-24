// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "bw_ui/kernel/IParameter.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace bws::ui::adapters
{

// ctor must remain trivial - callers construct ephemeral adapters per call
// and rely on equivalent adapters over the same juce param being safe.
class JuceParameterAdapter : public kernel::IParameter
{
public:
    explicit JuceParameterAdapter(juce::RangedAudioParameter* p) noexcept
        : p_(p)
    {}

    void beginGesture() override
    {
        JUCE_ASSERT_MESSAGE_THREAD;
        if (p_ != nullptr)
        {
            p_->beginChangeGesture();
        }
    }

    void setValueNormalized(float v) override
    {
        JUCE_ASSERT_MESSAGE_THREAD;
        if (p_ != nullptr)
        {
            p_->setValueNotifyingHost(v);
        }
    }

    void endGesture() override
    {
        JUCE_ASSERT_MESSAGE_THREAD;
        if (p_ != nullptr)
        {
            p_->endChangeGesture();
        }
    }

    float getDefaultValueNormalized() const override { return p_ != nullptr ? p_->getDefaultValue() : 0.0f; }

private:
    juce::RangedAudioParameter* p_ = nullptr;
};

} // namespace bws::ui::adapters

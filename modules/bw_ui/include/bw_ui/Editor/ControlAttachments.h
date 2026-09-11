// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "bw_ui/Components/TooltipHub.h"
#include "bw_ui/Components/UiKnob.h"
#include "bw_ui/Components/UiToggle.h"

namespace bws::ui::editor
{

// Screen readers want the full name; hosts truncate, we do not.
inline constexpr int kAccessibleNameLimit = 512;

// Bridges a UiKnob's slider with a RangedAudioParameter/APVTS entry and
// publishes the parameter's name to the screen-reader layer. The parameter
// is the single source of truth for what the control announces: title from
// the parameter name, description from the parameter's unit label. When a
// TooltipHub is supplied, the knob registers under the parameter id for the
// binding's lifetime so the hub can resolve its hover copy.
class UiKnobAttachment
{
public:
    UiKnobAttachment(juce::RangedAudioParameter& parameter, UiKnob& knob, TooltipHub* tooltipHubIn = nullptr)
        : attachment(parameter, knob.getSlider(), nullptr)
        , uiKnob(knob)
        , tooltipHub(tooltipHubIn)
    {
        auto& slider = knob.getSlider();
        slider.setTitle(parameter.getName(kAccessibleNameLimit));
        slider.setDescription(parameter.getLabel());
        if (tooltipHub != nullptr)
            tooltipHub->registerControl(uiKnob, parameter.getParameterID());
    }

    UiKnobAttachment(juce::AudioProcessorValueTreeState& state, const juce::String& paramId, UiKnob& knob,
                     TooltipHub* tooltipHubIn = nullptr)
        : UiKnobAttachment(*state.getParameter(paramId), knob, tooltipHubIn)
    {}

    ~UiKnobAttachment()
    {
        if (tooltipHub != nullptr)
            tooltipHub->unregisterControl(uiKnob);
    }

private:
    juce::SliderParameterAttachment attachment;
    UiKnob& uiKnob;
    TooltipHub* tooltipHub;
};

// Minimal attachment to bridge UiToggle with a RangedAudioParameter/APVTS
// entry. The toggle paints itself and receives focus directly, so the
// parameter's name lands on the UiToggle as its accessible title.
class UiToggleAttachment
{
public:
    UiToggleAttachment(juce::RangedAudioParameter& parameter, UiToggle& toggle, TooltipHub* tooltipHubIn = nullptr)
        : attachment(
              parameter, [this](float newValue) { parameterChanged(newValue); }, nullptr)
        , uiToggle(toggle)
        , tooltipHub(tooltipHubIn)
    {
        uiToggle.setTitle(parameter.getName(kAccessibleNameLimit));
        uiToggle.setDescription(parameter.getLabel());
        uiToggle.setOnChange([this](bool newValue) { toggleChanged(newValue); });
        if (tooltipHub != nullptr)
            tooltipHub->registerControl(uiToggle, parameter.getParameterID());
        attachment.sendInitialUpdate();
    }

    UiToggleAttachment(juce::AudioProcessorValueTreeState& state, const juce::String& paramId, UiToggle& toggle,
                       TooltipHub* tooltipHubIn = nullptr)
        : UiToggleAttachment(*state.getParameter(paramId), toggle, tooltipHubIn)
    {}

    ~UiToggleAttachment()
    {
        if (tooltipHub != nullptr)
            tooltipHub->unregisterControl(uiToggle);
        uiToggle.setOnChange(nullptr);
    }

    void sendInitialUpdate()
    {
        if (!juce::MessageManager::existsAndIsCurrentThread())
            return;
        attachment.sendInitialUpdate();
    }

private:
    void parameterChanged(float newValue)
    {
        // ParameterAttachment owns off-thread coalescing and invokes this
        // callback on the message thread. Do not enqueue a second object
        // callback here: it would outlive ParameterAttachment's cancellation.
        jassert(juce::MessageManager::existsAndIsCurrentThread());
        applyToToggle(newValue >= 0.5f);
    }

    void toggleChanged(bool newValue) { attachment.setValueAsCompleteGesture(newValue ? 1.0f : 0.0f); }

    void applyToToggle(bool value)
    {
        // UiToggle::setValue is programmatic and does not invoke onChange, so
        // parameter delivery cannot feed back into the parameter attachment.
        uiToggle.setValue(value);
    }

    juce::ParameterAttachment attachment;
    UiToggle& uiToggle;
    TooltipHub* tooltipHub;
};

} // namespace bws::ui::editor

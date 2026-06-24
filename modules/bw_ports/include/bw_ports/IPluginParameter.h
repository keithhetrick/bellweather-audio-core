// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <string>

namespace bws::domain
{

/**
 * Driven port: a single ranged audio plugin parameter.
 *
 * Replaces juce::RangedAudioParameter* in framework-agnostic code paths.
 * The JUCE adapter (JucePluginParameter in bw_juce_adapters) wraps a
 * juce::RangedAudioParameter*. Another host adapter would implement this
 * same port.
 *
 * Contract:
 * - getId(): stable string identifier (matches the host-visible parameter ID).
 * - getName(): human-readable label.
 * - getValue(): normalized 0..1.
 * - getDenormalized(): value in the parameter's natural range.
 * - getDefaultValue(): normalized 0..1.
 * - getMinValue() / getMaxValue(): natural-range bounds.
 * - setValue(normalized): host-notifying. Listeners fire as if the user moved the control.
 *
 * All methods may be called from any thread that the underlying plugin
 * format permits parameter access on (typically message thread for setValue).
 */
struct IPluginParameter
{
    virtual ~IPluginParameter() = default;

    virtual std::string getId() const = 0;
    virtual std::string getName() const = 0;
    virtual float getValue() const = 0;
    virtual float getDenormalized() const = 0;
    virtual float getDefaultValue() const = 0;
    virtual float getMinValue() const = 0;
    virtual float getMaxValue() const = 0;

    virtual void setValue(float normalizedValue) = 0;
};

} // namespace bws::domain

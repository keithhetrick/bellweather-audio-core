// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <juce_core/juce_core.h>

namespace bws::ui::localization
{

class LocalizedText
{
public:
    enum class Kind
    {
        Literal,
        Key
    };

    static LocalizedText literal(juce::String text) { return LocalizedText(Kind::Literal, std::move(text)); }
    static LocalizedText key(juce::String keyName) { return LocalizedText(Kind::Key, std::move(keyName)); }

    LocalizedText() = default;
    LocalizedText(const char* text)
        : kind_(Kind::Literal)
        , value_(text)
    {}
    LocalizedText(juce::String text)
        : kind_(Kind::Literal)
        , value_(std::move(text))
    {}

    Kind kind() const noexcept { return kind_; }
    bool isKey() const noexcept { return kind_ == Kind::Key; }
    bool isLiteral() const noexcept { return kind_ == Kind::Literal; }
    const juce::String& value() const noexcept { return value_; }

private:
    LocalizedText(Kind kind, juce::String value)
        : kind_(kind)
        , value_(std::move(value))
    {}

    Kind kind_ {Kind::Literal};
    juce::String value_;
};

} // namespace bws::ui::localization

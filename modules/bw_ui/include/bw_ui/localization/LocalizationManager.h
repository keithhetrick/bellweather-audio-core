// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <bw_ui/localization/LocalizedText.h>

#include <juce_events/juce_events.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace bws::ui::localization
{

using PlaceholderMap = std::unordered_map<std::string, juce::String>;

class LocalizationManager
{
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void localizationLanguageChanged() = 0;
    };

    LocalizationManager() = default;

    bool addCatalogFromJson(const char* data, size_t dataSize);
    bool addCatalogFromJson(const juce::String& jsonText);

    void setLanguage(juce::String languageCode);
    const juce::String& currentLanguage() const noexcept { return currentLanguage_; }

    juce::String get(juce::StringRef key) const;
    juce::String format(juce::StringRef key, const PlaceholderMap& placeholders) const;
    juce::String resolve(const LocalizedText& text) const;

    bool hasKey(juce::StringRef key) const;

    void addListener(Listener& listener);
    void removeListener(Listener& listener);

private:
    struct Catalog
    {
        juce::String owner;
        juce::String locale;
        std::unordered_map<std::string, juce::String> strings;
    };

    void assertMessageThread() const;
    const juce::String* findValue(juce::StringRef key, juce::StringRef locale) const;

    juce::String currentLanguage_ {"en-US"};
    std::vector<Catalog> catalogs_;
    juce::ListenerList<Listener> listeners_;
};

void addBwUiEmbeddedCatalogs(LocalizationManager& manager);

} // namespace bws::ui::localization

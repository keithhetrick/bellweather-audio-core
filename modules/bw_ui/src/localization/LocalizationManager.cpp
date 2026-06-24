// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#include "bw_ui/localization/LocalizationManager.h"

#include "LocalizationBinaryData.h"

#include <juce_core/juce_core.h>

namespace bws::ui::localization
{
namespace
{
juce::String propertyString(juce::DynamicObject* object, const juce::Identifier& name)
{
    if (object == nullptr)
        return {};
    return object->getProperty(name).toString();
}

juce::String substitutePlaceholders(juce::String text, const PlaceholderMap& placeholders)
{
    for (const auto& [name, value] : placeholders)
        text = text.replace("{" + juce::String(name) + "}", value);
    return text;
}
} // namespace

bool LocalizationManager::addCatalogFromJson(const char* data, size_t dataSize)
{
    if (data == nullptr || dataSize == 0)
        return false;

    return addCatalogFromJson(juce::String::fromUTF8(data, static_cast<int>(dataSize)));
}

bool LocalizationManager::addCatalogFromJson(const juce::String& jsonText)
{
    assertMessageThread();

    auto parsed = juce::JSON::parse(jsonText);
    auto* root = parsed.getDynamicObject();
    if (root == nullptr)
        return false;

    auto* meta = root->getProperty("_meta").getDynamicObject();
    auto* strings = root->getProperty("strings").getDynamicObject();
    if (meta == nullptr || strings == nullptr)
        return false;

    Catalog catalog;
    catalog.owner = propertyString(meta, "owner");
    catalog.locale = propertyString(meta, "locale");
    if (catalog.owner.isEmpty() || catalog.locale.isEmpty())
        return false;

    const auto& props = strings->getProperties();
    for (int index = 0; index < props.size(); ++index)
    {
        const auto key = props.getName(index).toString();
        auto* record = props.getValueAt(index).getDynamicObject();
        if (record == nullptr)
            continue;

        const auto value = propertyString(record, "value");
        if (key.isNotEmpty() && value.isNotEmpty())
            catalog.strings.emplace(key.toStdString(), value);
    }

    catalogs_.push_back(std::move(catalog));
    return true;
}

void LocalizationManager::setLanguage(juce::String languageCode)
{
    assertMessageThread();

    if (languageCode.isEmpty())
        languageCode = "en-US";

    if (currentLanguage_ == languageCode)
        return;

    currentLanguage_ = std::move(languageCode);
    listeners_.call([](Listener& listener) { listener.localizationLanguageChanged(); });
}

juce::String LocalizationManager::get(juce::StringRef key) const
{
    assertMessageThread();

    if (const auto* value = findValue(key, currentLanguage_))
        return *value;
    if (currentLanguage_ != "en-US")
        if (const auto* value = findValue(key, "en-US"))
            return *value;
    return key.text;
}

juce::String LocalizationManager::format(juce::StringRef key, const PlaceholderMap& placeholders) const
{
    assertMessageThread();
    return substitutePlaceholders(get(key), placeholders);
}

juce::String LocalizationManager::resolve(const LocalizedText& text) const
{
    return text.isKey() ? get(text.value()) : text.value();
}

bool LocalizationManager::hasKey(juce::StringRef key) const
{
    assertMessageThread();
    return findValue(key, currentLanguage_) != nullptr || findValue(key, "en-US") != nullptr;
}

void LocalizationManager::addListener(Listener& listener)
{
    assertMessageThread();
    listeners_.add(&listener);
}

void LocalizationManager::removeListener(Listener& listener)
{
    assertMessageThread();
    listeners_.remove(&listener);
}

void LocalizationManager::assertMessageThread() const
{
    if (juce::MessageManager::getInstanceWithoutCreating() != nullptr)
        jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
}

const juce::String* LocalizationManager::findValue(juce::StringRef key, juce::StringRef locale) const
{
    const auto keyText = juce::String(key).toStdString();
    for (const auto& catalog : catalogs_)
    {
        if (catalog.locale != locale)
            continue;

        const auto found = catalog.strings.find(keyText);
        if (found != catalog.strings.end())
            return &found->second;
    }
    return nullptr;
}

void addBwUiEmbeddedCatalogs(LocalizationManager& manager)
{
    manager.addCatalogFromJson(reinterpret_cast<const char*>(LocalizationBinaryData::enUS_json),
                               static_cast<size_t>(LocalizationBinaryData::enUS_jsonSize));
    manager.addCatalogFromJson(reinterpret_cast<const char*>(LocalizationBinaryData::deDE_json),
                               static_cast<size_t>(LocalizationBinaryData::deDE_jsonSize));
    manager.addCatalogFromJson(reinterpret_cast<const char*>(LocalizationBinaryData::esES_json),
                               static_cast<size_t>(LocalizationBinaryData::esES_jsonSize));
    manager.addCatalogFromJson(reinterpret_cast<const char*>(LocalizationBinaryData::frFR_json),
                               static_cast<size_t>(LocalizationBinaryData::frFR_jsonSize));
    manager.addCatalogFromJson(reinterpret_cast<const char*>(LocalizationBinaryData::jaJP_json),
                               static_cast<size_t>(LocalizationBinaryData::jaJP_jsonSize));
    manager.addCatalogFromJson(reinterpret_cast<const char*>(LocalizationBinaryData::koKR_json),
                               static_cast<size_t>(LocalizationBinaryData::koKR_jsonSize));
    manager.addCatalogFromJson(reinterpret_cast<const char*>(LocalizationBinaryData::ptBR_json),
                               static_cast<size_t>(LocalizationBinaryData::ptBR_jsonSize));
    manager.addCatalogFromJson(reinterpret_cast<const char*>(LocalizationBinaryData::qpsploc_json),
                               static_cast<size_t>(LocalizationBinaryData::qpsploc_jsonSize));
    manager.addCatalogFromJson(reinterpret_cast<const char*>(LocalizationBinaryData::zhCN_json),
                               static_cast<size_t>(LocalizationBinaryData::zhCN_jsonSize));
}

} // namespace bws::ui::localization

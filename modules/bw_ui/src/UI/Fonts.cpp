// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/foundation/Fonts.h"
#include <unordered_map>
#include <mutex>

namespace bw::Fonts
{
static Config gCfg {};
static FontFamily gFamily {FontFamily::Inter};

static juce::Array<juce::Typeface::Ptr>& typefaceSlots()
{
    static auto* slots = new juce::Array<juce::Typeface::Ptr>(); // never destroyed
    if (slots->size() < (int)bw::FontId::Count)
        slots->resize((int)bw::FontId::Count);
    return *slots;
}

static bool& inited()
{
    static auto* flag = new bool(false); // never destroyed
    return *flag;
}

static std::unordered_map<uint64_t, juce::Font>& fontCache()
{
    static auto* cache = new std::unordered_map<uint64_t, juce::Font>(); // never destroyed
    return *cache;
}

static std::mutex& fontsMutex()
{
    static auto* m = new std::mutex(); // never destroyed to avoid shutdown mutex issues
    return *m;
}

static void clearCaches()
{
    std::lock_guard<std::mutex> lg(fontsMutex());
    fontCache().clear();
    typefaceSlots().clear();
}

namespace
{
static_assert(static_cast<int>(bw::FontId::Count) <= 256, "FontId must fit makeKey's 8-bit id field");

uint64_t makeKey(bw::FontId id, float pt, float scale, int style)
{
    const uint64_t idPart = static_cast<uint64_t>(id) & 0xFF;
    const uint64_t ptPart = static_cast<uint64_t>(juce::roundToInt(pt * 100.f)) & 0xFFFF;
    const uint64_t scPart = static_cast<uint64_t>(juce::roundToInt(scale * 1000.f)) & 0xFFFF;
    const uint64_t stylePart = static_cast<uint64_t>(style) & 0xFFFF;
    return (idPart << 48) | (ptPart << 32) | (scPart << 16) | stylePart;
}

float currentScale()
{
    if (auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        return display->scale * juce::Desktop::getInstance().getGlobalScaleFactor();
    return juce::Desktop::getInstance().getGlobalScaleFactor();
}
} // namespace

static juce::Typeface::Ptr loadTypeface(bw::FontId id)
{
    auto idx = static_cast<size_t>(id);
    auto& slots = typefaceSlots();
    auto& slot = slots.getReference((int)idx);
    if (slot != nullptr)
        return slot;

    auto blob = bw::getFontBlob(id);
    if (blob.data != nullptr && blob.size > 0)
        slot = juce::Typeface::createSystemTypefaceFor(blob.data, blob.size);

    return slot;
}

static juce::Font fallbackSans(float pt)
{
    return juce::Font(juce::FontOptions(pt).withName(juce::Font::getDefaultSansSerifFontName()));
}

void setConfig(const Config& cfg)
{
    gCfg = cfg;
    clearCaches();
}

void setFontFamily(FontFamily family)
{
    gFamily = family;
    Config cfg = gCfg;
    switch (family)
    {
    case FontFamily::Inter:
        cfg.body = FontId::UiInterRegular;
        cfg.caption = FontId::UiInterRegular;
        cfg.title = FontId::UiInterSemibold;
        cfg.mono = FontId::MonoRegular;
        break;
    case FontFamily::JetBrainsMono:
    default:
        cfg.body = FontId::UiRegular;
        cfg.caption = FontId::UiRegular;
        cfg.title = FontId::UiSemibold;
        cfg.mono = FontId::MonoRegular;
        break;
    }
    setConfig(cfg);
}

void init()
{
    if (inited())
        return;
    inited() = true;

    // Apply family defaults before loading typefaces
    if (gFamily == FontFamily::JetBrainsMono)
        setFontFamily(FontFamily::JetBrainsMono);
    else
        setFontFamily(FontFamily::Inter);

    (void)loadTypeface(gCfg.body);
    (void)loadTypeface(gCfg.caption);
    (void)loadTypeface(gCfg.title);
    (void)loadTypeface(gCfg.mono);
}

static juce::Font fontFromId(bw::FontId id, float pt, int styleFlags = juce::Font::plain)
{
    if (!inited())
        init();

    // Cache key uses 1.0 for scale - JUCE handles DPI scaling internally.
    // Component-level scaling (ResizableEditor, etc.) should pre-multiply pt.
    const auto key = makeKey(id, pt, 1.0f, styleFlags);

    {
        std::lock_guard<std::mutex> lg(fontsMutex());
        if (auto it = fontCache().find(key); it != fontCache().end())
            return it->second;
    }

    juce::Font font = fallbackSans(pt).withStyle(styleFlags);
    if (auto tf = loadTypeface(id))
    {
        font = juce::Font(juce::FontOptions(pt).withTypeface(tf));
        font.setStyleFlags(styleFlags);
    }

    {
        std::lock_guard<std::mutex> lg(fontsMutex());
        fontCache().emplace(key, font);
    }
    return font;
}

juce::Font body(float pt)
{
    return fontFromId(gCfg.body, pt, juce::Font::plain);
}
juce::Font caption(float pt)
{
    return fontFromId(gCfg.caption, pt, juce::Font::plain);
}
juce::Font title(float pt)
{
    return fontFromId(gCfg.title, pt, juce::Font::plain);
}
juce::Font mono(float pt)
{
    return fontFromId(gCfg.mono, pt, juce::Font::plain);
}
} // namespace bw::Fonts

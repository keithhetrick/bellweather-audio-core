// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/internal/ScaledImageCache.h"

namespace bws::ui
{

ScaledImageCache& ScaledImageCache::getInstance()
{
    static ScaledImageCache instance;
    return instance;
}

juce::String ScaledImageCache::makeCompositeKey(const juce::String& key, int w, int h, float scale) const
{
    return key + "|" + juce::String(w) + "x" + juce::String(h) + "|s=" + juce::String(scale, 3);
}

juce::Image ScaledImageCache::getOrCreate(const juce::String& key, const juce::Image& source, int targetWidth,
                                          int targetHeight, float scale, juce::Graphics::ResamplingQuality quality)
{
    if (source.isNull() || targetWidth <= 0 || targetHeight <= 0)
        return source;

    const auto composite = makeCompositeKey(key, targetWidth, targetHeight, scale);
    if (auto it = cache.find(composite); it != cache.end())
    {
        return it->second;
    }

    auto scaled = source.rescaled(targetWidth, targetHeight, quality);
    cache[composite] = scaled;
    order.push_back(composite);
    if (order.size() > kMaxEntries)
    {
        auto evict = order.front();
        order.pop_front();
        cache.erase(evict);
    }
    return scaled;
}

void ScaledImageCache::clear()
{
    cache.clear();
    order.clear();
}

} // namespace bws::ui

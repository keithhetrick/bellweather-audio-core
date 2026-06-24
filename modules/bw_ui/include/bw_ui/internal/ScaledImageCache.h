// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_graphics/juce_graphics.h>
#include <unordered_map>
#include <deque>

namespace bws::ui
{

class ScaledImageCache
{
public:
    static ScaledImageCache& getInstance();

    juce::Image getOrCreate(const juce::String& key, const juce::Image& source, int targetWidth, int targetHeight,
                            float scale,
                            juce::Graphics::ResamplingQuality quality = juce::Graphics::highResamplingQuality);

    void clear();

private:
    ScaledImageCache() = default;

    struct Entry
    {
        juce::Image image;
        juce::String key;
    };

    juce::String makeCompositeKey(const juce::String& key, int w, int h, float scale) const;

    static constexpr size_t kMaxEntries = 128;
    std::unordered_map<juce::String, juce::Image> cache;
    std::deque<juce::String> order;
};

} // namespace bws::ui

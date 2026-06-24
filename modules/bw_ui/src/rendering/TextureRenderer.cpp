// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/rendering/TextureRenderer.h"
#include <random>

namespace bws::ui::rendering
{

TextureRenderer::TextureRenderer() = default;

void TextureRenderer::renderTexture(juce::Graphics& g, const juce::Rectangle<float>& bounds, tokens::TextureType type,
                                    float strength, float cornerRadius)
{
    if (type == tokens::TextureType::None || strength <= 0.0f)
    {
        return;
    }

    auto intBounds = bounds.toNearestInt();
    if (intBounds.isEmpty())
    {
        return;
    }

    // Get cached texture
    auto& texture = getCachedTexture(type, intBounds.getWidth(), intBounds.getHeight());

    // Save graphics state
    juce::Graphics::ScopedSaveState saveState(g);

    // Apply with opacity
    g.setOpacity(strength);

    // Clip to corner radius if needed
    if (cornerRadius > 0.0f)
    {
        juce::Path clipPath;
        clipPath.addRoundedRectangle(bounds, cornerRadius);
        g.reduceClipRegion(clipPath);
    }

    g.drawImageAt(texture, intBounds.getX(), intBounds.getY());
}

void TextureRenderer::clearCache()
{
    fineGrainCache = juce::Image();
    metalGrainCache = juce::Image();
    scratchedCache = juce::Image();
    weatheredCache = juce::Image();
}

juce::Image TextureRenderer::generateFineGrain(int width, int height)
{
    juce::Image img(juce::Image::ARGB, width, height, true);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 30);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            uint8_t noise = static_cast<uint8_t>(dis(gen));
            img.setPixelAt(x, y, juce::Colour(noise, noise, noise));
        }
    }

    return img;
}

juce::Image TextureRenderer::generateMetalGrain(int width, int height)
{
    juce::Image img(juce::Image::ARGB, width, height, true);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 40);

    // Horizontal grain lines
    for (int y = 0; y < height; ++y)
    {
        uint8_t lineNoise = static_cast<uint8_t>(dis(gen));
        for (int x = 0; x < width; ++x)
        {
            uint8_t pixelNoise = static_cast<uint8_t>((lineNoise + dis(gen)) / 2);
            img.setPixelAt(x, y, juce::Colour(pixelNoise, pixelNoise, pixelNoise));
        }
    }

    return img;
}

juce::Image TextureRenderer::generateScratches(int width, int height)
{
    juce::Image img(juce::Image::ARGB, width, height, true);
    img.clear(img.getBounds(), juce::Colours::transparentBlack);

    juce::Graphics g(img);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> posX(0, width);
    std::uniform_int_distribution<> posY(0, height);
    std::uniform_int_distribution<> len(10, 50);

    // Draw random scratches
    int numScratches = (width * height) / 5000;
    for (int i = 0; i < numScratches; ++i)
    {
        int x1 = posX(gen);
        int y1 = posY(gen);
        int length = len(gen);

        g.setColour(juce::Colours::white.withAlpha(bws::tokens::shared::opacity::texture::SCRATCH));
        g.drawLine(static_cast<float>(x1), static_cast<float>(y1), static_cast<float>(x1 + length),
                   static_cast<float>(y1), 0.5f);
    }

    return img;
}

juce::Image TextureRenderer::generateWeathering(int width, int height)
{
    juce::Image img(juce::Image::ARGB, width, height, true);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 50);

    // More aggressive noise for weathering
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            uint8_t noise = static_cast<uint8_t>(dis(gen));
            // Darken effect
            img.setPixelAt(x, y, juce::Colour(noise / 2, noise / 2, noise / 2));
        }
    }

    return img;
}

juce::Image& TextureRenderer::getCachedTexture(tokens::TextureType type, int width, int height)
{
    switch (type)
    {
    case tokens::TextureType::FineGrain:
        if (!fineGrainCache.isValid() || fineGrainCache.getWidth() != width || fineGrainCache.getHeight() != height)
        {
            fineGrainCache = generateFineGrain(width, height);
        }
        return fineGrainCache;

    case tokens::TextureType::MetalGrain:
        if (!metalGrainCache.isValid() || metalGrainCache.getWidth() != width || metalGrainCache.getHeight() != height)
        {
            metalGrainCache = generateMetalGrain(width, height);
        }
        return metalGrainCache;

    case tokens::TextureType::Scratched:
        if (!scratchedCache.isValid() || scratchedCache.getWidth() != width || scratchedCache.getHeight() != height)
        {
            scratchedCache = generateScratches(width, height);
        }
        return scratchedCache;

    case tokens::TextureType::Weathered:
        if (!weatheredCache.isValid() || weatheredCache.getWidth() != width || weatheredCache.getHeight() != height)
        {
            weatheredCache = generateWeathering(width, height);
        }
        return weatheredCache;

    default:
        // Return empty image for None
        static juce::Image emptyImage;
        return emptyImage;
    }
}

} // namespace bws::ui::rendering

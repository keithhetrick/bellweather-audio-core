// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/ambient/AmbientEffects.h"
#include <cmath>
#include <random>

namespace bws
{
namespace ui
{
namespace ambient
{

//==============================================================================
// BrassBreathing
//==============================================================================

void BrassBreathing::update(float deltaTimeMs)
{
    if (paused_)
        return;

    float deltaSeconds = deltaTimeMs * 0.001f;
    phase_ += deltaSeconds / periodSeconds_;

    // Wrap phase to 0-1
    while (phase_ >= 1.0f)
        phase_ -= 1.0f;
}

float BrassBreathing::getScaleMultiplier() const
{
    if (paused_)
        return 1.0f;

    // Sine wave: -1 to +1
    float sineWave = std::sin(phase_ * juce::MathConstants<float>::twoPi);

    // Map to scale range: 1.0 +/- amplitude
    return 1.0f + (sineWave * amplitude_);
}

//==============================================================================
// AtmosphericParticles
//==============================================================================

AtmosphericParticles::AtmosphericParticles() {}

void AtmosphericParticles::initialize(juce::Rectangle<int> bounds)
{
    bounds_ = bounds;

    // Calculate number of particles based on density
    float area = static_cast<float>(bounds.getWidth() * bounds.getHeight());
    int numParticles = static_cast<int>((area / 1000.0f) * density_);

    particles_.clear();
    particles_.resize(static_cast<size_t>(numParticles));

    for (auto& p : particles_)
        createParticle(p);
}

void AtmosphericParticles::update(float deltaTimeMs)
{
    if (paused_)
        return;

    float deltaSeconds = deltaTimeMs * 0.001f;

    for (auto& p : particles_)
    {
        // Update position
        p.position += p.velocity * deltaSeconds;

        // Wrap around edges
        wrapParticle(p);

        // Update life fade (fade out near edges)
        float distToEdgeX = std::min(p.position.x, static_cast<float>(bounds_.getWidth()) - p.position.x);
        float distToEdgeY = std::min(p.position.y, static_cast<float>(bounds_.getHeight()) - p.position.y);
        float minDistToEdge = std::min(distToEdgeX, distToEdgeY);
        float fadeDistance = 50.0f; // Fade over 50px

        p.lifeFade = juce::jlimit(0.0f, 1.0f, minDistToEdge / fadeDistance);
    }
}

void AtmosphericParticles::paint(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    if (particles_.empty())
        initialize(bounds);

    for (const auto& p : particles_)
    {
        float finalOpacity = p.opacity * p.lifeFade;
        g.setColour(particleColor_.withAlpha(finalOpacity));

        juce::Rectangle<float> particleRect(p.position.x - p.size * 0.5f, p.position.y - p.size * 0.5f, p.size, p.size);

        g.fillEllipse(particleRect);
    }
}

void AtmosphericParticles::setDensity(float density)
{
    density_ = density;
    if (!bounds_.isEmpty())
        initialize(bounds_);
}

void AtmosphericParticles::createParticle(Particle& p)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> xDist(0.0f, static_cast<float>(bounds_.getWidth()));
    std::uniform_real_distribution<float> yDist(0.0f, static_cast<float>(bounds_.getHeight()));
    std::uniform_real_distribution<float> angleDist(0.0f, juce::MathConstants<float>::twoPi);
    std::uniform_real_distribution<float> speedDist(0.5f, 1.5f);
    std::uniform_real_distribution<float> opacityDist(0.3f, 1.0f);
    std::uniform_real_distribution<float> sizeDist(1.0f, 3.0f);

    p.position = juce::Point<float>(xDist(gen), yDist(gen));

    float angle = angleDist(gen);
    float speed = baseVelocity_ * speedDist(gen);
    p.velocity = juce::Point<float>(std::cos(angle) * speed, std::sin(angle) * speed);

    p.opacity = maxOpacity_ * opacityDist(gen);
    p.size = sizeDist(gen);
    p.lifeFade = 1.0f;
}

void AtmosphericParticles::wrapParticle(Particle& p)
{
    float width = static_cast<float>(bounds_.getWidth());
    float height = static_cast<float>(bounds_.getHeight());

    if (p.position.x < 0.0f)
        p.position.x += width;
    else if (p.position.x > width)
        p.position.x -= width;

    if (p.position.y < 0.0f)
        p.position.y += height;
    else if (p.position.y > height)
        p.position.y -= height;
}

//==============================================================================
// HeatShimmer
//==============================================================================

void HeatShimmer::update(float deltaTimeMs)
{
    if (paused_)
        return;

    float deltaSeconds = deltaTimeMs * 0.001f;
    phase_ += deltaSeconds * frequency_;

    // Wrap phase to 0-1
    while (phase_ >= 1.0f)
        phase_ -= 1.0f;
}

juce::Point<float> HeatShimmer::applyShimmer(juce::Point<float> point) const
{
    if (paused_)
        return point;

    // Use Perlin-like noise for organic distortion
    // Simple approximation using sine waves at different frequencies
    float x = point.x * 0.01f; // Spatial frequency
    float t = phase_ * juce::MathConstants<float>::twoPi;

    float distortionX = std::sin(x * 2.0f + t) * 0.5f + std::sin(x * 3.7f + t * 1.3f) * 0.3f;

    float distortionY = std::cos(x * 1.8f + t * 0.9f) * 0.5f + std::cos(x * 4.2f + t * 1.5f) * 0.3f;

    // Normalize and scale by intensity
    distortionX = distortionX * intensity_;
    distortionY = distortionY * intensity_;

    return point + juce::Point<float>(distortionX, distortionY);
}

} // namespace ambient
} // namespace ui
} // namespace bws

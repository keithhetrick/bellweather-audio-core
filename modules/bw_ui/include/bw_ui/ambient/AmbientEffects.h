// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <juce_graphics/juce_graphics.h>
#include <bw_ui/generated/BwTokens.h>
#include <vector>
#include <cmath>

namespace bws
{
namespace ui
{
namespace ambient
{

//==============================================================================
/**
 * @brief Ultra-subtle brass breathing animation
 *
 * Gentle scale pulse on idle brass elements. So subtle you barely notice
 * consciously, but makes UI feel warm vs static.
 *
 * Usage:
 * @code
 * BrassBreathing breathing;
 *
 * void paint(Graphics& g) {
 *     float scale = breathing.getScaleMultiplier();
 *     // Apply scale to rendering (0.995 - 1.005)
 * }
 *
 * void timerCallback() {
 *     breathing.update(deltaTimeMs);
 *     repaint();
 * }
 *
 * void mouseDown(const MouseEvent&) {
 *     breathing.pause();  // Stop when user interacts
 * }
 * @endcode
 */
class BrassBreathing
{
public:
    BrassBreathing() = default;

    /**
     * Update animation
     * @param deltaTimeMs Time since last update
     */
    void update(float deltaTimeMs);

    /**
     * Get current scale multiplier (0.995 - 1.005 by default)
     */
    float getScaleMultiplier() const;

    /**
     * Pause breathing (on user interaction)
     */
    void pause() { paused_ = true; }

    /**
     * Resume breathing (on idle)
     */
    void resume() { paused_ = false; }

    bool isPaused() const { return paused_; }

    //==========================================================================
    // Tuning Parameters
    //==========================================================================

    /**
     * Breathing period in seconds
     * Default: 3.0s (slow, natural rhythm)
     */
    void setPeriodSeconds(float seconds) { periodSeconds_ = seconds; }

    /**
     * Breathing amplitude (scale variation)
     * Default: 0.005 (0.5% scale change)
     * Range: 0.001 - 0.01 (0.1% - 1.0%)
     */
    void setAmplitude(float amplitude) { amplitude_ = juce::jlimit(0.001f, 0.01f, amplitude); }

private:
    float phase_ = 0.0f;
    float periodSeconds_ = 3.0f;
    float amplitude_ = 0.005f;
    bool paused_ = false;
};

//==============================================================================
/**
 * @brief Atmospheric particle field
 *
 * Slow-moving monochromatic particles, very subtle background effect.
 * Creates sense of depth and motion without distraction.
 *
 * Usage:
 * @code
 * AtmosphericParticles particles;
 * particles.setDensity(10.0f);  // 10 particles per 1000px^2
 *
 * void paint(Graphics& g) {
 *     particles.paint(g, getLocalBounds());
 * }
 *
 * void timerCallback() {
 *     particles.update(deltaTimeMs);
 *     repaint();
 * }
 * @endcode
 */
class AtmosphericParticles
{
public:
    AtmosphericParticles();

    /**
     * Initialize particles for given bounds
     */
    void initialize(juce::Rectangle<int> bounds);

    /**
     * Update particle positions
     */
    void update(float deltaTimeMs);

    /**
     * Paint particles
     */
    void paint(juce::Graphics& g, juce::Rectangle<int> bounds);

    /**
     * Pause animation
     */
    void pause() { paused_ = true; }

    /**
     * Resume animation
     */
    void resume() { paused_ = false; }

    //==========================================================================
    // Tuning Parameters
    //==========================================================================

    /**
     * Particle density (particles per 1000px^2)
     * Default: 8.0
     * Range: 1.0 - 20.0
     */
    void setDensity(float density);

    /**
     * Particle velocity (pixels per second)
     * Default: 15.0 (very slow drift)
     * Range: 5.0 - 50.0
     */
    void setVelocity(float velocity) { baseVelocity_ = velocity; }

    /**
     * Maximum particle opacity
     * Default: 0.08 (8% - very subtle)
     * Range: 0.02 - 0.15
     */
    void setOpacity(float opacity) { maxOpacity_ = juce::jlimit(0.02f, 0.15f, opacity); }

    /**
     * Particle color (monochromatic - use theme color)
     */
    void setColor(juce::Colour color) { particleColor_ = color; }

private:
    struct Particle
    {
        juce::Point<float> position;
        juce::Point<float> velocity;
        float opacity;
        float size;
        float lifeFade; // 0-1, fades out near edges
    };

    std::vector<Particle> particles_;
    juce::Rectangle<int> bounds_;
    juce::Colour particleColor_ = juce::Colour(bws::tokens::shared::brand_text::SECONDARY);

    float density_ = 8.0f;
    float baseVelocity_ = 15.0f;
    float maxOpacity_ = 0.08f;
    bool paused_ = false;

    void createParticle(Particle& p);
    void wrapParticle(Particle& p);
};

//==============================================================================
/**
 * @brief Heat shimmer effect for curves
 *
 * Subtle distortion when idle, like viewing through heat waves.
 * Adds organic quality to static curves.
 *
 * Usage:
 * @code
 * HeatShimmer shimmer;
 *
 * void paintCurve(Graphics& g) {
 *     Path curvePath;
 *     for (float x = 0; x < width; x += 1.0f) {
 *         float y = calculateCurveY(x);
 *         auto point = shimmer.applyShimmer(Point<float>(x, y));
 *         curvePath.lineTo(point);
 *     }
 *     g.strokePath(curvePath, ...);
 * }
 *
 * void timerCallback() {
 *     shimmer.update(deltaTimeMs);
 *     repaint();
 * }
 * @endcode
 */
class HeatShimmer
{
public:
    HeatShimmer() = default;

    /**
     * Update shimmer animation
     */
    void update(float deltaTimeMs);

    /**
     * Apply shimmer to a point
     * @param point Original point position
     * @return Displaced point (max 1px displacement by default)
     */
    juce::Point<float> applyShimmer(juce::Point<float> point) const;

    /**
     * Pause shimmer (on user interaction)
     */
    void pause() { paused_ = true; }

    /**
     * Resume shimmer
     */
    void resume() { paused_ = false; }

    //==========================================================================
    // Tuning Parameters
    //==========================================================================

    /**
     * Maximum displacement in pixels
     * Default: 0.5px (very subtle)
     * Range: 0.1 - 2.0
     */
    void setIntensity(float intensity) { intensity_ = juce::jlimit(0.1f, 2.0f, intensity); }

    /**
     * Shimmer frequency (Hz)
     * Default: 0.5 Hz (slow wave)
     */
    void setFrequency(float frequency) { frequency_ = frequency; }

private:
    float phase_ = 0.0f;
    float intensity_ = 0.5f;
    float frequency_ = 0.5f;
    bool paused_ = false;
};

} // namespace ambient
} // namespace ui
} // namespace bws

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <deque>
#include <vector>
#include <map>

namespace bws
{
namespace ui
{

//==============================================================================
/**
 * @brief Spring-damped value animator with real physics
 *
 * Provides weighted, momentum-based value transitions for smooth, organic control feel.
 * Reusable across all knobs, faders, and animated parameters in any plugin.
 *
 * Usage:
 * @code
 * SpringDampedValue valueAnimator;
 * valueAnimator.setTarget(newValue);
 *
 * // In timer callback (~60fps):
 * valueAnimator.update(deltaTimeMs);
 * float smoothValue = valueAnimator.getCurrentValue();
 * @endcode
 */
class SpringDampedValue
{
public:
    SpringDampedValue() = default;

    /**
     * Set target value - spring will smoothly approach this
     */
    void setTarget(float target);

    /**
     * Set current value directly (no animation)
     */
    void setCurrentValue(float value);

    /**
     * Update physics simulation
     * @param deltaTimeMs Time since last update in milliseconds
     */
    void update(float deltaTimeMs);

    /**
     * Get current animated value
     */
    float getCurrentValue() const { return position_; }

    /**
     * Get current velocity (for debug/visualization)
     */
    float getVelocity() const { return velocity_; }

    /**
     * Check if animation has settled (velocity near zero)
     */
    bool isSettled(float threshold = 0.001f) const;

    //==========================================================================
    // Tuning Parameters
    //==========================================================================

    /**
     * Spring stiffness - how quickly value approaches target
     * Higher = tighter spring, faster response
     * Default: 180.0 (tuned for ~150ms settle time)
     * Range: 50-500
     */
    void setSpringStiffness(float stiffness) { springStiffness_ = stiffness; }

    /**
     * Damping coefficient - how quickly oscillation dies out
     * Higher = less overshoot, more damped
     * Default: 12.0 (critical damping for natural feel)
     * Range: 5-30
     */
    void setDamping(float damping) { damping_ = damping; }

    /**
     * Virtual mass - affects momentum and inertia
     * Higher = slower, heavier feel
     * Default: 1.0
     * Range: 0.5-5.0
     */
    void setMass(float mass) { mass_ = juce::jmax(0.1f, mass); }

private:
    float position_ = 0.0f;
    float velocity_ = 0.0f;
    float target_ = 0.0f;

    float springStiffness_ = 180.0f;
    float damping_ = 12.0f;
    float mass_ = 1.0f;
};

//==============================================================================
/**
 * @brief Inertial mouse drag behavior with momentum
 *
 * Tracks drag history to calculate velocity, adds coasting momentum after release.
 * Makes controls feel weighted and physical rather than directly coupled.
 *
 * Usage:
 * @code
 * InertialDragBehavior dragBehavior;
 *
 * void mouseDown(const MouseEvent& e) {
 *     dragBehavior.startDrag(e.position);
 * }
 *
 * void mouseDrag(const MouseEvent& e) {
 *     dragBehavior.updateDrag(e.position, deltaTimeMs);
 *     float delta = dragBehavior.getCurrentDelta();
 *     // Apply delta to parameter
 * }
 *
 * void mouseUp(const MouseEvent& e) {
 *     dragBehavior.endDrag();
 *     // Start timer to handle coasting
 * }
 *
 * void timerCallback() {
 *     if (dragBehavior.isCoasting()) {
 *         float momentum = dragBehavior.getMomentum();
 *         // Apply momentum to parameter
 *         dragBehavior.updateCoasting(deltaTimeMs);
 *     }
 * }
 * @endcode
 */
class InertialDragBehavior
{
public:
    InertialDragBehavior() = default;

    /**
     * Begin drag tracking
     */
    void startDrag(juce::Point<float> position);

    /**
     * Update drag position and calculate velocity
     * @param position Current mouse position
     * @param deltaTimeMs Time since last update
     */
    void updateDrag(juce::Point<float> position, float deltaTimeMs);

    /**
     * End drag and initiate coasting if velocity is high enough
     */
    void endDrag();

    /**
     * Update coasting momentum (call in timer after drag ends)
     * @param deltaTimeMs Time since last update
     * @return true if still coasting, false if settled
     */
    bool updateCoasting(float deltaTimeMs);

    /**
     * Get current delta for this frame
     */
    juce::Point<float> getCurrentDelta() const { return currentDelta_; }

    /**
     * Get current momentum (for coasting)
     */
    juce::Point<float> getMomentum() const { return momentum_; }

    /**
     * Check if currently coasting (after drag release)
     */
    bool isCoasting() const { return coasting_; }

    /**
     * Get current velocity (pixels/second)
     */
    juce::Point<float> getVelocity() const { return velocity_; }

    //==========================================================================
    // Tuning Parameters
    //==========================================================================

    /**
     * Minimum velocity to trigger coasting (pixels/second)
     * Default: 100.0
     */
    void setCoastThreshold(float threshold) { coastThreshold_ = threshold; }

    /**
     * Friction coefficient for coasting decay
     * Higher = faster deceleration
     * Default: 5.0
     */
    void setFriction(float friction) { friction_ = friction; }

    /**
     * History length for velocity calculation (samples)
     * Default: 5
     */
    void setHistoryLength(int length) { historyLength_ = length; }

private:
    struct DragSample
    {
        juce::Point<float> position;
        juce::int64 timestamp;
    };

    std::deque<DragSample> dragHistory_;
    juce::Point<float> lastPosition_;
    juce::Point<float> currentDelta_;
    juce::Point<float> velocity_;
    juce::Point<float> momentum_;

    bool coasting_ = false;
    int historyLength_ = 5;
    float coastThreshold_ = 100.0f;
    float friction_ = 5.0f;

    void calculateVelocity();
};

//==============================================================================
/**
 * @brief Preset morphing with staggered parameter animation
 *
 * Smoothly interpolates between preset states with slight per-parameter delays
 * for natural, organic preset transitions. Much better than instant snapping.
 *
 * Usage:
 * @code
 * PresetMorpher morpher;
 *
 * // When loading preset:
 * juce::ValueTree sourceState = getCurrentState();
 * juce::ValueTree targetState = presetToLoad;
 * morpher.startMorph(sourceState, targetState, 250.0f); // 250ms morph
 *
 * // In timer callback:
 * if (morpher.isMorphing()) {
 *     morpher.update(deltaTimeMs);
 *     auto currentValues = morpher.getCurrentValues();
 *     // Apply to parameters
 * }
 * @endcode
 */
class PresetMorpher
{
public:
    PresetMorpher() = default;

    /**
     * Start morphing from source to target preset
     * @param sourceState Current parameter state
     * @param targetState Target preset state
     * @param durationMs Total morph duration in milliseconds
     */
    void startMorph(const juce::ValueTree& sourceState, const juce::ValueTree& targetState, float durationMs = 250.0f);

    /**
     * Update morph animation
     * @param deltaTimeMs Time since last update
     */
    void update(float deltaTimeMs);

    /**
     * Check if currently morphing
     */
    bool isMorphing() const { return morphing_; }

    /**
     * Get current interpolated parameter values
     */
    std::map<juce::String, float> getCurrentValues() const;

    /**
     * Stop morphing immediately
     */
    void stop();

    //==========================================================================
    // Tuning Parameters
    //==========================================================================

    /**
     * Stagger delay between parameter animations
     * Creates cascading effect - parameters don't all start simultaneously
     * Default: 20ms
     */
    void setParameterStagger(float staggerMs) { parameterStagger_ = staggerMs; }

    /**
     * Easing curve type
     */
    enum class EasingCurve
    {
        Linear,
        EaseInOut,  // Smooth start and end (default)
        EaseOut,    // Quick start, slow end
        EaseIn,     // Slow start, quick end
        Exponential // Very smooth, natural feeling
    };

    void setEasingCurve(EasingCurve curve) { easingCurve_ = curve; }

private:
    struct ParameterAnimation
    {
        juce::String paramId;
        float startValue = 0.0f;
        float targetValue = 0.0f;
        float currentValue = 0.0f;
        float delayMs = 0.0f;  // Stagger offset
        float progress = 0.0f; // 0-1
        bool started = false;
    };

    std::vector<ParameterAnimation> animations_;
    float totalDurationMs_ = 250.0f;
    float globalProgress_ = 0.0f;
    float parameterStagger_ = 20.0f;
    EasingCurve easingCurve_ = EasingCurve::EaseInOut;
    bool morphing_ = false;

    float applyEasing(float t) const;
};

} // namespace ui
} // namespace bws

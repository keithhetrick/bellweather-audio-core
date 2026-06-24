// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/interactions/PhysicsInteractions.h"
#include <cmath>

namespace bws
{
namespace ui
{

//==============================================================================
// SpringDampedValue
//==============================================================================

void SpringDampedValue::setTarget(float target)
{
    target_ = target;
}

void SpringDampedValue::setCurrentValue(float value)
{
    position_ = value;
    target_ = value;
    velocity_ = 0.0f;
}

void SpringDampedValue::update(float deltaTimeMs)
{
    const float dt = deltaTimeMs * 0.001f; // Convert to seconds

    // Spring force: F = -k * (position - target)
    float displacement = position_ - target_;
    float springForce = -springStiffness_ * displacement;

    // Damping force: F = -c * velocity
    float dampingForce = -damping_ * velocity_;

    // Total force
    float totalForce = springForce + dampingForce;

    // Acceleration: F = ma, so a = F/m
    float acceleration = totalForce / mass_;

    // Integrate velocity: v = v + a*dt
    velocity_ += acceleration * dt;

    // Integrate position: p = p + v*dt
    position_ += velocity_ * dt;
}

bool SpringDampedValue::isSettled(float threshold) const
{
    return std::abs(velocity_) < threshold && std::abs(position_ - target_) < threshold;
}

//==============================================================================
// InertialDragBehavior
//==============================================================================

void InertialDragBehavior::startDrag(juce::Point<float> position)
{
    dragHistory_.clear();
    lastPosition_ = position;
    currentDelta_ = {};
    velocity_ = {};
    coasting_ = false;

    DragSample sample;
    sample.position = position;
    sample.timestamp = juce::Time::currentTimeMillis();
    dragHistory_.push_back(sample);
}

void InertialDragBehavior::updateDrag(juce::Point<float> position, float /*deltaTimeMs*/)
{
    currentDelta_ = position - lastPosition_;
    lastPosition_ = position;

    // Add to history
    DragSample sample;
    sample.position = position;
    sample.timestamp = juce::Time::currentTimeMillis();
    dragHistory_.push_back(sample);

    // Limit history length
    while (dragHistory_.size() > static_cast<size_t>(historyLength_))
        dragHistory_.pop_front();

    calculateVelocity();
}

void InertialDragBehavior::endDrag()
{
    calculateVelocity();

    // Check if velocity is high enough to coast
    float speed = velocity_.getDistanceFromOrigin();
    if (speed > coastThreshold_)
    {
        momentum_ = velocity_;
        coasting_ = true;
    }

    dragHistory_.clear();
}

bool InertialDragBehavior::updateCoasting(float deltaTimeMs)
{
    if (!coasting_)
        return false;

    const float dt = deltaTimeMs * 0.001f; // Convert to seconds

    // Apply friction
    float frictionDecel = friction_ * dt;
    float momentumMagnitude = momentum_.getDistanceFromOrigin();

    if (momentumMagnitude < frictionDecel)
    {
        // Momentum depleted
        momentum_ = {};
        coasting_ = false;
        return false;
    }

    // Reduce momentum by friction
    // Normalize the momentum vector manually (JUCE Point doesn't have normalised())
    juce::Point<float> direction = momentum_ / momentumMagnitude;
    juce::Point<float> frictionVector = direction * frictionDecel;
    momentum_ -= frictionVector;

    // Update current delta for this frame
    currentDelta_ = momentum_ * dt;

    return true;
}

void InertialDragBehavior::calculateVelocity()
{
    if (dragHistory_.size() < 2)
    {
        velocity_ = {};
        return;
    }

    const auto& oldest = dragHistory_.front();
    const auto& newest = dragHistory_.back();

    juce::int64 timeDiff = newest.timestamp - oldest.timestamp;
    if (timeDiff <= 0)
    {
        velocity_ = {};
        return;
    }

    juce::Point<float> positionDiff = newest.position - oldest.position;
    float timeSeconds = timeDiff * 0.001f;

    velocity_ = positionDiff / timeSeconds;
}

//==============================================================================
// PresetMorpher
//==============================================================================

void PresetMorpher::startMorph(const juce::ValueTree& sourceState, const juce::ValueTree& targetState, float durationMs)
{
    animations_.clear();
    totalDurationMs_ = durationMs;
    globalProgress_ = 0.0f;
    morphing_ = true;

    // Build animation list for all parameters
    int paramIndex = 0;
    for (int i = 0; i < targetState.getNumProperties(); ++i)
    {
        juce::Identifier propName = targetState.getPropertyName(i);
        juce::String paramId = propName.toString();

        // Get source and target values
        float sourceValue = sourceState.getProperty(propName, 0.0f);
        float targetValue = targetState.getProperty(propName, 0.0f);

        // Skip if values are identical
        if (std::abs(targetValue - sourceValue) < 0.0001f)
            continue;

        ParameterAnimation anim;
        anim.paramId = paramId;
        anim.startValue = sourceValue;
        anim.targetValue = targetValue;
        anim.currentValue = sourceValue;
        anim.delayMs = paramIndex * parameterStagger_; // Stagger
        anim.progress = 0.0f;
        anim.started = false;

        animations_.push_back(anim);
        paramIndex++;
    }
}

void PresetMorpher::update(float deltaTimeMs)
{
    if (!morphing_)
        return;

    globalProgress_ += deltaTimeMs / totalDurationMs_;

    if (globalProgress_ >= 1.0f)
    {
        // Morph complete
        for (auto& anim : animations_)
        {
            anim.currentValue = anim.targetValue;
            anim.progress = 1.0f;
        }
        morphing_ = false;
        return;
    }

    // Update each parameter animation
    for (auto& anim : animations_)
    {
        float globalTimeMs = globalProgress_ * totalDurationMs_;

        if (!anim.started)
        {
            if (globalTimeMs >= anim.delayMs)
                anim.started = true;
            else
                continue; // Not started yet
        }

        // Calculate parameter-local progress
        float localTimeMs = globalTimeMs - anim.delayMs;
        float paramDurationMs = totalDurationMs_ - anim.delayMs;
        anim.progress = juce::jlimit(0.0f, 1.0f, localTimeMs / paramDurationMs);

        // Apply easing
        float easedProgress = applyEasing(anim.progress);

        // Interpolate value
        anim.currentValue = juce::jmap(easedProgress, anim.startValue, anim.targetValue);
    }
}

std::map<juce::String, float> PresetMorpher::getCurrentValues() const
{
    std::map<juce::String, float> values;
    for (const auto& anim : animations_)
    {
        values[anim.paramId] = anim.currentValue;
    }
    return values;
}

void PresetMorpher::stop()
{
    morphing_ = false;
    animations_.clear();
}

float PresetMorpher::applyEasing(float t) const
{
    switch (easingCurve_)
    {
    case EasingCurve::Linear:
        return t;

    case EasingCurve::EaseInOut:
        return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;

    case EasingCurve::EaseOut:
        return 1.0f - std::pow(1.0f - t, 2.0f);

    case EasingCurve::EaseIn:
        return t * t;

    case EasingCurve::Exponential:
        return t * t * (3.0f - 2.0f * t); // Smoothstep

    default:
        return t;
    }
}

} // namespace ui
} // namespace bws

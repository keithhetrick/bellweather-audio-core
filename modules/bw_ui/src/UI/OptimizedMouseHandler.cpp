// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/internal/OptimizedMouseHandler.h"
#include <cmath>

namespace bws::ui
{

OptimizedMouseHandler::OptimizedMouseHandler() = default;

bool OptimizedMouseHandler::handleMouseDown(const juce::MouseEvent& e)
{
    state.position = e.position;
    state.startPosition = e.position;
    state.dragDistance = 0.0f;
    state.isDragging = false;
    state.modifiers = e.mods.getRawFlags();

    linearDragConfidence = 0.0f;
    circularDragConfidence = 0.0f;
    smoothedVelocity = 0.0f;

    return true;
}

bool OptimizedMouseHandler::handleMouseDrag(const juce::MouseEvent& e, float& deltaValue)
{
    const auto currentPos = e.position;
    const auto delta = currentPos - state.position;

    // Dead-zone filtering - ignore tiny movements
    if (!state.isDragging)
    {
        const auto distanceFromStart = state.startPosition.getDistanceFrom(currentPos);
        if (distanceFromStart < DEAD_ZONE_PIXELS)
        {
            deltaValue = 0.0f;
            return false; // Don't process micro-movements
        }
        state.isDragging = true;
    }

    // Update gesture recognition
    updateGestureRecognition(delta);

    // Calculate delta with velocity smoothing for buttery-smooth interaction
    const float rawVelocity = std::hypot(delta.x, delta.y);
    smoothedVelocity = smoothedVelocity * VELOCITY_SMOOTHING + rawVelocity * (1.0f - VELOCITY_SMOOTHING);

    // Use vertical drag for knobs (like most DAWs)
    deltaValue = -delta.y * (e.mods.isShiftDown() ? 0.1f : 1.0f); // Fine adjustment with Shift

    state.position = currentPos;
    state.dragDistance += std::abs(deltaValue);

    return true;
}

bool OptimizedMouseHandler::handleMouseUp(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    const bool wasDragging = state.isDragging;
    reset();
    return wasDragging;
}

void OptimizedMouseHandler::reset()
{
    state = MouseState {};
    linearDragConfidence = 0.0f;
    circularDragConfidence = 0.0f;
}

void OptimizedMouseHandler::updateGestureRecognition(const juce::Point<float>& delta)
{
    // Simple gesture recognition: linear vs circular motion
    if (lastDragDelta.x != 0.0f || lastDragDelta.y != 0.0f)
    {
        // Calculate angle consistency for linear drag
        const float currentAngle = std::atan2(delta.y, delta.x);
        const float lastAngle = std::atan2(lastDragDelta.y, lastDragDelta.x);
        const float angleDiff = std::abs(currentAngle - lastAngle);

        // Linear if angle is consistent
        linearDragConfidence = linearDragConfidence * 0.9f + (angleDiff < 0.2f ? 0.1f : 0.0f);
    }

    lastDragDelta = delta;
}

// CachedLayoutManager implementation
const std::vector<juce::Rectangle<int>>& CachedLayoutManager::getLayout(
    const juce::Rectangle<int>& currentBounds,
    std::function<void(const juce::Rectangle<int>&, std::vector<juce::Rectangle<int>>&)> calculator)
{
    if (!cache.valid || cache.lastBounds != currentBounds)
    {
        cache.lastBounds = currentBounds;
        cache.componentBounds.clear();
        calculator(currentBounds, cache.componentBounds);
        cache.valid = true;
    }

    return cache.componentBounds;
}

// OpenGLComponent implementation
OpenGLComponent::OpenGLComponent() = default;

OpenGLComponent::~OpenGLComponent()
{
    setOpenGLEnabled(false);
}

void OpenGLComponent::setOpenGLEnabled(bool enabled)
{
    if (enabled && !glContext)
    {
        glContext = std::make_unique<juce::OpenGLContext>();
        glContext->setRenderer(this);
        glContext->attachTo(*this);
        glContext->setContinuousRepainting(false); // Only render when needed
    }
    else if (!enabled && glContext)
    {
        glContext->detach();
        glContext.reset();
    }
}

void OpenGLComponent::newOpenGLContextCreated()
{
    // Setup OpenGL state
}

void OpenGLComponent::renderOpenGL()
{
    if (needsRender.exchange(false))
    {
        renderOpenGLContent();
    }
}

void OpenGLComponent::openGLContextClosing()
{
    // Cleanup OpenGL resources
}

} // namespace bws::ui

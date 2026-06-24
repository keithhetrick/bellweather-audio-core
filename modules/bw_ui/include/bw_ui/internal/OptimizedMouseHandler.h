// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>
#include <atomic>

namespace bws::ui
{

// Optimized mouse handler with gesture recognition and dead-zone filtering
class OptimizedMouseHandler
{
public:
    struct MouseState
    {
        juce::Point<float> position;
        juce::Point<float> startPosition;
        float dragDistance {0.0f};
        bool isDragging {false};
        uint32_t modifiers {0};
    };

    OptimizedMouseHandler();

    // Returns true if gesture should be processed (filters out dead-zone micro-movements)
    bool handleMouseDown(const juce::MouseEvent& e);
    bool handleMouseDrag(const juce::MouseEvent& e, float& deltaValue);
    bool handleMouseUp(const juce::MouseEvent& e);

    // Gesture recognition
    bool isLinearDrag() const { return linearDragConfidence > 0.8f; }
    bool isCircularDrag() const { return circularDragConfidence > 0.8f; }

    void reset();

private:
    MouseState state;

    // Dead-zone filtering (prevents accidental micro-movements)
    static constexpr float DEAD_ZONE_PIXELS = 3.0f;
    static constexpr float VELOCITY_SMOOTHING = 0.7f;

    // Gesture recognition
    float linearDragConfidence {0.0f};
    float circularDragConfidence {0.0f};
    juce::Point<float> lastDragDelta;
    float smoothedVelocity {0.0f};

    void updateGestureRecognition(const juce::Point<float>& delta);
};

// Cached layout manager - only recalculates when bounds actually change
class CachedLayoutManager
{
public:
    struct LayoutCache
    {
        juce::Rectangle<int> lastBounds;
        std::vector<juce::Rectangle<int>> componentBounds;
        bool valid {false};
    };

    CachedLayoutManager() = default;

    // Returns cached layout if bounds unchanged, otherwise recalculates
    const std::vector<juce::Rectangle<int>>& getLayout(
        const juce::Rectangle<int>& currentBounds,
        std::function<void(const juce::Rectangle<int>&, std::vector<juce::Rectangle<int>>&)> calculator);

    void invalidate() { cache.valid = false; }

private:
    LayoutCache cache;
};

// OpenGL-accelerated component base
class OpenGLComponent
    : public juce::Component
    , public juce::OpenGLRenderer
{
public:
    OpenGLComponent();
    ~OpenGLComponent() override;

    // Enable/disable OpenGL acceleration
    void setOpenGLEnabled(bool enabled);
    bool isOpenGLEnabled() const { return glContext != nullptr; }

protected:
    // OpenGLRenderer interface
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    // Override for custom OpenGL rendering
    virtual void renderOpenGLContent() {}

private:
    std::unique_ptr<juce::OpenGLContext> glContext;
    std::atomic<bool> needsRender {false};
};

} // namespace bws::ui

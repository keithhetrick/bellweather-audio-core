// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_ui/rendering/IRenderer.h>
#include <bw_ui/foundation/Fonts.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <unordered_map>
#include <vector>

namespace bws::ui::rendering
{

/**
 * JuceRenderer - IRenderer adapter backed by juce::Graphics.
 *
 * The only file in the 2D rendering layer that imports JUCE.
 * Swap this for VisageRenderer, SkiaRenderer, etc. without
 * touching any plugin paint code.
 */
class JuceRenderer final : public IRenderer
{
public:
    explicit JuceRenderer(juce::Graphics& g)
        : g_(g)
    {}

    ~JuceRenderer() override
    {
        for (auto& [id, grad] : gradients_)
            delete grad;
    }

    // ── Colour + state ──────────────────────────────────────────────────

    void setColour(uint32_t argb) override { g_.setColour(juce::Colour(argb)); }

    void setOpacity(float alpha) override { g_.setOpacity(alpha); }

    void saveState() override { g_.saveState(); }
    void restoreState() override { g_.restoreState(); }

    void clipRect(float x, float y, float w, float h) override
    {
        g_.reduceClipRegion(
            juce::Rectangle<int>(static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), static_cast<int>(h)));
    }

    // ── Shapes ──────────────────────────────────────────────────────────

    void fillRect(float x, float y, float w, float h) override { g_.fillRect(juce::Rectangle<float>(x, y, w, h)); }

    void fillRoundedRect(float x, float y, float w, float h, float r) override
    {
        g_.fillRoundedRectangle(x, y, w, h, r);
    }

    void fillEllipse(float x, float y, float w, float h) override { g_.fillEllipse(x, y, w, h); }

    void drawRect(float x, float y, float w, float h, float t) override
    {
        g_.drawRect(juce::Rectangle<float>(x, y, w, h), t);
    }

    void drawRoundedRect(float x, float y, float w, float h, float r, float t) override
    {
        g_.drawRoundedRectangle(x, y, w, h, r, t);
    }

    void drawEllipse(float x, float y, float w, float h, float t) override { g_.drawEllipse(x, y, w, h, t); }

    void drawLine(float x1, float y1, float x2, float y2, float t) override { g_.drawLine(x1, y1, x2, y2, t); }

    // ── Text ────────────────────────────────────────────────────────────

    void setFont(float size, FontWeight weight) override
    {
        switch (weight)
        {
        case FontWeight::Bold:
            g_.setFont(bw::Fonts::title(size));
            break;
        case FontWeight::Mono:
            g_.setFont(bw::Fonts::mono(size));
            break;
        case FontWeight::Regular:
        default:
            g_.setFont(bw::Fonts::body(size));
            break;
        }
    }

    void drawText(const char* text, float x, float y, float w, float h, Justification j) override
    {
        auto just = j == Justification::Left           ? juce::Justification::centredLeft
                    : j == Justification::Right        ? juce::Justification::centredRight
                    : j == Justification::CentreBottom ? juce::Justification::centredBottom
                                                       : juce::Justification::centred;
        g_.drawText(juce::String::fromUTF8(text), juce::Rectangle<float>(x, y, w, h), just, false);
    }

    // ── Gradients (opaque handle pattern) ───────────────────────────────

    GradientHandle createLinearGradient(float x1, float y1, float x2, float y2) override
    {
        auto* grad = new juce::ColourGradient(juce::Colours::transparentBlack, x1, y1, juce::Colours::transparentBlack,
                                              x2, y2, false);
        grad->clearColours(); // drop placeholder endpoints; addGradientStop defines all stops
        const uint32_t id = nextGradientId_++;
        gradients_[id] = grad;
        return {id};
    }

    GradientHandle createRadialGradient(float cx, float cy, float radius) override
    {
        auto* grad = new juce::ColourGradient(juce::Colours::transparentBlack, cx, cy, juce::Colours::transparentBlack,
                                              cx + radius, cy, true);
        grad->clearColours(); // drop placeholder endpoints; addGradientStop defines all stops
        const uint32_t id = nextGradientId_++;
        gradients_[id] = grad;
        return {id};
    }

    GradientHandle createRadialGradient(float x1, float y1, float x2, float y2) override
    {
        auto* grad = new juce::ColourGradient(juce::Colours::transparentBlack, x1, y1, juce::Colours::transparentBlack,
                                              x2, y2, true);
        grad->clearColours(); // drop placeholder endpoints; addGradientStop defines all stops
        const uint32_t id = nextGradientId_++;
        gradients_[id] = grad;
        return {id};
    }

    void addGradientStop(GradientHandle h, float position, uint32_t argb) override
    {
        auto it = gradients_.find(h.id);
        if (it == gradients_.end())
            return;

        it->second->addColour(static_cast<double>(position), juce::Colour(argb));
    }

    void applyGradient(GradientHandle h) override
    {
        auto it = gradients_.find(h.id);
        if (it != gradients_.end())
            g_.setGradientFill(*it->second);
    }

    void destroyGradient(GradientHandle h) override
    {
        auto it = gradients_.find(h.id);
        if (it != gradients_.end())
        {
            delete it->second;
            gradients_.erase(it);
        }
    }

    // ── Path builder ────────────────────────────────────────────────────

    void beginPath() override { currentPath_.clear(); }

    void moveTo(float x, float y) override { currentPath_.startNewSubPath(x, y); }

    void lineTo(float x, float y) override { currentPath_.lineTo(x, y); }

    void quadraticTo(float cx, float cy, float x, float y) override { currentPath_.quadraticTo(cx, cy, x, y); }

    void cubicTo(float c1x, float c1y, float c2x, float c2y, float x, float y) override
    {
        currentPath_.cubicTo(c1x, c1y, c2x, c2y, x, y);
    }

    void arcTo(float cx, float cy, float radius, float startAngle, float endAngle) override
    {
        currentPath_.addCentredArc(cx, cy, radius, radius, 0.0f, startAngle, endAngle, true);
    }

    void closePath() override { currentPath_.closeSubPath(); }

    void fillPath(WindingRule rule = WindingRule::NonZero) override
    {
        if (rule == WindingRule::EvenOdd)
            currentPath_.setUsingNonZeroWinding(false);
        g_.fillPath(currentPath_);
    }

    void setLineDash(const float* pattern, int count) override { dashPattern_.assign(pattern, pattern + count); }

    void clearLineDash() override { dashPattern_.clear(); }

    void strokePath(float thickness) override
    {
        juce::PathStrokeType stroke(thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        if (!dashPattern_.empty())
        {
            juce::Path dashed;
            stroke.createDashedStroke(dashed, currentPath_, dashPattern_.data(), static_cast<int>(dashPattern_.size()));
            g_.strokePath(dashed, stroke);
        }
        else
        {
            g_.strokePath(currentPath_, stroke);
        }
    }

    void clipPath() override { g_.reduceClipRegion(currentPath_); }

private:
    juce::Graphics& g_;
    juce::Path currentPath_;
    std::unordered_map<uint32_t, juce::ColourGradient*> gradients_;
    uint32_t nextGradientId_ = 1;
    std::vector<float> dashPattern_;
};

} // namespace bws::ui::rendering

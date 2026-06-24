// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/weather/containers/CollapsiblePanel.h"
#include "bw_ui/foundation/UiTheme.h"
#include "bw_ui/generated/BwTokens.h"
#include <cmath>

namespace bws::weather
{
namespace
{
const bws::ui::kernel::ThemeSnapshot& defaultPanelKernelTheme()
{
    static const auto kDefault = bws::ui::resolveTheme(bws::ui::defaultUiThemeBase(), {});
    static const auto kDefaultKernel = bws::ui::adapters::makeKernelThemeSnapshot(kDefault);
    return kDefaultKernel;
}
} // namespace

//==============================================================================
// CONSTRUCTION
//==============================================================================

CollapsiblePanel::CollapsiblePanel(const juce::String& title)
    : title_(title)
{
    setOpaque(false);
}

CollapsiblePanel::~CollapsiblePanel()
{
    stopTimer();
}

//==============================================================================
// CONFIGURATION
//==============================================================================

void CollapsiblePanel::setContent(std::unique_ptr<juce::Component> content)
{
    if (content_)
        removeChildComponent(content_.get());

    content_ = std::move(content);

    if (content_)
    {
        addChildComponent(content_.get());
        content_->setVisible(expanded_ || expansionProgress_ > 0.0f);
    }

    updateLayout();
}

void CollapsiblePanel::setExpanded(bool expanded, bool animate)
{
    if (expanded_ == expanded && !isAnimating_)
        return;

    expanded_ = expanded;

    if (animate)
    {
        startAnimation(expanded);
    }
    else
    {
        // Immediate change
        expansionProgress_ = expanded ? 1.0f : 0.0f;
        targetProgress_ = expansionProgress_;
        isAnimating_ = false;
        stopTimer();

        if (content_)
            content_->setVisible(expanded);

        updateLayout();
        repaint();

        if (onExpandedChanged)
            onExpandedChanged(expanded);
    }
}

void CollapsiblePanel::setHeaderHeight(int height)
{
    headerHeight_ = height;
    updateLayout();
    repaint();
}

void CollapsiblePanel::setShowArrow(bool show)
{
    showArrow_ = show;
    repaint();
}

void CollapsiblePanel::setTitle(const juce::String& title)
{
    title_ = title;
    repaint();
}

void CollapsiblePanel::setAnimationDuration(int durationMs)
{
    animationDurationMs_ = juce::jlimit(50, 1000, durationMs);
}

void CollapsiblePanel::setScale(float scale)
{
    // Fix #4: Update layout when scale changes, not just repaint
    // (Scale affects height calculations which require layout update)
    if (std::abs(scale_ - scale) > 0.001f)
    {
        scale_ = scale;
        updateLayout();
        repaint();
    }
}

//==============================================================================
// SIZE CALCULATION
//==============================================================================

int CollapsiblePanel::getCollapsedHeight() const
{
    return static_cast<int>(headerHeight_ * scale_);
}

int CollapsiblePanel::getExpandedHeight() const
{
    return static_cast<int>((headerHeight_ + contentHeight_) * scale_);
}

int CollapsiblePanel::getCurrentHeight() const
{
    int collapsedH = getCollapsedHeight();
    int expandedH = getExpandedHeight();
    return collapsedH + static_cast<int>((expandedH - collapsedH) * expansionProgress_);
}

void CollapsiblePanel::setContentHeight(int height)
{
    contentHeight_ = height;
    updateLayout();
}

//==============================================================================
// COMPONENT OVERRIDES
//==============================================================================

void CollapsiblePanel::paint(juce::Graphics& g)
{
    // Draw header
    paintHeader(g);

    // Draw content border/background if expanded or animating
    if (expansionProgress_ > 0.001f)
    {
        paintContentBorder(g);
    }
}

void CollapsiblePanel::resized()
{
    updateLayout();
}

void CollapsiblePanel::mouseDown(const juce::MouseEvent& e)
{
    // Check if click is in header area
    if (headerBounds_.contains(e.getPosition()))
    {
        setExpanded(!expanded_, true);
    }
}

void CollapsiblePanel::mouseEnter(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    isHovering_ = true;
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    repaint();
}

void CollapsiblePanel::mouseExit(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    isHovering_ = false;
    setMouseCursor(juce::MouseCursor::NormalCursor);
    repaint();
}

//==============================================================================
// TIMER CALLBACK
//==============================================================================

void CollapsiblePanel::timerCallback()
{
    updateAnimation();
}

//==============================================================================
// DRAWING HELPERS
//==============================================================================

void CollapsiblePanel::paintHeader(juce::Graphics& g)
{
    static const auto kDefault = bws::ui::resolveTheme(bws::ui::defaultUiThemeBase(), {});
    const auto& t = theme_ ? *theme_ : kDefault;
    const auto kernelTheme = theme_ ? bws::ui::adapters::makeKernelThemeSnapshot(t) : defaultPanelKernelTheme();

    auto bounds = headerBounds_.toFloat();
    const float cornerRadius = t.weatherColors.radiusMd * scale_;

    // Header background - brass gradient
    juce::ColourGradient headerGradient(t.weatherColors.bgElevated.brighter(0.05f), bounds.getTopLeft(),
                                        t.weatherColors.bgRaised, bounds.getBottomLeft(), false);
    g.setGradientFill(headerGradient);

    // If expanded, only round top corners
    if (expansionProgress_ > 0.5f)
    {
        juce::Path headerPath;
        headerPath.addRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(),
                                       cornerRadius, cornerRadius, true, true, // top-left, top-right
                                       false, false                            // bottom-left, bottom-right
        );
        g.fillPath(headerPath);
    }
    else
    {
        g.fillRoundedRectangle(bounds, cornerRadius);
    }

    // Hover glow
    if (isHovering_)
    {
        g.setColour(t.weatherColors.glow.withAlpha(bws::tokens::shared::opacity::panel::GLOW_WASH));
        g.fillRoundedRectangle(bounds, cornerRadius);
    }

    // Top brass accent line
    g.setColour(t.weatherColors.accent.withAlpha(bws::tokens::shared::opacity::panel::ACCENT_EDGE));
    g.fillRect(bounds.getX() + cornerRadius, bounds.getY(), bounds.getWidth() - cornerRadius * 2, 1.0f * scale_);

    // Draw arrow indicator
    if (showArrow_)
    {
        float arrowSize = 12.0f * scale_;
        float arrowX = bounds.getX() + 16.0f * scale_;
        float arrowY = bounds.getCentreY() - arrowSize * 0.5f;
        paintArrow(g, {arrowX, arrowY, arrowSize, arrowSize});
    }

    // Draw title
    float textX = showArrow_ ? 36.0f * scale_ : 16.0f * scale_;
    auto textBounds = bounds.withTrimmedLeft(textX).withTrimmedRight(16.0f * scale_);

    g.setColour(t.weatherColors.textPrimary);
    g.setFont(bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::ControlText, scale_));
    g.drawText(title_, textBounds, juce::Justification::centredLeft);

    // Border
    g.setColour(t.weatherColors.borderLight.withAlpha(bws::tokens::shared::opacity::panel::BORDER));
    if (expansionProgress_ > 0.5f)
    {
        juce::Path borderPath;
        borderPath.addRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(),
                                       cornerRadius, cornerRadius, true, true, false, false);
        g.strokePath(borderPath, juce::PathStrokeType(1.0f * scale_));
    }
    else
    {
        g.drawRoundedRectangle(bounds.reduced(bws::tokens::shared::geometry::STROKE_HALF_PX), cornerRadius,
                               1.0f * scale_);
    }
}

void CollapsiblePanel::paintArrow(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    static const auto kDefault = bws::ui::resolveTheme(bws::ui::defaultUiThemeBase(), {});
    const auto& t = theme_ ? *theme_ : kDefault;

    juce::Path arrow;

    // Calculate rotation based on expansion progress
    // 0 = pointing right (collapsed), 90 = pointing down (expanded)
    float rotationDegrees = expansionProgress_ * 90.0f;

    // Create right-pointing triangle
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    float size = bounds.getWidth() * 0.4f;

    // Triangle pointing right (will be rotated)
    arrow.addTriangle(cx - size * 0.5f, cy - size * 0.6f, // top-left
                      cx - size * 0.5f, cy + size * 0.6f, // bottom-left
                      cx + size * 0.5f, cy                // right point
    );

    // Apply rotation around center
    juce::AffineTransform rotation = juce::AffineTransform::rotation(juce::degreesToRadians(rotationDegrees), cx, cy);
    arrow.applyTransform(rotation);

    // Draw with brass color
    g.setColour(t.weatherColors.accent);
    g.fillPath(arrow);
}

void CollapsiblePanel::paintContentBorder(juce::Graphics& g)
{
    if (contentBounds_.isEmpty())
        return;

    static const auto kDefault = bws::ui::resolveTheme(bws::ui::defaultUiThemeBase(), {});
    const auto& t = theme_ ? *theme_ : kDefault;

    auto bounds = contentBounds_.toFloat();
    const float cornerRadius = t.weatherColors.radiusMd * scale_;

    // Content background
    g.setColour(
        t.weatherColors.bgMain.withAlpha(bws::tokens::shared::opacity::panel::BG_EXPANSION_BASE * expansionProgress_));

    juce::Path contentPath;
    contentPath.addRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), cornerRadius,
                                    cornerRadius, false, false, // top corners
                                    true, true                  // bottom corners
    );
    g.fillPath(contentPath);

    // Brass border around content
    g.setColour(t.weatherColors.borderLight.withAlpha(bws::tokens::shared::opacity::panel::BORDER_EXPANSION_BASE *
                                                      expansionProgress_));

    // Left edge
    g.fillRect(bounds.getX(), bounds.getY(), 1.0f * scale_, bounds.getHeight() - cornerRadius);

    // Right edge
    g.fillRect(bounds.getRight() - 1.0f * scale_, bounds.getY(), 1.0f * scale_, bounds.getHeight() - cornerRadius);

    // Bottom rounded rectangle
    juce::Path bottomPath;
    bottomPath.addRoundedRectangle(bounds.getX(), bounds.getBottom() - cornerRadius * 2, bounds.getWidth(),
                                   cornerRadius * 2, cornerRadius, cornerRadius, false, false, true, true);
    g.strokePath(bottomPath, juce::PathStrokeType(1.0f * scale_));
}

//==============================================================================
// ANIMATION
//==============================================================================

void CollapsiblePanel::startAnimation(bool expanding)
{
    targetProgress_ = expanding ? 1.0f : 0.0f;

    // Calculate speed based on duration
    // At 60 FPS, we have (duration / 16.67) frames
    float framesNeeded = animationDurationMs_ / 16.667f;
    animationSpeed_ = 1.0f / framesNeeded;

    isAnimating_ = true;

    // Show content during animation
    if (content_ && expanding)
        content_->setVisible(true);

    startTimerHz(60);
}

void CollapsiblePanel::updateAnimation()
{
    if (!isAnimating_)
    {
        stopTimer();
        return;
    }

    // Move toward target
    float direction = (targetProgress_ > expansionProgress_) ? 1.0f : -1.0f;
    expansionProgress_ += direction * animationSpeed_;

    // Clamp and check completion
    bool completed = false;
    if (direction > 0 && expansionProgress_ >= targetProgress_)
    {
        expansionProgress_ = targetProgress_;
        completed = true;
    }
    else if (direction < 0 && expansionProgress_ <= targetProgress_)
    {
        expansionProgress_ = targetProgress_;
        completed = true;
    }

    // Update layout
    updateLayout();
    repaint();

    // Notify progress
    if (onAnimationProgress)
        onAnimationProgress(expansionProgress_);

    // Handle completion
    if (completed)
    {
        isAnimating_ = false;
        stopTimer();

        // Hide content if collapsed
        if (content_ && !expanded_)
            content_->setVisible(false);

        // Notify completion
        if (onExpandedChanged)
            onExpandedChanged(expanded_);
    }
}

//==============================================================================
// LAYOUT
//==============================================================================

void CollapsiblePanel::updateLayout()
{
    auto bounds = getLocalBounds();
    int scaledHeaderHeight = static_cast<int>(headerHeight_ * scale_);

    // Header always at top
    headerBounds_ = bounds.removeFromTop(scaledHeaderHeight);

    // Content area (may be zero height when collapsed)
    int currentHeight = getCurrentHeight();
    int contentAreaHeight = currentHeight - scaledHeaderHeight;

    if (contentAreaHeight > 0)
    {
        contentBounds_ = bounds.withHeight(contentAreaHeight);

        // Position content component with padding
        if (content_)
        {
            int padding = static_cast<int>(8.0f * scale_);
            auto contentArea = contentBounds_.reduced(padding);

            // Apply alpha based on expansion
            content_->setAlpha(expansionProgress_);
            content_->setBounds(contentArea);
        }
    }
    else
    {
        contentBounds_ = {};
    }
}

} // namespace bws::weather

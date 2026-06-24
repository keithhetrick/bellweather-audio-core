// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/weather/containers/CondensedStrip.h"
#include "bw_ui/adapters/UiThemeKernelAdapter.h"
#include "bw_ui/generated/BwTokens.h"
#include <bw_ui/rendering/JuceRenderer.h>
#include "bw_ui/foundation/UiTheme.h"
#include <cmath>

namespace bws::weather
{
namespace
{
const bws::ui::UiThemeResolved& defaultTheme()
{
    static const auto kDefault = bws::ui::resolveTheme(bws::ui::defaultUiThemeBase(), {});
    return kDefault;
}
} // namespace

//==============================================================================
// CONSTRUCTION
//==============================================================================

CondensedStrip::CondensedStrip()
    : kernelTheme_(bws::ui::adapters::makeKernelThemeSnapshot(defaultTheme()))
{
    setOpaque(false);
}

CondensedStrip::~CondensedStrip()
{
    stopTimer();
}

//==============================================================================
// CONFIGURATION
//==============================================================================

void CondensedStrip::setContent(juce::Component* content)
{
    if (content_ != content)
    {
        if (content_)
            removeChildComponent(content_);

        content_ = content;

        if (content_)
        {
            addChildComponent(content_);
            content_->setVisible(expanded_ || expansionProgress_ > 0.0f);
        }

        updateLayout();
    }
}

void CondensedStrip::addParameterValue(const juce::String& paramId, const juce::String& shortName,
                                       std::function<juce::String()> valueGetter)
{
    parameters_.push_back({paramId, shortName, std::move(valueGetter)});
    repaint();
}

void CondensedStrip::clearParameterValues()
{
    parameters_.clear();
    repaint();
}

void CondensedStrip::setExpanded(bool expanded, bool animate)
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

void CondensedStrip::setCollapsedHeight(int height)
{
    collapsedHeight_ = height;
    updateLayout();
    repaint();
}

void CondensedStrip::setExpandedHeight(int height)
{
    expandedHeight_ = height;
    updateLayout();
    repaint();
}

int CondensedStrip::getCurrentHeight() const
{
    int collapsedH = static_cast<int>(collapsedHeight_ * scale_);
    int expandedH = static_cast<int>(expandedHeight_ * scale_);
    return collapsedH + static_cast<int>((expandedH - collapsedH) * expansionProgress_);
}

void CondensedStrip::setScale(float scale)
{
    if (std::abs(scale_ - scale) > 0.001f)
    {
        scale_ = scale;
        updateLayout();
        repaint();
    }
}

void CondensedStrip::setTheme(const bws::ui::UiThemeResolved& t)
{
    theme_ = &t;
    kernelTheme_ = bws::ui::adapters::makeKernelThemeSnapshot(t);
    repaint();
}

void CondensedStrip::setTitle(const juce::String& title)
{
    title_ = title;
    repaint();
}

void CondensedStrip::setAnimationDuration(int durationMs)
{
    animationDurationMs_ = juce::jlimit(50, 1000, durationMs);
}

//==============================================================================
// COMPONENT OVERRIDES
//==============================================================================

void CondensedStrip::paint(juce::Graphics& g)
{
    if (expanded_ || expansionProgress_ > 0.5f)
    {
        paintExpandedHeader(g);
    }
    else
    {
        paintCollapsedStrip(g);
    }
}

void CondensedStrip::paintCollapsedStrip(juce::Graphics& g)
{
    const auto& t = theme_ ? *theme_ : defaultTheme();

    bws::ui::rendering::JuceRenderer renderer(g);

    auto bounds = getLocalBounds().toFloat();
    const float cornerRadius = t.weatherColors.radiusMd * scale_;

    // Background with brass gradient
    auto bgH = renderer.createLinearGradient(bounds.getX(), bounds.getY(), bounds.getX(), bounds.getBottom());
    renderer.addGradientStop(bgH, 0.0f, t.weatherColors.bgElevated.brighter(0.03f).getARGB());
    renderer.addGradientStop(bgH, 1.0f, t.weatherColors.bgRaised.getARGB());
    renderer.applyGradient(bgH);
    renderer.fillRoundedRect(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), cornerRadius);
    renderer.destroyGradient(bgH);

    // Brass border
    const auto borderBounds = bounds.reduced(bws::tokens::shared::geometry::STROKE_HALF_PX);
    renderer.setColour(
        t.weatherColors.accent.withAlpha(bws::tokens::shared::opacity::strip::ACCENT_GLOW_DIM).getARGB());
    renderer.drawRoundedRect(borderBounds.getX(), borderBounds.getY(), borderBounds.getWidth(),
                             borderBounds.getHeight(), cornerRadius, 1.5f * scale_);

    // Top brass accent line
    renderer.setColour(
        t.weatherColors.accent.withAlpha(bws::tokens::shared::opacity::strip::ACCENT_GLOW_BRIGHT).getARGB());
    renderer.fillRect(bounds.getX() + cornerRadius, bounds.getY(), bounds.getWidth() - cornerRadius * 2, 1.0f * scale_);

    // Hover glow effect
    if (isHovering_)
    {
        renderer.setColour(t.weatherColors.glow.withAlpha(bws::tokens::shared::opacity::strip::GLOW_DIM).getARGB());
        renderer.fillRoundedRect(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), cornerRadius);
    }

    // Title label on left with arrow
    float textX = 12.0f * scale_;
    g.setColour(t.weatherColors.accent);
    g.setFont(bws::ui::adapters::makeFont(kernelTheme_, bws::ui::kernel::TextRole::Readout, scale_));

    // Draw right-pointing arrow
    juce::Path arrow;
    float arrowX = textX;
    float arrowY = bounds.getCentreY();
    float arrowSize = 5.0f * scale_;
    arrow.addTriangle(arrowX, arrowY - arrowSize, arrowX, arrowY + arrowSize, arrowX + arrowSize * 1.2f, arrowY);
    g.fillPath(arrow);

    // Title text
    textX += arrowSize * 2.0f + 8.0f * scale_;
    float titleZoneW = juce::jlimit(80.0f * scale_, 160.0f * scale_, bounds.getWidth() * 0.18f);
    g.drawText(title_, juce::Rectangle<float>(textX, 0, titleZoneW, bounds.getHeight()),
               juce::Justification::centredLeft);

    // Parameter values across strip
    g.setColour(t.weatherColors.textPrimary);
    g.setFont(bws::ui::adapters::makeFont(kernelTheme_, bws::ui::kernel::TextRole::Readout, scale_));

    float x = textX + titleZoneW;
    float maxX = bounds.getRight() - 24.0f * scale_;

    for (const auto& param : parameters_)
    {
        if (x >= maxX)
            break;

        juce::String display = param.shortName + ":" + param.valueGetter();

        // Use GlyphArrangement for text width calculation (avoids deprecation warning)
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(g.getCurrentFont(), display, 0.0f, 0.0f);
        float textWidth = glyphs.getBoundingBox(0, -1, true).getWidth();

        // Alternate colors for visual separation
        g.setColour(t.weatherColors.textSecondary);
        g.drawText(param.shortName + ":", x, 0, textWidth, bounds.getHeight(), juce::Justification::centredLeft);

        juce::GlyphArrangement labelGlyphs;
        labelGlyphs.addLineOfText(g.getCurrentFont(), param.shortName + ":", 0.0f, 0.0f);
        float labelWidth = labelGlyphs.getBoundingBox(0, -1, true).getWidth();

        g.setColour(t.weatherColors.textPrimary);
        g.drawText(param.valueGetter(), x + labelWidth, 0, textWidth - labelWidth + 5.0f * scale_, bounds.getHeight(),
                   juce::Justification::centredLeft);

        x += textWidth + 12.0f * scale_;
    }

    // Click hint arrow on right
    g.setColour(t.weatherColors.textSecondary.withAlpha(bws::tokens::shared::opacity::strip::SECONDARY_TEXT));
    float hintArrowX = bounds.getRight() - 20.0f * scale_;
    float hintArrowY = bounds.getCentreY();
    float hintSize = 4.0f * scale_;
    juce::Path hintArrow;
    hintArrow.addTriangle(hintArrowX - hintSize, hintArrowY - hintSize, hintArrowX - hintSize, hintArrowY + hintSize,
                          hintArrowX + hintSize * 0.8f, hintArrowY);
    g.fillPath(hintArrow);
}

void CondensedStrip::paintExpandedHeader(juce::Graphics& g)
{
    const auto& t = theme_ ? *theme_ : defaultTheme();

    auto bounds = getLocalBounds();
    int scaledHeaderHeight = static_cast<int>(collapsedHeight_ * scale_);
    auto headerBounds = bounds.removeFromTop(scaledHeaderHeight).toFloat();
    const float cornerRadius = t.weatherColors.radiusMd * scale_;

    // Header background - brass gradient
    juce::ColourGradient headerGradient(t.weatherColors.bgElevated.brighter(0.05f), headerBounds.getTopLeft(),
                                        t.weatherColors.bgRaised, headerBounds.getBottomLeft(), false);
    g.setGradientFill(headerGradient);

    // Only round top corners when expanded
    juce::Path headerPath;
    headerPath.addRoundedRectangle(headerBounds.getX(), headerBounds.getY(), headerBounds.getWidth(),
                                   headerBounds.getHeight(), cornerRadius, cornerRadius, true,
                                   true,        // top-left, top-right
                                   false, false // bottom-left, bottom-right
    );
    g.fillPath(headerPath);

    // Hover effect on header
    if (isHovering_)
    {
        g.setColour(t.weatherColors.glow.withAlpha(bws::tokens::shared::opacity::strip::GLOW_DIM));
        g.fillPath(headerPath);
    }

    // Top brass accent line
    g.setColour(t.weatherColors.accent.withAlpha(bws::tokens::shared::opacity::strip::ACCENT_EDGE));
    g.fillRect(headerBounds.getX() + cornerRadius, headerBounds.getY(), headerBounds.getWidth() - cornerRadius * 2,
               1.0f * scale_);

    // Draw down-pointing arrow
    float arrowX = 12.0f * scale_;
    float arrowY = headerBounds.getCentreY();
    float arrowSize = 5.0f * scale_;
    juce::Path arrow;
    arrow.addTriangle(arrowX - arrowSize, arrowY - arrowSize * 0.6f, arrowX + arrowSize, arrowY - arrowSize * 0.6f,
                      arrowX, arrowY + arrowSize * 0.8f);
    g.setColour(t.weatherColors.accent);
    g.fillPath(arrow);

    // Title text
    float textX = arrowX + arrowSize + 12.0f * scale_;
    g.setFont(bws::ui::adapters::makeFont(kernelTheme_, bws::ui::kernel::TextRole::Readout, scale_));
    g.drawText(title_, juce::Rectangle<float>(textX, headerBounds.getY(), 150 * scale_, headerBounds.getHeight()),
               juce::Justification::centredLeft);

    // Header border
    g.setColour(t.weatherColors.borderLight.withAlpha(bws::tokens::shared::opacity::strip::BORDER_DEFAULT));
    g.strokePath(headerPath, juce::PathStrokeType(1.0f * scale_));

    // Content area background
    auto contentBounds = getLocalBounds().toFloat();
    contentBounds.removeFromTop(scaledHeaderHeight);

    if (contentBounds.getHeight() > 0)
    {
        g.setColour(t.weatherColors.bgMain.withAlpha(bws::tokens::shared::opacity::strip::BG_EXPANSION_BASE *
                                                     expansionProgress_));

        juce::Path contentPath;
        contentPath.addRoundedRectangle(contentBounds.getX(), contentBounds.getY(), contentBounds.getWidth(),
                                        contentBounds.getHeight(), cornerRadius, cornerRadius, false,
                                        false,     // top corners
                                        true, true // bottom corners
        );
        g.fillPath(contentPath);

        // Brass border around content
        g.setColour(t.weatherColors.borderLight.withAlpha(bws::tokens::shared::opacity::strip::BORDER_EXPANSION_FADED *
                                                          expansionProgress_));

        // Left edge
        g.fillRect(contentBounds.getX(), contentBounds.getY(), 1.0f * scale_, contentBounds.getHeight() - cornerRadius);

        // Right edge
        g.fillRect(contentBounds.getRight() - 1.0f * scale_, contentBounds.getY(), 1.0f * scale_,
                   contentBounds.getHeight() - cornerRadius);

        // Bottom rounded edge
        juce::Path bottomPath;
        bottomPath.addRoundedRectangle(contentBounds.getX(), contentBounds.getBottom() - cornerRadius * 2,
                                       contentBounds.getWidth(), cornerRadius * 2, cornerRadius, cornerRadius, false,
                                       false, true, true);
        g.strokePath(bottomPath, juce::PathStrokeType(1.0f * scale_));
    }
}

void CondensedStrip::resized()
{
    updateLayout();
}

void CondensedStrip::mouseDown(const juce::MouseEvent& e)
{
    // Click header area to toggle
    int scaledHeaderHeight = static_cast<int>(collapsedHeight_ * scale_);
    auto headerArea = getLocalBounds().removeFromTop(scaledHeaderHeight);

    if (!expanded_ || headerArea.contains(e.getPosition()))
    {
        setExpanded(!expanded_, true);
    }
}

void CondensedStrip::mouseEnter(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    isHovering_ = true;
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    repaint();
}

void CondensedStrip::mouseExit(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    isHovering_ = false;
    setMouseCursor(juce::MouseCursor::NormalCursor);
    repaint();
}

//==============================================================================
// TIMER CALLBACK
//==============================================================================

void CondensedStrip::timerCallback()
{
    updateAnimation();
}

//==============================================================================
// ANIMATION
//==============================================================================

void CondensedStrip::startAnimation(bool expanding)
{
    targetProgress_ = expanding ? 1.0f : 0.0f;

    // Calculate speed based on duration at 60 FPS
    float framesNeeded = animationDurationMs_ / 16.667f;
    animationSpeed_ = 1.0f / framesNeeded;

    isAnimating_ = true;

    // Show content during expand animation
    if (content_ && expanding)
        content_->setVisible(true);

    startTimerHz(60);
}

void CondensedStrip::updateAnimation()
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

    // Update layout and repaint
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

void CondensedStrip::updateLayout()
{
    auto bounds = getLocalBounds();
    int scaledHeaderHeight = static_cast<int>(collapsedHeight_ * scale_);

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
            int padding = theme_ ? static_cast<int>(theme_->spacing.sm * scale_) : static_cast<int>(8.0f * scale_);
            auto contentArea = contentBounds_.reduced(padding);

            // Apply alpha based on expansion
            content_->setAlpha(expansionProgress_);
            content_->setBounds(contentArea);
        }
    }
    else
    {
        contentBounds_ = {};
        if (content_)
            content_->setBounds(0, 0, 0, 0);
    }
}

} // namespace bws::weather

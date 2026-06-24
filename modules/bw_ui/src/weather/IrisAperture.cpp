// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include <bw_ui/weather/displays/IrisAperture.h>
#include "bw_ui/adapters/UiThemeKernelAdapter.h"
#include "bw_ui/foundation/UiTheme.h"
#include "bw_ui/generated/BwTokens.h"

namespace bws::weather
{
namespace
{
const bws::ui::kernel::ThemeSnapshot& defaultIrisKernelTheme()
{
    static const auto kDefault = bws::ui::resolveTheme(bws::ui::defaultUiThemeBase(), {});
    static const auto kDefaultKernel = bws::ui::adapters::makeKernelThemeSnapshot(kDefault);
    return kDefaultKernel;
}
} // namespace

//==============================================================================
// CONSTRUCTION
//==============================================================================

IrisAperture::IrisAperture()
{
    setOpaque(false);
    setWantsKeyboardFocus(false);
}

IrisAperture::~IrisAperture()
{
    stopTimer();
}

//==============================================================================
// CONFIGURATION
//==============================================================================

void IrisAperture::setContent(juce::Component* content)
{
    if (content_ != content)
    {
        // Remove old content
        if (content_ != nullptr)
        {
            removeChildComponent(content_);
        }

        content_ = content;

        // Add new content
        if (content_ != nullptr)
        {
            addAndMakeVisible(content_);
            content_->setVisible(false); // Hidden initially
            updateContentVisibility();
        }

        resized();
    }
}

void IrisAperture::setOpen(bool open, bool animate)
{
    if (open_ != open)
    {
        open_ = open;
        targetProgress_ = open ? 1.0f : 0.0f;

        if (!animate)
        {
            // Instant state change - stop any in-flight animation first
            isAnimating_ = false;
            stopTimer();
            openProgress_ = targetProgress_;
            updateContentVisibility();
            repaint();

            if (onOpenChanged)
                onOpenChanged(open_);
        }
        else
        {
            // Start animation
            isAnimating_ = true;
            startTimerHz(60);
        }
    }
}

void IrisAperture::setScale(float scale)
{
    if (scale_ != scale)
    {
        scale_ = scale;
        repaint();
    }
}

void IrisAperture::setDropZoneActive(bool active)
{
    if (dropZoneActive_ != active)
    {
        dropZoneActive_ = active;

        // Start timer if not already running (for glow animation)
        if (active && !isTimerRunning())
        {
            startTimerHz(60);
        }

        repaint();
    }
}

//==============================================================================
// COMPONENT OVERRIDES
//==============================================================================

void IrisAperture::paint(juce::Graphics& g)
{
    static const auto kDefault = bws::ui::resolveTheme(bws::ui::defaultUiThemeBase(), {});
    const auto& t = theme_ ? *theme_ : kDefault;
    const auto kernelTheme = theme_ ? bws::ui::adapters::makeKernelThemeSnapshot(t) : defaultIrisKernelTheme();

    auto bounds = getLocalBounds().toFloat();

    // Background
    g.fillAll(t.weatherColors.bgMain);

    // Calculate current iris dimensions based on animation progress
    float currentDiameter = closedDiameter_ + (openDiameter_ - closedDiameter_) * openProgress_;
    float currentRadius = currentDiameter / 2.0f;

    // Iris center
    float centerX = bounds.getCentreX();
    float centerY = bounds.getCentreY();

    // Outer decorative border (brass ring around whole component when closed)
    if (openProgress_ < 0.5f)
    {
        float borderAlpha = 1.0f - (openProgress_ * 2.0f); // Fades as it opens
        g.setColour(
            t.weatherColors.borderLight.withAlpha(borderAlpha * bws::tokens::shared::opacity::iris::BORDER_MULT));
        g.drawRect(bounds.reduced(bws::tokens::shared::spacing::XXS), 1.0f);
    }

    // Draw iris opening (inner circle)
    juce::Path irisCircle;
    irisCircle.addEllipse(centerX - currentRadius, centerY - currentRadius, currentDiameter, currentDiameter);

    // Fill iris interior with slightly elevated background
    g.setColour(t.weatherColors.bgElevated);
    g.fillPath(irisCircle);

    // Draw brass blades
    drawBlades(g, centerX, centerY, currentRadius);

    // Draw inner decorative ring (visible when opening)
    if (openProgress_ > 0.1f)
    {
        drawInnerRing(g, centerX, centerY, currentRadius);
    }

    // Draw iris border (outer edge)
    float borderThickness = 2.0f + (openProgress_ * 1.0f); // Thickens as it opens
    g.setColour(t.weatherColors.accent);
    g.strokePath(irisCircle, juce::PathStrokeType(borderThickness));

    // Draw drop zone glow (when detached window is hovering near)
    if (dropZoneActive_ || dropZoneGlowPhase_ > 0.01f)
    {
        // Pulsing alpha: 0.3 → 0.7 → 0.3 (smooth sine wave, 1.5-2 sec cycle)
        // Base alpha 0.5, ±0.2 gives range 0.3 → 0.7
        float pulseAlpha = 0.5f + (std::sin(dropZoneGlowPhase_) * 0.2f);

        // If not active but fading out, use the phase as fade multiplier
        if (!dropZoneActive_)
            pulseAlpha *= dropZoneGlowPhase_ / juce::MathConstants<float>::halfPi;

        // Outermost glow halo (very soft, wide spread)
        g.setColour(t.weatherColors.glow.withAlpha(pulseAlpha * bws::tokens::shared::opacity::iris::GLOW_PULSE_SUBTLE));
        g.drawEllipse(centerX - currentRadius - 18.0f, centerY - currentRadius - 18.0f, currentDiameter + 36.0f,
                      currentDiameter + 36.0f,
                      14.0f); // Wide soft halo

        // Outer glow ring (thick, prominent)
        g.setColour(t.weatherColors.glow.withAlpha(pulseAlpha * bws::tokens::shared::opacity::iris::GLOW_PULSE_MEDIUM));
        g.drawEllipse(centerX - currentRadius - 10.0f, centerY - currentRadius - 10.0f, currentDiameter + 20.0f,
                      currentDiameter + 20.0f,
                      8.0f); // Main outer glow

        // Inner glow ring (bright, crisp edge)
        g.setColour(t.weatherColors.glow.withAlpha(pulseAlpha));
        g.drawEllipse(centerX - currentRadius - 4.0f, centerY - currentRadius - 4.0f, currentDiameter + 8.0f,
                      currentDiameter + 8.0f,
                      3.0f); // Crisp inner glow
    }

    // Subtle inner shadow for depth
    {
        juce::ColourGradient shadowGradient(
            juce::Colours::black.withAlpha(bws::tokens::shared::opacity::iris::CLOSING_MULT * (1.0f - openProgress_)),
            centerX, centerY - currentRadius, juce::Colours::transparentBlack, centerX, centerY, false);
        g.setGradientFill(shadowGradient);
        g.fillPath(irisCircle);
    }

    // Closed state label/hint - smooth fade on hover over iris circle
    if (openProgress_ < 0.2f && hoverTextAlpha_ > 0.01f)
    {
        float labelAlpha = hoverTextAlpha_ * (1.0f - (openProgress_ * 5.0f)); // Fades as iris opens

        // Draw hint text centered in iris
        g.setColour(
            t.weatherColors.textSecondary.withAlpha(labelAlpha * bws::tokens::shared::opacity::iris::LABEL_MULT));
        g.setFont(bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::ControlText, scale_));

        juce::Rectangle<float> labelBounds(centerX - currentRadius,
                                           centerY - 8.0f, // Vertically centered
                                           currentRadius * 2.0f, 20.0f);
        g.drawText("Click to reveal", labelBounds, juce::Justification::centred);
    }

    // Draw small aperture symbol in center when fully closed (fades out as hover text fades in)
    if (openProgress_ < 0.05f)
    {
        float symbolAlpha = 0.7f * (1.0f - hoverTextAlpha_); // Inverse of hover text
        if (symbolAlpha > 0.01f)
        {
            g.setColour(t.weatherColors.accent.withAlpha(symbolAlpha));
            g.setFont(bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::Title, scale_));
            g.drawText(juce::CharPointer_UTF8("\xe2\x97\x8f"), // ● symbol
                       juce::Rectangle<float>(centerX - 12, centerY - 10, 24, 20), juce::Justification::centred);
        }
    }

    // Hover highlight when closed - only when hovering over the iris circle
    if (isHoveringIris_ && openProgress_ < 0.5f)
    {
        g.setColour(t.weatherColors.glow.withAlpha(bws::tokens::shared::opacity::iris::HOVER_GLOW * hoverTextAlpha_));
        g.fillPath(irisCircle);
    }
}

void IrisAperture::drawBlades(juce::Graphics& g, float centerX, float centerY, float currentRadius)
{
    static const auto kDefault = bws::ui::resolveTheme(bws::ui::defaultUiThemeBase(), {});
    const auto& t = theme_ ? *theme_ : kDefault;

    // Blade rotation increases as iris opens (blades retract outward)
    // 0 degrees when closed, 45 degrees when fully open
    float bladeRotation = openProgress_ * 45.0f;

    for (int i = 0; i < NUM_BLADES; ++i)
    {
        auto bladePath = createBladePath(i, currentRadius, bladeRotation);

        // Blade gradient fill (brass with depth)
        {
            float angleStep = 360.0f / NUM_BLADES;
            float bladeAngle = i * angleStep + bladeRotation;
            float angleRad = juce::degreesToRadians(bladeAngle);

            // Gradient from center outward for metallic effect
            float outerX = centerX + std::cos(angleRad) * (currentRadius + 30.0f);
            float outerY = centerY + std::sin(angleRad) * (currentRadius + 30.0f);

            juce::ColourGradient bladeGradient(t.weatherColors.accent.darker(0.4f), centerX, centerY,
                                               t.weatherColors.accent.darker(0.7f), outerX, outerY, false);
            g.setGradientFill(bladeGradient);
            g.fillPath(bladePath);
        }

        // Blade highlight edge (top edge catches light)
        g.setColour(t.weatherColors.accent.withAlpha(bws::tokens::shared::opacity::iris::ACCENT_FULL));
        g.strokePath(bladePath, juce::PathStrokeType(1.0f));

        // Blade shadow edge (bottom edge in shadow)
        g.setColour(t.weatherColors.borderDark.withAlpha(bws::tokens::shared::opacity::iris::BORDER_DARK));
        g.strokePath(bladePath, juce::PathStrokeType(0.5f));
    }
}

juce::Path IrisAperture::createBladePath(int bladeIndex, float currentRadius, float bladeRotation)
{
    auto bounds = getLocalBounds().toFloat();
    float centerX = bounds.getCentreX();
    float centerY = bounds.getCentreY();

    // Each blade is positioned radially around the iris
    float angleStep = 360.0f / NUM_BLADES;
    float bladeAngle = bladeIndex * angleStep + bladeRotation;
    float angleRad = juce::degreesToRadians(bladeAngle);

    // Blade dimensions
    // Length shrinks as iris opens (blades retract)
    float bladeLength = 35.0f * (1.0f - openProgress_ * 0.8f);
    // Width narrows slightly when open
    float bladeWidth = 12.0f * (1.0f - openProgress_ * 0.3f);

    // Inner point (on iris circle edge)
    float innerX = centerX + std::cos(angleRad) * currentRadius;
    float innerY = centerY + std::sin(angleRad) * currentRadius;

    // Outer point (blade tip, extends outward from iris edge)
    float outerX = centerX + std::cos(angleRad) * (currentRadius + bladeLength);
    float outerY = centerY + std::sin(angleRad) * (currentRadius + bladeLength);

    // Perpendicular direction for blade width
    float perpAngle = angleRad + juce::MathConstants<float>::halfPi;
    float perpX = std::cos(perpAngle) * bladeWidth;
    float perpY = std::sin(perpAngle) * bladeWidth;

    // Create blade polygon (trapezoid that tapers at tip)
    juce::Path blade;
    blade.startNewSubPath(innerX - perpX, innerY - perpY);
    blade.lineTo(innerX + perpX, innerY + perpY);
    blade.lineTo(outerX + perpX * 0.4f, outerY + perpY * 0.4f); // Tapered tip
    blade.lineTo(outerX - perpX * 0.4f, outerY - perpY * 0.4f);
    blade.closeSubPath();

    return blade;
}

void IrisAperture::drawInnerRing(juce::Graphics& g, float centerX, float centerY, float radius)
{
    static const auto kDefault = bws::ui::resolveTheme(bws::ui::defaultUiThemeBase(), {});
    const auto& t = theme_ ? *theme_ : kDefault;

    // Decorative inner ring that appears as iris opens
    float ringAlpha = std::min(1.0f, (openProgress_ - 0.1f) * 1.5f);
    float innerRingRadius = radius * 0.92f;

    // Inner brass ring
    g.setColour(t.weatherColors.accent.withAlpha(ringAlpha * bws::tokens::shared::opacity::iris::RING_MULT));
    g.drawEllipse(centerX - innerRingRadius, centerY - innerRingRadius, innerRingRadius * 2.0f, innerRingRadius * 2.0f,
                  1.5f);

    // Subtle tick marks around ring (like camera aperture markings)
    if (openProgress_ > 0.5f)
    {
        float tickAlpha = (openProgress_ - 0.5f) * 2.0f;
        g.setColour(t.weatherColors.textTertiary.withAlpha(tickAlpha * bws::tokens::shared::opacity::iris::TICK_MULT));

        for (int i = 0; i < 16; ++i)
        {
            float tickAngle = juce::degreesToRadians(i * 22.5f);
            float innerTick = innerRingRadius - 3.0f;
            float outerTick = innerRingRadius + 3.0f;

            float x1 = centerX + std::cos(tickAngle) * innerTick;
            float y1 = centerY + std::sin(tickAngle) * innerTick;
            float x2 = centerX + std::cos(tickAngle) * outerTick;
            float y2 = centerY + std::sin(tickAngle) * outerTick;

            g.drawLine(x1, y1, x2, y2, 1.0f);
        }
    }
}

void IrisAperture::resized()
{
    if (content_ == nullptr)
        return;

    // Content fills the iris area, centered
    auto bounds = getLocalBounds();

    // Center content within the iris opening
    // Use open diameter as content size since that's when content is visible
    int contentSize = openDiameter_;
    int x = (bounds.getWidth() - contentSize) / 2;
    int y = (bounds.getHeight() - contentSize) / 2;

    content_->setBounds(x, y, contentSize, contentSize);
}

void IrisAperture::mouseDown(const juce::MouseEvent& /*e*/)
{
    // Ignore clicks when content is detached (content_ == nullptr)
    // This prevents confusing state where iris animates but shows nothing
    if (content_ == nullptr)
        return;

    // Toggle open/close on click
    setOpen(!open_, true);
}

void IrisAperture::mouseEnter(const juce::MouseEvent& e)
{
    isHovering_ = true;

    // Check if cursor is within the iris circle
    auto bounds = getLocalBounds().toFloat();
    float centerX = bounds.getCentreX();
    float centerY = bounds.getCentreY();
    float currentDiameter = closedDiameter_ + (openDiameter_ - closedDiameter_) * openProgress_;
    float currentRadius = currentDiameter / 2.0f;

    float dx = e.position.x - centerX;
    float dy = e.position.y - centerY;
    float distanceFromCenter = std::sqrt(dx * dx + dy * dy);

    isHoveringIris_ = (distanceFromCenter <= currentRadius);

    // Show pointing hand if content is present and hovering over iris (any state)
    if (content_ != nullptr && isHoveringIris_)
        setMouseCursor(juce::MouseCursor::PointingHandCursor);

    // Start timer for hover text fade animation (only when closed)
    if (openProgress_ < 0.5f && !isTimerRunning())
        startTimerHz(60);

    if (openProgress_ < 0.5f)
        repaint();
}

void IrisAperture::mouseMove(const juce::MouseEvent& e)
{
    // Check if cursor is within the iris circle
    auto bounds = getLocalBounds().toFloat();
    float centerX = bounds.getCentreX();
    float centerY = bounds.getCentreY();
    float currentDiameter = closedDiameter_ + (openDiameter_ - closedDiameter_) * openProgress_;
    float currentRadius = currentDiameter / 2.0f;

    float dx = e.position.x - centerX;
    float dy = e.position.y - centerY;
    float distanceFromCenter = std::sqrt(dx * dx + dy * dy);

    bool wasHoveringIris = isHoveringIris_;
    isHoveringIris_ = (distanceFromCenter <= currentRadius);

    // Update cursor based on iris hover - works for both open and closed states
    if (content_ != nullptr)
    {
        if (isHoveringIris_)
            setMouseCursor(juce::MouseCursor::PointingHandCursor);
        else
            setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    // Start timer for fade animation if hover state changed (only when closed)
    if (wasHoveringIris != isHoveringIris_ && openProgress_ < 0.5f)
    {
        if (!isTimerRunning())
            startTimerHz(60);
        repaint();
    }
}

void IrisAperture::mouseExit(const juce::MouseEvent& /*e*/)
{
    isHovering_ = false;
    isHoveringIris_ = false;
    setMouseCursor(juce::MouseCursor::NormalCursor);

    // Start timer for fade-out animation
    if (openProgress_ < 0.5f && hoverTextAlpha_ > 0.0f && !isTimerRunning())
        startTimerHz(60);

    if (openProgress_ < 0.5f)
        repaint();
}

//==============================================================================
// TIMER CALLBACK
//==============================================================================

void IrisAperture::timerCallback()
{
    bool needsRepaint = false;

    // Spring animation towards target (iris open/close)
    float delta = targetProgress_ - openProgress_;

    if (std::abs(delta) < ANIMATION_THRESHOLD)
    {
        // Animation complete
        if (isAnimating_)
        {
            openProgress_ = targetProgress_;
            isAnimating_ = false;

            // Notify completion
            if (onOpenChanged)
                onOpenChanged(open_);
        }
    }
    else
    {
        // Spring interpolation with damping
        openProgress_ += delta * ANIMATION_SPEED;
        needsRepaint = true;
    }

    // Hover text fade animation (snappy but smooth)
    // Fade speed: ~0.08 per frame at 60fps = ~0.2 second fade in/out
    constexpr float kHoverFadeSpeed = 0.08f;

    if (isHoveringIris_ && openProgress_ < 0.2f)
    {
        // Fade in when hovering over iris
        if (hoverTextAlpha_ < 1.0f)
        {
            hoverTextAlpha_ = std::min(1.0f, hoverTextAlpha_ + kHoverFadeSpeed);
            needsRepaint = true;
        }
    }
    else
    {
        // Fade out when not hovering over iris
        if (hoverTextAlpha_ > 0.0f)
        {
            hoverTextAlpha_ = std::max(0.0f, hoverTextAlpha_ - kHoverFadeSpeed);
            needsRepaint = true;
        }
    }

    // Drop zone glow pulse animation
    // Target: 1.5-2 second cycle (2π radians / 1.75s / 60fps ≈ 0.06 radians per frame)
    if (dropZoneActive_)
    {
        dropZoneGlowPhase_ += 0.06f; // ~1.75 second full cycle at 60fps
        if (dropZoneGlowPhase_ > juce::MathConstants<float>::twoPi)
            dropZoneGlowPhase_ -= juce::MathConstants<float>::twoPi;
        needsRepaint = true;
    }
    else if (dropZoneGlowPhase_ > 0.01f)
    {
        // Fade out glow smoothly when not in drop zone
        dropZoneGlowPhase_ = std::max(0.0f, dropZoneGlowPhase_ - 0.15f);
        needsRepaint = true;
    }

    // Stop timer if no animations active
    bool hoverAnimating = (hoverTextAlpha_ > 0.0f && hoverTextAlpha_ < 1.0f) ||
                          (isHoveringIris_ && hoverTextAlpha_ < 1.0f) || (!isHoveringIris_ && hoverTextAlpha_ > 0.0f);

    if (!isAnimating_ && !dropZoneActive_ && dropZoneGlowPhase_ < 0.01f && !hoverAnimating)
    {
        stopTimer();
    }

    // Notify progress
    if (onAnimationProgress)
        onAnimationProgress(openProgress_);

    updateContentVisibility();

    if (needsRepaint)
        repaint();
}

//==============================================================================
// ANIMATION HELPERS
//==============================================================================

void IrisAperture::updateContentVisibility()
{
    if (content_ == nullptr)
        return;

    // Content visible when iris is 35% open (blades parted enough to see through cleanly)
    bool shouldBeVisible = openProgress_ > 0.35f;
    content_->setVisible(shouldBeVisible);

    if (shouldBeVisible)
    {
        // Near-instant ramp: 0% at 0.35, 100% at ~0.475 (~2 frames at 60fps)
        float alpha = juce::jmin(1.0f, (openProgress_ - 0.35f) * 8.0f);
        content_->setAlpha(alpha);
    }
}

} // namespace bws::weather

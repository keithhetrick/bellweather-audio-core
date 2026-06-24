// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/Components/UiArcKnob.h"
#include <bw_ui/Components/UiArcKnobTickGeometry.h>
#include <bw_ui/interaction/InteractionLogging.h>
#include <bw_ui/interaction/ResetExecutor.h>
#include <bw_ui/kernel/DoubleClickDecision.h>
#include <bw_ui/adapters/FineDragModifier.h>
#include <bw_ui/kernel/InteractionContract.h>
#include <bw_ui/rendering/JuceRenderer.h>
#include "bw_ui/adapters/UiThemeKernelAdapter.h"
#include "bw_ui/foundation/Fonts.h"
#include "bw_ui/foundation/UiTheme.h"

#include <variant>

namespace bws::ui
{
namespace
{
constexpr float kLockedAlpha = bws::tokens::shared::opacity::arc_knob::LOCKED_DIM;
}

// Static member initialization
juce::String UiArcKnob::clipboardValueKey_ = "UiArcKnob_ClipboardValue";

UiArcKnob::UiArcKnob()
{
    const auto geometry = kernel::resolveArcGeometry(knobRole_);
    setSliderStyle(juce::Slider::RotaryVerticalDrag);
    setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    setRotaryParameters(geometry.startAngle, geometry.endAngle, true);
    setWantsKeyboardFocus(true);

    // Default value formatter for dB display
    valueFormatter_ = defaultValueFormatter;

    // Set cursor for better UX
    setMouseCursor(juce::MouseCursor::PointingHandCursor);

    policy_.doubleClick.reset =
        interaction::CallbackReset {[this]() { setValue(getDoubleClickReturnValue(), juce::sendNotificationAsync); }};
}

void UiArcKnob::setKnobRole(UiKnobRole role)
{
    knobRole_ = role;
    resolvedModel_ = kernel::resolveKnobModel(role);
    const auto geometry = kernel::resolveArcGeometry(role);
    setRotaryParameters(geometry.startAngle, geometry.endAngle, true);
    repaint();
}

void UiArcKnob::setValueDisplayFormat(std::function<juce::String(double)> formatter)
{
    valueFormatter_ = formatter ? std::move(formatter) : defaultValueFormatter;
    repaint();
}

void UiArcKnob::setTrimDisplayFormat(std::function<juce::String(double)> formatter)
{
    trimFormatter_ = std::move(formatter);
    repaint();
}

void UiArcKnob::setFineTuneModeForTesting(bool enabled)
{
    isFineTuneMode_ = enabled;
    repaint();
}

void UiArcKnob::setBackgroundColours(juce::Colour top, juce::Colour bottom)
{
    bgTopColour_ = top;
    bgBottomColour_ = bottom;
    repaint();
}

void UiArcKnob::setLocked(bool locked)
{
    if (locked_ == locked)
        return;
    locked_ = locked;
    if (locked_)
        cancelUserInteraction();
    repaint();
}

bool UiArcKnob::hasConfiguredTrimCapability() const noexcept
{
    return resolvedModel_.trimCapable && static_cast<bool>(onTrimValueChange) && static_cast<bool>(getTrimValue);
}

void UiArcKnob::cancelUserInteraction()
{
    if (isTrimMode_ && onTrimDragEnd)
        onTrimDragEnd();

    isTrimMode_ = false;
    isFineTuneMode_ = false;
    isTrimPreviewMode_ = false;
    inlineEditor_.reset();
}

juce::String UiArcKnob::defaultValueFormatter(double value)
{
    // Handle -infinity display for extreme low values
    if (value <= -96.0)
        return juce::String::fromUTF8("-\u221E dB"); // -∞ dB

    if (value >= 0.0)
        return "+" + juce::String(value, 1) + " dB";
    else
        return juce::String(value, 1) + " dB";
}

void UiArcKnob::paint(juce::Graphics& g)
{
    static const auto kDefault = bws::ui::resolveTheme(bws::ui::defaultUiThemeBase(), {});
    static const auto kDefaultKernel = bws::ui::adapters::makeKernelThemeSnapshot(kDefault);
    const auto& wc = kDefault.weatherColors;

    rendering::JuceRenderer renderer(g);

    const float alphaMultiplier = locked_ ? kLockedAlpha : 1.0f;
    renderer.saveState();
    renderer.setOpacity(alphaMultiplier);

    const auto bounds = getLocalBounds().toFloat();
    const auto cx = bounds.getCentreX();
    const auto cy = bounds.getCentreY();
    const auto diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
    const auto radius = diameter / 2.0f;

    // Inset for visual padding
    const float inset = 4.0f;
    const auto knobBounds = bounds.reduced(inset);
    const auto faceBounds = knobBounds.reduced(bws::tokens::shared::spacing::XXS);
    const float faceRadius = juce::jmin(faceBounds.getWidth(), faceBounds.getHeight()) * 0.5f;
    const auto geometry = kernel::resolveArcGeometry(knobRole_);

    // 1. OUTER RING (subtle border)
    const auto ringBounds = knobBounds.reduced(bws::tokens::shared::geometry::STROKE_FULL_PX);
    renderer.setColour(ringColour_.getARGB());
    renderer.drawEllipse(ringBounds.getX(), ringBounds.getY(), ringBounds.getWidth(), ringBounds.getHeight(), 2.0f);

    // 2. BACKGROUND FILL (dark gradient)
    auto bgH = renderer.createLinearGradient(cx, knobBounds.getY(), cx, knobBounds.getBottom());
    renderer.addGradientStop(bgH, 0.0, bgTopColour_.getARGB());
    renderer.addGradientStop(bgH, 1.0, bgBottomColour_.getARGB());
    renderer.applyGradient(bgH);
    renderer.fillEllipse(faceBounds.getX(), faceBounds.getY(), faceBounds.getWidth(), faceBounds.getHeight());
    renderer.destroyGradient(bgH);

    // 3. HOVER GLOW (optional)
    if (!locked_ && showHoverGlow_ && isMouseOver())
    {
        const auto glowBounds = knobBounds.reduced(bws::tokens::shared::spacing::XS);
        renderer.setColour(arcColour_.withAlpha(bws::tokens::shared::opacity::arc_knob::HOVER_GLOW).getARGB());
        renderer.fillEllipse(glowBounds.getX(), glowBounds.getY(), glowBounds.getWidth(), glowBounds.getHeight());
    }

    // 4. ARC INDICATOR
    const float sliderPos = static_cast<float>(valueToProportionOfLength(getValue()));
    const float angle = geometry.startAngle + sliderPos * (geometry.endAngle - geometry.startAngle);
    const float arcRadius = juce::jmax(1.0f, faceRadius * geometry.familyTrackRadiusRatio);

    // Precision blue tints the gain arc only when fine-tuning gain, not trim.
    const auto baseArcColour = (isFineTuneMode_ && !isTrimMode_) ? wc.precisionBlue : arcColour_;

    // Apply 15% brightness increase on hover for better feedback.
    const auto activeArcColour =
        (!locked_ && showHoverGlow_ && isMouseOver()) ? baseArcColour.brighter(0.15f) : baseArcColour;

    // Background arc (dim) - shows full range
    drawArc(renderer, geometry.startAngle, geometry.endAngle, arcRadius, arcThickness_, dimArcColour_);

    // Active arc (brass gold, brightened on hover) - shows current value
    if (sliderPos > 0.001f)
    {
        drawArc(renderer, geometry.startAngle, angle, arcRadius, arcThickness_, activeArcColour);
    }

    const bool trimAvailable = hasConfiguredTrimCapability();

    // 4b. TRIM ARC (yellow, shown only during active trim adjustment)
    if (trimAvailable && isTrimMode_ && getTrimValue)
    {
        const float trimDb = getTrimValue();
        // Convert trim dB (-12 to +12) to normalized position (0 to 1)
        const float trimNormalized = (trimDb + 12.0f) / 24.0f;
        const float trimAngle = geometry.startAngle + trimNormalized * (geometry.endAngle - geometry.startAngle);

        // Keep trim tied to the same family track instead of a renderer-local inset.
        const float trimArcRadius =
            juce::jmax(1.0f, faceRadius * (geometry.familyTrackRadiusRatio - geometry.trimTrackInsetRatio));
        drawArc(renderer, geometry.startAngle, geometry.endAngle, trimArcRadius, 3.0f,
                dimArcColour_.withAlpha(bws::tokens::shared::opacity::arc_knob::TRIM_TRACK_DIM));

        // Fine attaches to the trim arc when refining trim.
        const auto trimActiveColour = isFineTuneMode_ ? wc.precisionBlue : trimArcColour_;
        if (trimNormalized > 0.001f)
        {
            drawArc(renderer, geometry.startAngle, trimAngle, trimArcRadius, 3.0f, trimActiveColour);
        }
    }

    // 5. TICK MARK (optional, at specified value like 0 dB)
    if (showTickMark_)
    {
        const float tickPos = static_cast<float>(valueToProportionOfLength(tickMarkValue_));
        // Round-cap visual overshoot: the stroked arc extends half-thickness past
        // its geometric endpoint along the tangent. Shift the tick by the same
        // angular amount so it coincides with the perceived arc end.
        const float capOvershootAngle = (arcThickness_ * 0.5f) / juce::jmax(1.0f, arcRadius);
        const float tickAngle =
            geometry.startAngle + tickPos * (geometry.endAngle - geometry.startAngle) + capOvershootAngle;

        const auto tick = resolveArcTickMetrics(diameter);
        const float tickOuterRadius = arcRadius + tick.outerDelta;
        const float tickInnerRadius = arcRadius - tick.innerDelta;

        // JUCE angles: 0 = right, PI/2 = down, so we need to adjust
        const float adjustedAngle = tickAngle - juce::MathConstants<float>::halfPi;

        const float x1 = cx + tickOuterRadius * std::cos(adjustedAngle);
        const float y1 = cy + tickOuterRadius * std::sin(adjustedAngle);
        const float x2 = cx + tickInnerRadius * std::cos(adjustedAngle);
        const float y2 = cy + tickInnerRadius * std::sin(adjustedAngle);

        renderer.setColour(wc.tickMark.getARGB());
        renderer.drawLine(x1, y1, x2, y2, tick.lineThickness);

        const float labelRadius = tickInnerRadius - tick.labelInnerDelta;
        const float labelX = cx + labelRadius * std::cos(adjustedAngle) - tick.labelOffsetX;
        const float labelY = cy + labelRadius * std::sin(adjustedAngle) - tick.labelOffsetY;
        g.setFont(
            bws::ui::adapters::makeFont(kDefaultKernel, bws::ui::kernel::TextRole::ControlText, tick.labelFontScale));
        g.drawText("0", static_cast<int>(labelX), static_cast<int>(labelY), static_cast<int>(tick.labelBoxW),
                   static_cast<int>(tick.labelBoxH), juce::Justification::centred, false);
    }

    // 6. DIGITAL READOUT (centered, large)
    // Show trim value if in trim mode or trim preview, otherwise show main value
    const bool showTrimValue = trimAvailable && (isTrimMode_ || isTrimPreviewMode_) && trimFormatter_;

    if (resolvedModel_.internalReadoutAllowed)
    {
        juce::String valueText;
        juce::Colour textColour = juce::Colours::white.withAlpha(bws::tokens::shared::opacity::arc_knob::READOUT_TEXT);
        if (showTrimValue)
        {
            const float trimDb = getTrimValue();
            valueText = trimFormatter_(static_cast<double>(trimDb));
            textColour = isTrimMode_
                             ? wc.trimYellow
                             : wc.trimYellow.withAlpha(bws::tokens::shared::opacity::arc_knob::TRIM_PREVIEW_TEXT);
        }
        else
        {
            valueText = valueFormatter_(getValue());
        }

        renderer.setColour(textColour.getARGB());

        // Font size proportional to knob diameter (~11% of diameter)
        const float proportionalFontSize = diameter * 0.11f;
        const float effectiveFontSize = (valueFontSize_ != 28.0f) ? valueFontSize_ : proportionalFontSize;
        renderer.setFont(effectiveFontSize, rendering::FontWeight::Bold);

        // Text area - centered in the knob
        const auto textBounds = knobBounds.reduced(radius * 0.25f);
        renderer.drawText(valueText.toRawUTF8(), textBounds.getX(), textBounds.getY(), textBounds.getWidth(),
                          textBounds.getHeight(), rendering::Justification::Centre);

        // 6b. FINE MODE BADGE (shown during Shift+drag)
        if (isFineTuneMode_)
        {
            const float badgeFontSize = juce::jmax(8.0f, diameter * 0.04f);
            renderer.setFont(badgeFontSize, rendering::FontWeight::Bold);
            renderer.setColour(
                wc.precisionBlue.withAlpha(bws::tokens::shared::opacity::arc_knob::FINE_BADGE_TEXT).getARGB());
            auto badgeBounds =
                textBounds.withTrimmedTop(textBounds.getHeight() * 0.65f).withHeight(badgeFontSize + 2.0f);
            renderer.drawText("FINE", badgeBounds.getX(), badgeBounds.getY(), badgeBounds.getWidth(),
                              badgeBounds.getHeight(), rendering::Justification::Centre);
        }
    }

    // 7. SUBTLE TOP SHINE (highlight for depth)
    const auto shineBounds = knobBounds.reduced(bws::tokens::shared::spacing::XS);
    auto shineH = renderer.createLinearGradient(cx, shineBounds.getY(), cx, shineBounds.getCentreY());
    renderer.addGradientStop(shineH, 0.0,
                             juce::Colours::white.withAlpha(bws::tokens::shared::opacity::shadow::SUBTLE).getARGB());
    renderer.addGradientStop(shineH, 1.0, juce::Colours::transparentBlack.getARGB());
    renderer.applyGradient(shineH);

    // Only fill top half with shine - clip to ellipse via arcTo (full circle)
    const float shineRadius = juce::jmin(shineBounds.getWidth(), shineBounds.getHeight()) / 2.0f;
    renderer.saveState();
    renderer.beginPath();
    renderer.arcTo(shineBounds.getCentreX(), shineBounds.getCentreY(), shineRadius, 0.0f,
                   juce::MathConstants<float>::twoPi);
    renderer.closePath();
    renderer.clipPath();
    renderer.fillRect(shineBounds.getX(), shineBounds.getY(), shineBounds.getWidth(), shineBounds.getHeight() / 2.0f);
    renderer.restoreState();
    renderer.destroyGradient(shineH);

    renderer.restoreState();
}

void UiArcKnob::drawArc(rendering::IRenderer& r, float startAngle, float endAngle, float radius, float thickness,
                        juce::Colour colour)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto cx = bounds.getCentreX();
    const auto cy = bounds.getCentreY();

    r.setColour(colour.getARGB());
    r.beginPath();
    r.arcTo(cx, cy, radius, startAngle, endAngle);
    r.strokePath(thickness);
}

bool UiArcKnob::hitTest(int x, int y)
{
    // Only respond to clicks within the circular knob area
    const auto bounds = getLocalBounds().toFloat();
    const auto cx = bounds.getCentreX();
    const auto cy = bounds.getCentreY();
    const auto diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
    const auto radius = diameter / 2.0f;

    // Calculate distance from center
    const float dx = static_cast<float>(x) - cx;
    const float dy = static_cast<float>(y) - cy;
    const float distance = std::sqrt(dx * dx + dy * dy);

    // Hit if within the circular area (with small inset to match visual)
    return distance <= (radius - 4.0f);
}

void UiArcKnob::mouseDown(const juce::MouseEvent& e)
{
    if (locked_)
        return;
    // Right-click: Show context menu
    // Cmd+right-click shows trim context menu if trim mode is available
    if (e.mods.isPopupMenu())
    {
        const bool showTrimMenu =
            kernel::isSecondaryAxisGesture(kernel::fromJuce(e.mods), onTrimValueChange != nullptr) && getTrimValue;
        showContextMenu(showTrimMenu);
        return;
    }

    if (kernel::isSecondaryAxisGesture(kernel::fromJuce(e.mods), onTrimValueChange != nullptr) && e.mods.isAltDown())
    {
        onTrimValueChange(0.5f);
        repaint();
        return;
    }

    gainDragStartY_ = e.y;
    gainDragStartValue_ = static_cast<float>(valueToProportionOfLength(getValue()));

    isFineTuneMode_ = kernel::isFineGesture(kernel::fromJuce(e.mods));
    isTrimMode_ = kernel::isSecondaryAxisGesture(kernel::fromJuce(e.mods), onTrimValueChange != nullptr);

    if (isTrimMode_)
    {
        // Trim mode: handle manually (not using Slider's built-in drag)
        trimDragStartY_ = e.y;
        if (getTrimValue)
        {
            // Convert current trim dB (-12 to +12) to normalized (0-1)
            const float trimDb = getTrimValue();
            trimDragStartValue_ = (trimDb + 12.0f) / 24.0f;
        }
        else
        {
            trimDragStartValue_ = 0.5f; // Default to center (0 dB trim)
        }

        // Notify start of trim gesture (for DAW automation recording)
        if (onTrimDragStart)
            onTrimDragStart();

        repaint(); // Update display to show trim mode
    }
    else
    {
        setVelocityBasedMode(false);
        kernel::applyFineDragModifier(*this, e.mods);
        juce::Slider::mouseDown(e);
    }
}

void UiArcKnob::mouseDrag(const juce::MouseEvent& e)
{
    if (locked_)
        return;
    const bool wantsTrimMode = kernel::isSecondaryAxisGesture(kernel::fromJuce(e.mods), hasConfiguredTrimCapability());

    // Handle mode switching mid-drag
    if (wantsTrimMode && !isTrimMode_)
    {
        // Switching FROM gain TO trim mid-drag
        isTrimMode_ = true;
        trimDragStartY_ = e.y;
        if (getTrimValue)
        {
            const float trimDb = getTrimValue();
            trimDragStartValue_ = (trimDb + 12.0f) / 24.0f;
        }
        else
        {
            trimDragStartValue_ = 0.5f;
        }

        // Notify start of trim gesture (for DAW automation recording)
        if (onTrimDragStart)
            onTrimDragStart();

        repaint();
    }
    else if (!wantsTrimMode && isTrimMode_)
    {
        // Switching FROM trim TO gain mid-drag
        // Notify end of trim gesture (for DAW automation recording)
        if (onTrimDragEnd)
            onTrimDragEnd();

        isTrimMode_ = false;
        // Reset gain drag from current position
        gainDragStartY_ = e.y;
        gainDragStartValue_ = static_cast<float>(valueToProportionOfLength(getValue()));
        repaint();
    }

    if (isTrimMode_)
    {
        const bool wantsFineTune = kernel::isFineGesture(kernel::fromJuce(e.mods));

        // Re-anchor from the current position when fine toggles, so Cmd refines the
        // active trim from where it is rather than jumping the value.
        if (wantsFineTune != isFineTuneMode_)
        {
            isFineTuneMode_ = wantsFineTune;
            trimDragStartY_ = e.y;
            if (getTrimValue)
                trimDragStartValue_ = (getTrimValue() + 12.0f) / 24.0f;
            repaint();
        }

        constexpr kernel::FineDragSensitivity kTrimSensitivity {};
        const int sensitivity = isFineTuneMode_ ? kTrimSensitivity.fineSensitivity : kTrimSensitivity.baseSensitivity;

        const float delta = static_cast<float>(trimDragStartY_ - e.y) / static_cast<float>(sensitivity);
        const float newValue = juce::jlimit(0.0f, 1.0f, trimDragStartValue_ + delta);

        if (onTrimValueChange)
        {
            onTrimValueChange(newValue);
        }

        repaint();
    }
    else
    {
        const bool wantsFineTune = kernel::isFineGesture(kernel::fromJuce(e.mods));

        if (wantsFineTune != isFineTuneMode_)
        {
            isFineTuneMode_ = wantsFineTune;
            repaint(); // Update fine-mode visual indicator
        }

        kernel::applyFineDragModifier(*this, e.mods);
        juce::Slider::mouseDrag(e);
    }
}

void UiArcKnob::mouseUp(const juce::MouseEvent& e)
{
    if (locked_)
    {
        cancelUserInteraction();
        return;
    }
    const bool wasTrimMode = isTrimMode_;
    const bool wasFineTuneMode = isFineTuneMode_;
    isTrimMode_ = false;
    isFineTuneMode_ = false;

    if (wasTrimMode)
    {
        // Notify end of trim gesture (for DAW automation recording)
        if (onTrimDragEnd)
            onTrimDragEnd();

        repaint(); // Restore normal display
    }
    else
    {
        juce::Slider::mouseUp(e);
        if (wasFineTuneMode)
            repaint(); // Clear fine-mode visual indicator
    }
}

void UiArcKnob::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (locked_)
        return;

    if (kernel::isSecondaryAxisGesture(kernel::fromJuce(e.mods), hasConfiguredTrimCapability()))
    {
        showTrimTextEntry();
        return;
    }

    if (!e.mods.isAnyModifierKeyDown())
    {
        JUCE_ASSERT_MESSAGE_THREAD;
        const kernel::WiringState w {
            .hasTextEntryCallback = static_cast<bool>(policy_.doubleClick.onTextEntry),
            .hasResetMechanism = !std::holds_alternative<interaction::NoReset>(policy_.doubleClick.reset),
            .hasCustomCallback = static_cast<bool>(policy_.doubleClick.onCustom),
            .locked = locked_,
            .textEntryDelegates = false,
        };
        const auto outcome = kernel::decideBareDoubleClick(policy_.doubleClick.action, w);
        const char* diag = policy_.diagnosticName ? policy_.diagnosticName->c_str() : getName().toRawUTF8();

        using O = kernel::BareDoubleClickOutcome;
        switch (outcome)
        {
        case O::InvokeTextEntry:
            policy_.doubleClick.onTextEntry();
            return;
        case O::InvokeReset:
            interaction::performReset(policy_.doubleClick.reset);
            return;
        case O::InvokeCustom:
            policy_.doubleClick.onCustom();
            return;
        case O::AutoDegradeTextEntry:
            logAutoDegrade("UiArcKnob", diag, "TextEntry", "UiArcKnob has hidden readout");
            return;
        case O::AutoDegradeCustom:
            bws::ui::logAutoDegrade(
                "UiArcKnob", diag, "Custom",
                "onCustom not wired; setInteractionPolicy(...) or setCustomDoubleClickCallback(...) to enable");
            return;
        case O::NoOp:
            return;
        case O::BaseFallthrough:
            break;
        case O::DelegateToModifierPath:
            break;
        }
    }

    juce::Slider::mouseDoubleClick(e);
}

void UiArcKnob::setDoubleClickAction(DoubleClickAction action)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    policy_.doubleClick.action = action;
}

void UiArcKnob::setCustomDoubleClickCallback(std::function<void()> callback)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    policy_.doubleClick.onCustom = std::move(callback);
}

void UiArcKnob::setInteractionPolicy(InteractionPolicy newPolicy)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    policy_ = std::move(newPolicy);
}

void UiArcKnob::mouseMove(const juce::MouseEvent& e)
{
    juce::Slider::mouseMove(e);

    const bool wantsTrimPreview =
        kernel::isSecondaryAxisGesture(kernel::fromJuce(e.mods), hasConfiguredTrimCapability());

    if (!locked_ && wantsTrimPreview != isTrimPreviewMode_)
    {
        isTrimPreviewMode_ = wantsTrimPreview;
        repaint();
    }
}

void UiArcKnob::mouseEnter(const juce::MouseEvent& e)
{
    juce::Slider::mouseEnter(e);

    isTrimPreviewMode_ =
        !locked_ && kernel::isSecondaryAxisGesture(kernel::fromJuce(e.mods), hasConfiguredTrimCapability());

    if (showHoverGlow_ || isTrimPreviewMode_)
        repaint();
}

void UiArcKnob::mouseExit(const juce::MouseEvent& e)
{
    juce::Slider::mouseExit(e);

    // Clear trim preview when mouse exits
    isTrimPreviewMode_ = false;

    if (showHoverGlow_)
        repaint();
}

void UiArcKnob::modifierKeysChanged(const juce::ModifierKeys& modifiers)
{
    juce::Slider::modifierKeysChanged(modifiers);

    if (!locked_ && isMouseOver())
    {
        const bool wantsTrimPreview =
            kernel::isSecondaryAxisGesture(kernel::fromJuce(modifiers), hasConfiguredTrimCapability());

        if (wantsTrimPreview != isTrimPreviewMode_)
        {
            isTrimPreviewMode_ = wantsTrimPreview;
            repaint();
        }
    }
}

void UiArcKnob::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (locked_)
        return;
    const bool isTrimMode = kernel::isSecondaryAxisGesture(kernel::fromJuce(e.mods), hasConfiguredTrimCapability());
    const bool isFineTune = kernel::isFineGesture(kernel::fromJuce(e.mods));

    // Calculate delta: fine-tune = 5x slower adjustment
    const float sensitivity = isFineTune ? 0.002f : 0.01f;
    const float delta = wheel.deltaY * sensitivity;

    if (isTrimMode && onTrimValueChange)
    {
        // Adjust trim parameter
        float currentTrim = 0.5f; // Default to center
        if (getTrimValue)
        {
            // Convert trim dB (-12 to +12) to normalized (0-1)
            const float trimDb = getTrimValue();
            currentTrim = (trimDb + 12.0f) / 24.0f;
        }

        const float newTrim = juce::jlimit(0.0f, 1.0f, currentTrim + delta);
        onTrimValueChange(newTrim);
        repaint();
    }
    else
    {
        // Adjust main gain parameter
        const double currentNormalized = valueToProportionOfLength(getValue());
        const double newNormalized = juce::jlimit(0.0, 1.0, currentNormalized + static_cast<double>(delta));
        setValue(proportionOfLengthToValue(newNormalized));
    }
}

// =============================================================================
// Text Entry Mode
// =============================================================================

void UiArcKnob::showTrimTextEntry()
{
    if (locked_)
        return;
    // Allow parent to handle trim text entry if callback is set
    if (onTrimTextEntryRequested)
    {
        onTrimTextEntryRequested();
        return;
    }

    // Default implementation: show inline text editor for trim value
    if (!getTrimValue || !onTrimValueChange)
        return;

    createInlineEditor(true);
}

void UiArcKnob::showTextEntry()
{
    if (locked_)
        return;
    // Allow parent to handle text entry if callback is set
    if (onTextEntryRequested)
    {
        onTextEntryRequested();
        return;
    }

    // Default implementation: show inline text editor for gain value
    createInlineEditor(false);
}

void UiArcKnob::createInlineEditor(bool forTrim)
{
    static const auto kDefault = bws::ui::resolveTheme(bws::ui::defaultUiThemeBase(), {});
    const auto& wc = kDefault.weatherColors;

    // Remove any existing editor
    inlineEditor_.reset();

    // Get current value string
    juce::String currentValueStr;

    if (forTrim)
    {
        if (!getTrimValue)
            return;

        const float currentTrimDb = getTrimValue();
        if (currentTrimDb >= 0.0f)
            currentValueStr = "+" + juce::String(currentTrimDb, 1);
        else
            currentValueStr = juce::String(currentTrimDb, 1);
    }
    else
    {
        const double currentValue = getValue();
        if (currentValue <= getMinimum())
            currentValueStr = "-inf";
        else if (currentValue >= 0.0)
            currentValueStr = "+" + juce::String(currentValue, 1);
        else
            currentValueStr = juce::String(currentValue, 1);
    }

    inlineEditorIsForTrim_ = forTrim;

    // Create inline text editor
    inlineEditor_ = std::make_unique<juce::TextEditor>();
    inlineEditor_->setMultiLine(false);
    inlineEditor_->setReturnKeyStartsNewLine(false);
    inlineEditor_->setText(currentValueStr);
    inlineEditor_->selectAll();

    // Seamless integration with knob
    // Transparent background to let knob show through, subtle border only
    const juce::Colour textColour = forTrim ? wc.trimYellow : juce::Colours::white;
    const juce::Colour bgColour = wc.bgMain.withAlpha(bws::tokens::shared::opacity::arc_knob::INLINE_EDITOR_BG);
    const juce::Colour borderColour =
        arcColour_.withAlpha(bws::tokens::shared::opacity::arc_knob::INLINE_EDITOR_BORDER);

    inlineEditor_->setColour(juce::TextEditor::backgroundColourId, bgColour);
    inlineEditor_->setColour(juce::TextEditor::textColourId, textColour);
    inlineEditor_->setColour(juce::TextEditor::highlightColourId,
                             arcColour_.withAlpha(bws::tokens::shared::opacity::arc_knob::INLINE_EDITOR_HIGHLIGHT));
    inlineEditor_->setColour(juce::TextEditor::highlightedTextColourId, juce::Colours::white);
    inlineEditor_->setColour(juce::TextEditor::outlineColourId, borderColour);
    inlineEditor_->setColour(juce::TextEditor::focusedOutlineColourId, arcColour_);
    inlineEditor_->setColour(juce::CaretComponent::caretColourId, textColour);

    // Match the value display's themed bold face.
    inlineEditor_->setJustification(juce::Justification::centred);
    inlineEditor_->setFont(bw::Fonts::title(valueFontSize_ * 0.75f));

    // No scrollbars for minimal look
    inlineEditor_->setScrollbarsShown(false);

    // Position the editor over the value display area (center of knob)
    const auto bounds = getLocalBounds();
    const int editorWidth = 90;
    const int editorHeight = static_cast<int>(valueFontSize_ * 1.1f);
    const int editorX = (bounds.getWidth() - editorWidth) / 2;
    const int editorY = (bounds.getHeight() - editorHeight) / 2;

    inlineEditor_->setBounds(editorX, editorY, editorWidth, editorHeight);
    addAndMakeVisible(inlineEditor_.get());

    // Handle Return key to confirm, Escape to cancel
    inlineEditor_->onReturnKey = [this]() {
        handleInlineEditorResult(inlineEditor_->getText(), inlineEditorIsForTrim_);
        inlineEditor_.reset();
        repaint();
    };

    inlineEditor_->onEscapeKey = [this]() {
        inlineEditor_.reset();
        repaint();
    };

    // Also close if focus is lost
    inlineEditor_->onFocusLost = [this]() {
        // Only close on focus lost if the editor still exists
        if (inlineEditor_)
        {
            handleInlineEditorResult(inlineEditor_->getText(), inlineEditorIsForTrim_);
            inlineEditor_.reset();
            repaint();
        }
    };

    // Grab keyboard focus
    inlineEditor_->grabKeyboardFocus();
}

void UiArcKnob::handleInlineEditorResult(const juce::String& text, bool forTrim)
{
    const juce::String input = text.trim();
    if (input.isEmpty())
        return;

    if (forTrim)
    {
        // Parse the trim value (simpler parsing than main gain)
        juce::String valueStr = input.toLowerCase().replace("db", "").trim();
        if (valueStr.startsWithChar('+'))
            valueStr = valueStr.substring(1);

        const float parsedTrimDb = valueStr.getFloatValue();
        const float clampedTrimDb = juce::jlimit(-12.0f, 12.0f, parsedTrimDb);

        // Convert to normalized (0-1) and send to callback
        const float normalizedTrim = (clampedTrimDb + 12.0f) / 24.0f;
        if (onTrimValueChange)
            onTrimValueChange(normalizedTrim);
    }
    else
    {
        const double parsedValue = parseValueInput(input);
        const double clampedValue = juce::jlimit(getMinimum(), getMaximum(), parsedValue);
        setValue(clampedValue);
    }
}

double UiArcKnob::parseValueInput(const juce::String& input) const
{
    juce::String trimmed = input.trim().toLowerCase();

    // Handle -inf / infinity
    if (trimmed == "-inf" || trimmed == "inf" || trimmed == "-infinity")
    {
        return getMinimum();
    }

    // Handle amplitude multiplier: "2x" = +6dB, "0.5x" = -6dB
    if (trimmed.containsIgnoreCase("x"))
    {
        const float multiplier = trimmed.upToFirstOccurrenceOf("x", false, true).getFloatValue();
        if (multiplier > 0.0f)
        {
            return juce::Decibels::gainToDecibels(static_cast<double>(multiplier));
        }
        return 0.0; // Invalid multiplier
    }

    // Handle percentage: "50%" = halfway through range
    if (trimmed.contains("%"))
    {
        const float percent = trimmed.upToFirstOccurrenceOf("%", false, true).getFloatValue();
        const double range = getMaximum() - getMinimum();
        return getMinimum() + (range * percent / 100.0);
    }

    // Direct dB value (with or without "dB" suffix)
    juce::String valueStr = trimmed.replace("db", "").trim();

    // Handle leading + sign
    if (valueStr.startsWithChar('+'))
        valueStr = valueStr.substring(1);

    return valueStr.getDoubleValue();
}

// =============================================================================
// Context Menu
// =============================================================================

void UiArcKnob::showContextMenu(bool forTrim)
{
    if (locked_)
        return;
    juce::PopupMenu menu;

    if (forTrim && hasConfiguredTrimCapability())
    {
        // ===== TRIM CONTEXT MENU =====
        const float currentTrimDb = getTrimValue();
        juce::String trimValueStr = (currentTrimDb >= 0.0f) ? "+" + juce::String(currentTrimDb, 1) + " dB"
                                                            : juce::String(currentTrimDb, 1) + " dB";

        // Header showing current trim value
        menu.addItem(-1, "TRIM: " + trimValueStr, false, false);
        menu.addSeparator();

        // Reset trim to 0 dB
        menu.addItem(101, "Reset Trim to 0 dB");

        menu.addSeparator();

        // Copy/Paste trim
        menu.addItem(103, "Copy Trim Value");
        menu.addItem(104, "Paste Trim Value", juce::SystemClipboard::getTextFromClipboard().isNotEmpty());

        menu.addSeparator();

        // Enter trim value manually
        menu.addItem(105, "Enter Trim Value...");

        menu.addSeparator();

        // Help hints
        menu.addItem(-1, "Shift+Drag: Adjust Trim", false, false);
        menu.addItem(-1, "Cmd+Shift+Drag: Fine Trim", false, false);

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this).withMinimumWidth(180),
                           [this](int result) {
                               switch (result)
                               {
                               case 101: // Reset trim to 0 dB
                                   if (onTrimValueChange)
                                       onTrimValueChange(0.5f); // 0.5 normalized = 0 dB for -12 to +12 range
                                   repaint();
                                   break;

                               case 103: // Copy trim value
                                   if (getTrimValue)
                                   {
                                       const float trimDb = getTrimValue();
                                       juce::String valueStr =
                                           (trimDb >= 0.0f) ? "+" + juce::String(trimDb, 2) : juce::String(trimDb, 2);
                                       juce::SystemClipboard::copyTextToClipboard(valueStr);
                                   }
                                   break;

                               case 104: // Paste trim value
                               {
                                   const juce::String text = juce::SystemClipboard::getTextFromClipboard();
                                   if (text.isNotEmpty() && onTrimValueChange)
                                   {
                                       juce::String valueStr = text.trim().toLowerCase().replace("db", "").trim();
                                       if (valueStr.startsWithChar('+'))
                                           valueStr = valueStr.substring(1);
                                       const float parsedDb = valueStr.getFloatValue();
                                       const float clampedDb = juce::jlimit(-12.0f, 12.0f, parsedDb);
                                       const float normalized = (clampedDb + 12.0f) / 24.0f;
                                       onTrimValueChange(normalized);
                                       repaint();
                                   }
                               }
                               break;

                               case 105: // Enter trim value
                                   showTrimTextEntry();
                                   break;

                               default:
                                   break;
                               }
                           });
    }
    else
    {
        // ===== GAIN CONTEXT MENU (original) =====

        // Reset to 0 dB
        menu.addItem(1, "Reset to 0 dB");

        // Reset to default (if custom default is set)
        if (hasCustomDefault_ && std::abs(customDefaultValue_) > 0.001)
        {
            menu.addItem(2, "Reset to Default (" + juce::String(customDefaultValue_, 1) + " dB)");
        }

        menu.addSeparator();

        // Copy/Paste
        menu.addItem(3, "Copy Value");
        menu.addItem(4, "Paste Value", juce::SystemClipboard::getTextFromClipboard().isNotEmpty());

        menu.addSeparator();

        // Enter value manually
        menu.addItem(5, "Enter Value...");

        // Set as default (if callback is provided)
        if (onSetAsDefault)
        {
            menu.addSeparator();
            menu.addItem(6, "Set Current as Default");
        }

        // Show keyboard shortcuts
        menu.addSeparator();
        menu.addItem(-1, "Double-click: Reset", false, false);
        menu.addItem(-1, "Cmd+Drag: Fine Control (5x)", false, false);

        // Show trim mode info if available
        if (hasConfiguredTrimCapability())
        {
            menu.addItem(-1, "Shift+Drag: Adjust Trim", false, false);
            menu.addItem(-1, "Shift+Right-Click: Trim Menu", false, false);
        }

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this).withMinimumWidth(150),
                           [this](int result) {
                               switch (result)
                               {
                               case 1: // Reset to 0 dB
                                   setValue(0.0);
                                   break;

                               case 2: // Reset to custom default
                                   if (hasCustomDefault_)
                                       setValue(customDefaultValue_);
                                   break;

                               case 3: // Copy value
                                   copyValueToClipboard();
                                   break;

                               case 4: // Paste value
                                   pasteValueFromClipboard();
                                   break;

                               case 5: // Enter value
                                   showTextEntry();
                                   break;

                               case 6: // Set as default
                                   if (onSetAsDefault)
                                       onSetAsDefault(static_cast<float>(getValue()));
                                   break;

                               default:
                                   break;
                               }
                           });
    }
}

void UiArcKnob::copyValueToClipboard()
{
    const double value = getValue();
    juce::String valueStr;

    if (value <= getMinimum())
        valueStr = "-inf";
    else if (value >= 0.0)
        valueStr = "+" + juce::String(value, 2);
    else
        valueStr = juce::String(value, 2);

    juce::SystemClipboard::copyTextToClipboard(valueStr);
}

void UiArcKnob::pasteValueFromClipboard()
{
    const juce::String text = juce::SystemClipboard::getTextFromClipboard();
    if (text.isNotEmpty())
    {
        const double parsedValue = parseValueInput(text);
        const double clampedValue = juce::jlimit(getMinimum(), getMaximum(), parsedValue);
        setValue(clampedValue);
    }
}

void UiArcKnob::visibilityChanged()
{
    if (!isVisible())
        cancelUserInteraction();
    juce::Slider::visibilityChanged();
}

void UiArcKnob::parentHierarchyChanged()
{
    if (getParentComponent() == nullptr)
        cancelUserInteraction();
    juce::Slider::parentHierarchyChanged();
}

bool UiArcKnob::keyPressed(const juce::KeyPress& key)
{
    if (locked_ || !resolvedModel_.interactions.arrowKeyStep)
        return juce::Slider::keyPressed(key);

    const int keyCode = key.getKeyCode();
    const bool increment = keyCode == juce::KeyPress::upKey || keyCode == juce::KeyPress::rightKey;
    const bool decrement = keyCode == juce::KeyPress::downKey || keyCode == juce::KeyPress::leftKey;

    if (!increment && !decrement)
        return juce::Slider::keyPressed(key);

    const double baseStep = getInterval() > 0.0 ? getInterval() : (getMaximum() - getMinimum()) / 100.0;
    const double step =
        key.getModifiers().isShiftDown() && resolvedModel_.interactions.shiftArrowFineStep ? baseStep / 10.0 : baseStep;
    const double delta = increment ? step : -step;
    setValue(juce::jlimit(getMinimum(), getMaximum(), getValue() + delta), juce::sendNotificationAsync);
    return true;
}

} // namespace bws::ui

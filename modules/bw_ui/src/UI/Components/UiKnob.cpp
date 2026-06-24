// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/Components/UiKnob.h"
#include "bw_ui/Components/ControlStackMetrics.h"
#include "bw_ui/internal/UiMode.h"
#include "bw_ui/generated/BwTokens.h"
#include "bw_ui/interaction/InteractionLogging.h"
#include "bw_ui/interaction/ResetExecutor.h"
#include "bw_ui/kernel/DoubleClickDecision.h"
#include "bw_ui/adapters/FineDragModifier.h"

#include <variant>
#include <cmath>
#include <cstdlib>
#include <optional>

namespace bws::ui
{
namespace
{
struct SecondaryKnobResolvedStyle
{
    float warmth {1.0f};
    float depth {1.0f};
    float arcEnergy {1.0f};
    float readoutEmphasis {1.0f};
    float opticalCompression {1.0f};
    float valuePresence {0.0f};
    float responsiveLift {1.0f};
    float ringThicknessScale {1.0f};
    float arcThicknessScale {1.0f};
    float pointerLengthScale {1.0f};
    float pointerThicknessScale {1.0f};
    float topShineAlpha {0.0f};
    float lowerShadowAlpha {0.0f};
    float faceResponseAlpha {0.0f};
};

UiReadout::CompactAppearance resolveCompanionCompactAppearance(UiKnobRole role, UiReadoutRole readoutRole,
                                                               const kernel::ResolvedKnobModel& model) noexcept;

enum class EditableUnitHint
{
    None,
    Decibels,
    Hertz,
    KiloHertz,
    Milliseconds,
    Percent
};

struct EditableSeed
{
    juce::String text;
    EditableUnitHint unitHint {EditableUnitHint::None};
};

bool isPartialNumericInput(const juce::String& text) noexcept
{
    const auto trimmed = text.trim();
    return trimmed.isEmpty() || trimmed == "-" || trimmed == "+" || trimmed == "." || trimmed == "-." ||
           trimmed == "+.";
}

float resolveKnobDisabledVisualAlpha(const juce::Slider& sliderRef) noexcept
{
    const auto lockedVisual = static_cast<bool>(sliderRef.getProperties().getWithDefault("bws_locked_visual", false));
    if (!lockedVisual && sliderRef.isEnabled())
        return 1.0f;

    return juce::jlimit(0.0f, 1.0f, sliderRef.getAlpha());
}

std::optional<double> parseStrictDouble(const juce::String& text)
{
    const auto trimmed = text.trim();
    if (isPartialNumericInput(trimmed))
        return std::nullopt;

    const auto utf8 = trimmed.toStdString();
    char* end = nullptr;
    const double value = std::strtod(utf8.c_str(), &end);
    if (end == nullptr || end != utf8.c_str() + static_cast<long>(utf8.size()) || !std::isfinite(value))
        return std::nullopt;
    return value;
}

bool consumeSuffix(juce::String& text, const juce::String& suffix) noexcept
{
    auto trimmed = text.trimEnd();
    if (!trimmed.endsWithIgnoreCase(suffix))
        return false;

    trimmed = trimmed.dropLastCharacters(suffix.length()).trimEnd();
    text = trimmed;
    return true;
}

EditableUnitHint inferUnitHintFromDisplay(const juce::String& displayText) noexcept
{
    auto valueText = displayText.trim();
    if (consumeSuffix(valueText, "kHz"))
        return EditableUnitHint::KiloHertz;
    if (consumeSuffix(valueText, "Hz"))
        return EditableUnitHint::Hertz;
    if (consumeSuffix(valueText, "dB"))
        return EditableUnitHint::Decibels;
    if (consumeSuffix(valueText, "ms"))
        return EditableUnitHint::Milliseconds;
    if (consumeSuffix(valueText, "%"))
        return EditableUnitHint::Percent;
    return EditableUnitHint::None;
}

int decimalsFromInterval(double interval) noexcept
{
    if (interval <= 0.0)
        return 2;
    if (interval >= 1.0)
        return 0;
    if (interval >= 0.1)
        return 1;
    if (interval >= 0.01)
        return 2;
    return 3;
}

EditableSeed makeEditableSeedFromDisplay(const juce::String& displayText, double value, double interval)
{
    EditableSeed seed;
    auto stripped = displayText.trim();
    seed.unitHint = inferUnitHintFromDisplay(stripped);
    switch (seed.unitHint)
    {
    case EditableUnitHint::KiloHertz:
        consumeSuffix(stripped, "kHz");
        break;
    case EditableUnitHint::Hertz:
        consumeSuffix(stripped, "Hz");
        break;
    case EditableUnitHint::Decibels:
        consumeSuffix(stripped, "dB");
        break;
    case EditableUnitHint::Milliseconds:
        consumeSuffix(stripped, "ms");
        break;
    case EditableUnitHint::Percent:
        consumeSuffix(stripped, "%");
        break;
    case EditableUnitHint::None:
        break;
    }

    if (auto parsed = parseStrictDouble(stripped))
    {
        seed.text = stripped.trim();
        return seed;
    }

    const int decimals = decimalsFromInterval(interval);
    switch (seed.unitHint)
    {
    case EditableUnitHint::KiloHertz:
        seed.text = juce::String(value / 1000.0, juce::jmax(1, juce::jmin(3, decimals + 1)));
        break;
    default:
        seed.text = juce::String(value, decimals);
        break;
    }
    return seed;
}

std::optional<std::pair<double, EditableUnitHint>> parseEditableValue(const juce::String& candidate,
                                                                      EditableUnitHint fallbackHint)
{
    auto valueText = candidate.trim();
    if (isPartialNumericInput(valueText))
        return std::nullopt;

    EditableUnitHint explicitHint = EditableUnitHint::None;
    if (consumeSuffix(valueText, "kHz"))
        explicitHint = EditableUnitHint::KiloHertz;
    else if (consumeSuffix(valueText, "Hz"))
        explicitHint = EditableUnitHint::Hertz;
    else if (consumeSuffix(valueText, "dB"))
        explicitHint = EditableUnitHint::Decibels;
    else if (consumeSuffix(valueText, "ms"))
        explicitHint = EditableUnitHint::Milliseconds;
    else if (consumeSuffix(valueText, "%"))
        explicitHint = EditableUnitHint::Percent;

    auto trimmedValue = valueText.trim();
    if (trimmedValue.endsWithIgnoreCase("k"))
    {
        trimmedValue = trimmedValue.dropLastCharacters(1).trimEnd();
        valueText = trimmedValue;
        if (explicitHint == EditableUnitHint::None)
            explicitHint = EditableUnitHint::KiloHertz;
    }

    auto parsed = parseStrictDouble(valueText);
    if (!parsed)
        return std::nullopt;

    const auto effectiveHint = explicitHint == EditableUnitHint::None ? fallbackHint : explicitHint;
    double resolved = *parsed;
    if (effectiveHint == EditableUnitHint::KiloHertz)
        resolved *= 1000.0;
    else if (effectiveHint == EditableUnitHint::Percent)
        resolved /= 100.0;

    return std::make_pair(resolved, effectiveHint);
}

double snapToInterval(double value, double minimum, double maximum, double interval) noexcept
{
    const double clamped = juce::jlimit(minimum, maximum, value);
    if (interval <= 0.0)
        return clamped;

    const double steps = std::round((clamped - minimum) / interval);
    return juce::jlimit(minimum, maximum, minimum + steps * interval);
}

SecondaryKnobResolvedStyle resolveSecondaryKnobStyle(const UiThemeResolved& theme, float sizeRatio, float sliderPos,
                                                     bool interactive, bool dragging)
{
    SecondaryKnobResolvedStyle style;
    style.warmth = juce::jlimit(0.9f, 1.12f, theme.secondaryKnobStyle.warmth);
    style.depth = juce::jlimit(0.9f, 1.16f, theme.secondaryKnobStyle.depth);
    style.arcEnergy = juce::jlimit(0.95f, 1.16f, theme.secondaryKnobStyle.arcEnergy);
    style.readoutEmphasis = juce::jlimit(0.95f, 1.12f, theme.secondaryKnobStyle.readoutEmphasis);
    style.opticalCompression = std::pow(juce::jlimit(0.35f, 1.0f, sizeRatio), 0.85f);
    style.valuePresence = juce::jlimit(0.0f, 1.0f, std::pow(sliderPos, 0.8f));
    style.responsiveLift = (0.65f + style.valuePresence * 0.44f) * style.arcEnergy;
    style.ringThicknessScale = style.opticalCompression * 0.74f;
    style.arcThicknessScale = style.opticalCompression * 0.58f;
    style.pointerLengthScale = style.opticalCompression * 0.5f;
    style.pointerThicknessScale = style.opticalCompression * 0.55f;
    style.topShineAlpha = (interactive ? 0.085f : 0.06f) * (0.92f + style.valuePresence * 0.24f) * style.depth;
    style.lowerShadowAlpha = (interactive ? 0.18f : 0.13f) * style.depth;
    style.faceResponseAlpha =
        ((interactive ? 0.0425f : 0.0275f) * (0.85f + style.valuePresence * 0.35f) * style.depth) * 0.24f;
    juce::ignoreUnused(dragging);
    return style;
}

void fillEllipseWithVerticalGradient(juce::Graphics& g, const juce::Rectangle<float>& bounds, juce::Colour top,
                                     juce::Colour bottom)
{
    juce::ColourGradient gradient(top, bounds.getCentreX(), bounds.getY(), bottom, bounds.getCentreX(),
                                  bounds.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillEllipse(bounds);
}

void paintTopShine(juce::Graphics& g, const juce::Rectangle<float>& bounds, float alpha)
{
    if (alpha <= 0.0f)
        return;

    juce::Graphics::ScopedSaveState state(g);
    juce::Path clip;
    clip.addEllipse(bounds);
    g.reduceClipRegion(clip);

    juce::ColourGradient shine(juce::Colours::white.withAlpha(alpha), bounds.getCentreX(), bounds.getY(),
                               juce::Colours::transparentBlack, bounds.getCentreX(), bounds.getCentreY(), false);
    g.setGradientFill(shine);
    g.fillRect(bounds.withHeight(bounds.getHeight() * 0.5f));
}

void paintBottomShadow(juce::Graphics& g, const juce::Rectangle<float>& bounds, float alpha)
{
    if (alpha <= 0.0f)
        return;

    juce::Graphics::ScopedSaveState state(g);
    juce::Path clip;
    clip.addEllipse(bounds);
    g.reduceClipRegion(clip);

    juce::ColourGradient shadow(juce::Colours::transparentBlack, bounds.getCentreX(), bounds.getCentreY(),
                                juce::Colours::black.withAlpha(alpha), bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill(shadow);
    g.fillRect(bounds.withTrimmedTop(bounds.getHeight() * 0.4f));
}

void paintDirectionalFaceResponse(juce::Graphics& g, const juce::Rectangle<float>& bounds, float angle, float sizeRatio,
                                  float highlightAlpha, float shadowAlpha)
{
    const float resolvedStrength = juce::jmax(highlightAlpha, shadowAlpha);
    const float visibilityThreshold = juce::jmap(juce::jlimit(0.35f, 1.0f, sizeRatio), 0.35f, 1.0f, 0.0135f, 0.0065f);
    if (resolvedStrength <= visibilityThreshold)
        return;

    juce::Graphics::ScopedSaveState state(g);
    juce::Path clip;
    clip.addEllipse(bounds);
    g.reduceClipRegion(clip);

    const auto centre = bounds.getCentre();
    const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const juce::Point<float> lightDir(std::sin(angle), -std::cos(angle));
    const auto highlightCentre =
        centre.translated(lightDir.x * radius * 0.22f, lightDir.y * radius * 0.18f - radius * 0.08f);
    const auto shadowCentre =
        centre.translated(-lightDir.x * radius * 0.18f, -lightDir.y * radius * 0.16f + radius * 0.12f);

    if (highlightAlpha > 0.0f)
    {
        namespace opl = bws::tokens::shared::opacity::knob_lighting;
        juce::ColourGradient highlight(juce::Colours::white.withAlpha(highlightAlpha * opl::HIGHLIGHT_TOP_COEFF),
                                       highlightCentre.x, highlightCentre.y, juce::Colours::transparentBlack,
                                       highlightCentre.x, highlightCentre.y + radius * 1.45f, true);
        highlight.addColour(0.58, juce::Colours::white.withAlpha(highlightAlpha * opl::HIGHLIGHT_MID_COEFF));
        g.setGradientFill(highlight);
        g.fillEllipse(bounds.expanded(radius * 0.03f));
    }

    if (shadowAlpha > 0.0f)
    {
        namespace opl = bws::tokens::shared::opacity::knob_lighting;
        juce::ColourGradient shadow(juce::Colours::black.withAlpha(shadowAlpha * opl::SHADOW_TOP_COEFF), shadowCentre.x,
                                    shadowCentre.y, juce::Colours::transparentBlack, shadowCentre.x,
                                    shadowCentre.y - radius * 1.35f, true);
        shadow.addColour(0.6, juce::Colours::black.withAlpha(shadowAlpha * opl::SHADOW_MID_COEFF));
        g.setGradientFill(shadow);
        g.fillEllipse(bounds.expanded(radius * 0.03f));
    }
}

void paintEmbeddedKnobFace(juce::Graphics& g, const juce::Rectangle<float>& knobArea, juce::Colour rim,
                           juce::Colour rimHighlight, juce::Colour rimShadow, juce::Colour faceTop,
                           juce::Colour faceBottom, juce::Colour innerEdge, juce::Colour innerContour,
                           juce::Colour bowlShadow, juce::Colour bowlHighlight, juce::Colour seatShadow,
                           float ringThickness, float shineAlpha, float lowerShadowAlpha, float faceResponseAngle,
                           float faceResponseAlpha)
{
    const auto ringBounds = knobArea.reduced(bws::tokens::shared::geometry::STROKE_FULL_PX);
    const auto faceBounds = knobArea.reduced(juce::jmax(1.35f, ringThickness * 1.08f));
    const auto seatBounds = knobArea.translated(0.0f, juce::jmax(0.8f, ringThickness * 0.55f))
                                .reduced(juce::jmax(1.8f, ringThickness * 0.9f));
    g.setColour(seatShadow);
    g.fillEllipse(seatBounds);
    g.setColour(rim);
    g.drawEllipse(ringBounds, juce::jmax(0.8f, ringThickness));
    g.setColour(rimHighlight);
    g.drawEllipse(ringBounds.reduced(juce::jmax(0.55f, ringThickness * 0.45f)), 0.65f);
    g.setColour(rimShadow);
    g.drawEllipse(ringBounds.reduced(juce::jmax(1.1f, ringThickness * 0.9f)), 0.9f);
    fillEllipseWithVerticalGradient(g, faceBounds, faceTop, faceBottom);
    const auto bowlBounds = faceBounds.reduced(juce::jmax(2.1f, ringThickness * 1.45f));
    const auto coreBounds =
        bowlBounds.reduced(juce::jmax(static_cast<float>(bws::tokens::shared::spacing::XXS), ringThickness * 1.2f));
    g.setColour(bowlShadow);
    g.fillEllipse(bowlBounds);
    fillEllipseWithVerticalGradient(
        g, coreBounds, juce::Colours::transparentBlack,
        juce::Colours::black.withAlpha(bws::tokens::shared::opacity::knob_lighting::DROP_SHADOW));
    g.setColour(bowlHighlight);
    g.drawEllipse(bowlBounds.reduced(juce::jmax(0.9f, ringThickness * 0.7f)), 0.75f);
    g.setColour(innerEdge);
    g.drawEllipse(faceBounds, 0.9f);
    g.setColour(innerContour);
    g.drawEllipse(faceBounds.reduced(juce::jmax(1.2f, ringThickness * 1.1f)), 0.75f);
    paintDirectionalFaceResponse(
        g, faceBounds.reduced(juce::jmax(1.6f, ringThickness * 1.15f)), faceResponseAngle,
        juce::jlimit(0.35f, 1.0f, knobArea.getWidth() > 0.0f ? faceBounds.getWidth() / knobArea.getWidth() : 1.0f),
        faceResponseAlpha, faceResponseAlpha * 0.85f);
    paintTopShine(g, faceBounds, shineAlpha);
    paintBottomShadow(g, faceBounds, lowerShadowAlpha);
}
} // namespace

UiKnob::KnobSlider::KnobSlider()
{
    const auto geometry = kernel::resolveArcGeometry(UiKnobRole::Primary);
    setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    setMouseDragSensitivity(baseSensitivity);
    setVelocityBasedMode(false);
    setWantsKeyboardFocus(true);
    setRotaryParameters(geometry.startAngle, geometry.endAngle, true);
}

void UiKnob::KnobSlider::mouseDown(const juce::MouseEvent& e)
{
    if (locked || interactionSuppressed)
        return;

    const kernel::InteractionPolicy policy {
        .sensitivity = {.baseSensitivity = baseSensitivity, .fineSensitivity = fineSensitivity},
    };
    if (kernel::applySliderInteraction(*this, e.mods, false, policy, onRequestReset, onRequestTextEntry, nullptr))
        return;

    if (onDragChanged)
        onDragChanged(true);
    juce::Slider::mouseDown(e);
}

void UiKnob::KnobSlider::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (locked || interactionSuppressed)
        return;

    if (!e.mods.isAnyModifierKeyDown())
    {
        JUCE_ASSERT_MESSAGE_THREAD;
        const kernel::WiringState w {
            .hasTextEntryCallback = static_cast<bool>(policy_.doubleClick.onTextEntry),
            .hasResetMechanism = !std::holds_alternative<interaction::NoReset>(policy_.doubleClick.reset),
            .hasCustomCallback = static_cast<bool>(policy_.doubleClick.onCustom),
            .locked = false,
            .textEntryDelegates = true, // readout-state, not callback presence, decides TextEntry
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
        case O::AutoDegradeCustom:
            bws::ui::logAutoDegrade(
                "UiKnob", diag, "Custom",
                "onCustom not wired; setInteractionPolicy(...) or setCustomDoubleClickCallback(...) to enable");
            return;
        case O::NoOp:
            return;
        case O::DelegateToModifierPath:
            break; // fall through to kernel modifier routing below
        case O::AutoDegradeTextEntry:
        case O::BaseFallthrough:
            break;
        }
    }

    const kernel::InteractionPolicy policy {
        .sensitivity = {.baseSensitivity = baseSensitivity, .fineSensitivity = fineSensitivity},
    };
    const auto legacyReset = [this]() {
        if (auto* c = std::get_if<interaction::CallbackReset>(&policy_.doubleClick.reset))
        {
            if (c->callback)
                c->callback();
        }
        else if (onRequestReset)
        {
            onRequestReset();
        }
    };
    if (kernel::applySliderInteraction(*this, e.mods, true, policy, legacyReset, onRequestTextEntry, nullptr))
        return;

    juce::Slider::mouseDoubleClick(e);
}

void UiKnob::KnobSlider::mouseDrag(const juce::MouseEvent& e)
{
    if (locked || interactionSuppressed)
        return;

    const kernel::FineDragSensitivity sens {.baseSensitivity = baseSensitivity, .fineSensitivity = fineSensitivity};
    kernel::applyFineDragModifier(*this, e.mods, sens);
    juce::Slider::mouseDrag(e);

    if (static_cast<bool>(getProperties().getWithDefault("bws_center_detent_enabled", false)))
    {
        const double detentProp =
            static_cast<double>(getProperties().getWithDefault("bws_center_detent_position", 0.5));
        const double currentProp = valueToProportionOfLength(getValue());
        constexpr double kSnapRadius = 0.015;
        if (std::abs(currentProp - detentProp) < kSnapRadius)
            setValue(proportionOfLengthToValue(detentProp), juce::sendNotificationSync);
    }
}

void UiKnob::KnobSlider::mouseUp(const juce::MouseEvent& e)
{
    if (onDragChanged)
        onDragChanged(false);

    if (locked || interactionSuppressed)
        return;

    juce::Slider::mouseUp(e);
}

void UiKnob::KnobSlider::mouseEnter(const juce::MouseEvent& e)
{
    if (interactionSuppressed)
        return;

    if (onHoverChanged)
        onHoverChanged(true);
    juce::Slider::mouseEnter(e);
}

void UiKnob::KnobSlider::mouseExit(const juce::MouseEvent& e)
{
    if (interactionSuppressed)
        return;

    if (onHoverChanged)
        onHoverChanged(false);
    juce::Slider::mouseExit(e);
}

void UiKnob::KnobSlider::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (locked || interactionSuppressed || !resolvedModel.interactions.mouseWheel)
        return;

    juce::Slider::mouseWheelMove(e, wheel);
}

void UiKnob::KnobSlider::focusGained(FocusChangeType cause)
{
    if (onFocusChanged)
        onFocusChanged(true);
    juce::Slider::focusGained(cause);
}

void UiKnob::KnobSlider::focusLost(FocusChangeType cause)
{
    if (onFocusChanged)
        onFocusChanged(false);
    if (onDragChanged)
        onDragChanged(false);
    juce::Slider::focusLost(cause);
}

bool UiKnob::KnobSlider::keyPressed(const juce::KeyPress& key)
{
    if (locked || interactionSuppressed)
        return false;

    if (!resolvedModel.interactions.arrowKeyStep)
        return juce::Slider::keyPressed(key);

    const int keyCode = key.getKeyCode();
    const bool increment = keyCode == juce::KeyPress::upKey || keyCode == juce::KeyPress::rightKey;
    const bool decrement = keyCode == juce::KeyPress::downKey || keyCode == juce::KeyPress::leftKey;

    if (!increment && !decrement)
        return juce::Slider::keyPressed(key);

    const double baseStep = getInterval() > 0.0 ? getInterval() : (getMaximum() - getMinimum()) / 100.0;
    const double step =
        key.getModifiers().isShiftDown() && resolvedModel.interactions.shiftArrowFineStep ? baseStep / 10.0 : baseStep;
    const double delta = increment ? step : -step;
    setValue(juce::jlimit(getMinimum(), getMaximum(), getValue() + delta), juce::sendNotificationAsync);
    return true;
}

UiKnob::KnobLookAndFeel::KnobLookAndFeel(const UiThemeResolved* themeIn, UiKnobRole roleIn, float* scaleRef)
    : theme(themeIn)
    , role(roleIn)
    , scalePtr(scaleRef)
{}

void UiKnob::KnobLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                               float sliderPosProportional, float rotaryStartAngle,
                                               float rotaryEndAngle, juce::Slider& sliderRef)
{
    juce::ignoreUnused(sliderPosProportional);
    if (theme == nullptr || scalePtr == nullptr)
        return;

    const auto metrics = UiKnobs::metrics(role, *scalePtr, *theme);
    const auto geometry = kernel::resolveArcGeometry(role);
    auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height);
    const float avail = juce::jmin(bounds.getWidth(), bounds.getHeight());
    const float effectiveDiameter = juce::jmin(metrics.diameter, avail);
    const float scale = (metrics.diameter > 0.0f) ? effectiveDiameter / metrics.diameter : 1.0f;
    const bool clamped = metrics.diameter > avail;
    float ring = juce::jmax(1.0f, metrics.ringThickness * scale);
    float arc = juce::jmax(1.0f, metrics.arcThickness * scale);
    float pointerLength = metrics.pointerLength * scale;
    float pointerThickness = juce::jmax(1.0f, metrics.pointerThickness * scale);
    const float safeInset = clamped ? juce::jmax(1.5f, (ring * 0.5f + 1.5f) * scale) : 0.0f;
    const float drawDiameter = clamped ? juce::jmax(8.0f, effectiveDiameter - safeInset * 2.0f) : effectiveDiameter;
    const bool forceHover = static_cast<bool>(sliderRef.getProperties().getWithDefault("bws_force_hover", false));
    const bool forceActive = static_cast<bool>(sliderRef.getProperties().getWithDefault("bws_force_active", false));
    const float disabledVisualAlpha = resolveKnobDisabledVisualAlpha(sliderRef);
    const bool visuallyLocked = disabledVisualAlpha < 0.999f;
    const bool interactive = !visuallyLocked && (sliderRef.isMouseOverOrDragging() ||
                                                 sliderRef.hasKeyboardFocus(true) || forceHover || forceActive);
    const bool dragging = !visuallyLocked && sliderRef.isMouseButtonDown();

    auto centre = bounds.getCentre();
    auto knobArea = juce::Rectangle<float>(centre.x - drawDiameter * 0.5f, centre.y - drawDiameter * 0.5f, drawDiameter,
                                           drawDiameter);
    const float sizeRatio = juce::jlimit(0.35f, 1.0f, metrics.diameter > 0.0f ? drawDiameter / metrics.diameter : 1.0f);
    const float faceRadius = drawDiameter * 0.5f;
    const float trackRadius = juce::jmax(1.0f, faceRadius * geometry.familyTrackRadiusRatio);

    const float sliderPos = sliderRef.valueToProportionOfLength(sliderRef.getValue());
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    if (role == UiKnobRole::Secondary)
    {
        const auto& wc = theme->weatherColors;
        const auto secondaryStyle = resolveSecondaryKnobStyle(*theme, sizeRatio, sliderPos, interactive, dragging);
        const float warmth = secondaryStyle.warmth;
        const auto weatherBlend = [warmth](juce::Colour weather, juce::Colour neutral, float neutralAmount) {
            const float weatherAmount = juce::jlimit(0.0f, 1.0f, (1.0f - neutralAmount) * warmth);
            return neutral.interpolatedWith(weather, weatherAmount);
        };
        const float depth = secondaryStyle.depth;
        ring = juce::jmax(0.8f, ring * secondaryStyle.ringThicknessScale);
        arc = juce::jmax(0.7f, arc * secondaryStyle.arcThicknessScale);
        pointerLength *= secondaryStyle.pointerLengthScale;
        pointerThickness = juce::jmax(0.8f, pointerThickness * secondaryStyle.pointerThicknessScale);

        // =====================================================================
        // Procedural depth lighting - multiplicative alpha chain, NOT tokens.
        // The following ~18 withAlpha(...) calls are depth/shading math, not
        // semantic design decisions. Each per-state coefficient multiplies
        // with `depth` / `disabledVisualAlpha` / `valuePresence` to produce
        // sculpted lighting. Tokenizing would fragment the lighting chain
        // across unrelated tokens without enabling theme-swap semantics.
        // Keep this as procedural lighting rather than semantic tokens.
        // =====================================================================
        auto rim = weatherBlend(wc.borderLight, theme->colors.outline0, visuallyLocked ? 0.72f : 0.35f)
                       .withAlpha((interactive ? 0.76f : (visuallyLocked ? 0.42f : 0.6f)) *
                                  (visuallyLocked ? disabledVisualAlpha : 1.0f));
        auto rimHighlight = weatherBlend(wc.surfaceHighlight, theme->colors.text1, 0.74f)
                                .withAlpha((interactive ? 0.24f : (visuallyLocked ? 0.1f : 0.17f)) * depth *
                                           (visuallyLocked ? disabledVisualAlpha : 1.0f));
        auto rimShadow = weatherBlend(wc.panelNearBlack, theme->colors.bg0, 0.45f)
                             .withAlpha((interactive ? 0.34f : (visuallyLocked ? 0.18f : 0.28f)) * depth *
                                        (visuallyLocked ? disabledVisualAlpha : 1.0f));
        auto faceTop = weatherBlend(wc.bgRaised.interpolatedWith(wc.surfaceDark, 0.24f), theme->colors.bg1, 0.22f)
                           .withAlpha(visuallyLocked ? 0.94f * disabledVisualAlpha : 0.98f);
        auto faceBottom = weatherBlend(wc.bgMain, theme->colors.bg0, 0.32f)
                              .withAlpha(visuallyLocked ? 0.93f * disabledVisualAlpha : 0.985f);
        auto innerEdge = weatherBlend(wc.borderDark, theme->colors.outline0, 0.45f)
                             .withAlpha((interactive ? 0.34f : (visuallyLocked ? 0.14f : 0.24f)) *
                                        (visuallyLocked ? disabledVisualAlpha : 1.0f));
        auto innerContour = weatherBlend(wc.surfaceHighlight, theme->colors.bg2, 0.72f)
                                .withAlpha((interactive ? 0.18f : (visuallyLocked ? 0.08f : 0.13f)) *
                                           (0.92f + secondaryStyle.valuePresence * 0.18f) * depth *
                                           (visuallyLocked ? disabledVisualAlpha : 1.0f));
        auto bowlShadow = weatherBlend(wc.panelNearBlack, theme->colors.bg0, 0.4f)
                              .withAlpha((interactive ? 0.31f : (visuallyLocked ? 0.16f : 0.24f)) * depth *
                                         (visuallyLocked ? disabledVisualAlpha : 1.0f));
        auto bowlHighlight = weatherBlend(wc.surfaceHighlight, theme->colors.text0, 0.82f)
                                 .withAlpha((interactive ? 0.2f : (visuallyLocked ? 0.08f : 0.14f)) *
                                            (0.9f + secondaryStyle.valuePresence * 0.32f) * depth *
                                            (visuallyLocked ? disabledVisualAlpha : 1.0f));
        auto seatShadow =
            theme->colors.bg0.darker(0.18f).withAlpha((interactive ? 0.38f : (visuallyLocked ? 0.22f : 0.31f)) * depth *
                                                      (visuallyLocked ? disabledVisualAlpha : 1.0f));
        auto guideArc = weatherBlend(wc.surfaceDark, theme->colors.outline0, 0.55f)
                            .withAlpha((interactive ? 0.19f : (visuallyLocked ? 0.08f : 0.13f)) *
                                       (visuallyLocked ? disabledVisualAlpha : 1.0f));
        auto activeArc = weatherBlend(wc.accent, theme->colors.accent0, 0.68f)
                             .withAlpha(((dragging ? 0.8f : (interactive ? 0.7f : (visuallyLocked ? 0.24f : 0.59f))) *
                                         secondaryStyle.responsiveLift) *
                                        (visuallyLocked ? disabledVisualAlpha : 1.0f));

        paintEmbeddedKnobFace(g, knobArea, rim, rimHighlight, rimShadow, faceTop, faceBottom, innerEdge, innerContour,
                              bowlShadow, bowlHighlight, seatShadow, juce::jmax(0.8f, ring * 0.92f),
                              secondaryStyle.topShineAlpha, secondaryStyle.lowerShadowAlpha, angle,
                              secondaryStyle.faceResponseAlpha);

        juce::Path guidePath;
        guidePath.addCentredArc(centre.x, centre.y, trackRadius, trackRadius, 0.0f, rotaryStartAngle, rotaryEndAngle,
                                true);
        g.setColour(guideArc);
        g.strokePath(guidePath, juce::PathStrokeType(juce::jmax(0.65f, arc * 0.9f), juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));

        juce::Path activePath;
        activePath.addCentredArc(centre.x, centre.y, trackRadius, trackRadius, 0.0f, rotaryStartAngle, angle, true);
        const float activeArcStroke = juce::jmax(0.75f, arc * 0.92f);
        g.setColour(activeArc);
        g.strokePath(activePath, juce::PathStrokeType(activeArcStroke, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));

        const float markerAngle = angle - juce::MathConstants<float>::halfPi;
        const juce::Point<float> markerDir(std::cos(markerAngle), std::sin(markerAngle));
        const float markerLength = juce::jmax(1.5f, activeArcStroke * 1.7f);
        const float markerInnerRadius = trackRadius + activeArcStroke * 0.15f;
        const float markerOuterRadius = markerInnerRadius + markerLength;
        g.setColour(activeArc);
        g.drawLine(centre.x + markerDir.x * markerInnerRadius, centre.y + markerDir.y * markerInnerRadius,
                   centre.x + markerDir.x * markerOuterRadius, centre.y + markerDir.y * markerOuterRadius,
                   juce::jmax(activeArcStroke, 1.1f));
        return;
    }

    namespace opk = bws::tokens::shared::opacity::knob_chrome;
    const float baseAlpha = visuallyLocked ? disabledVisualAlpha : 1.0f;
    g.setColour(theme->colors.bg2.withAlpha((visuallyLocked ? opk::FACE_BG_LOCKED : opk::FACE_BG_IDLE) * baseAlpha));
    g.fillEllipse(knobArea);

    g.setColour(theme->colors.outline0.withAlpha((visuallyLocked ? opk::RING_LOCKED : opk::RING_IDLE) * baseAlpha));
    g.drawEllipse(knobArea, ring);

    juce::Path guidePath;
    guidePath.addCentredArc(centre.x, centre.y, trackRadius, trackRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(
        theme->colors.outline0.withAlpha((visuallyLocked ? opk::GUIDE_ARC_LOCKED : opk::GUIDE_ARC_IDLE) * baseAlpha));
    g.strokePath(guidePath, juce::PathStrokeType(juce::jmax(1.0f, arc * 0.85f), juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

    const bool centerDetentEnabled =
        static_cast<bool>(sliderRef.getProperties().getWithDefault("bws_center_detent_enabled", false));
    const float arcOriginProp =
        centerDetentEnabled
            ? static_cast<float>(sliderRef.getProperties().getWithDefault("bws_center_detent_position", 0.5))
            : 0.0f;
    const float arcOriginAngle = rotaryStartAngle + arcOriginProp * (rotaryEndAngle - rotaryStartAngle);
    juce::Path arcPath;
    arcPath.addCentredArc(centre.x, centre.y, trackRadius, trackRadius, 0.0f, arcOriginAngle, angle, true);
    g.setColour(
        theme->colors.accent0.withAlpha((visuallyLocked ? opk::ACTIVE_ARC_LOCKED : opk::ACTIVE_ARC_IDLE) * baseAlpha));
    g.strokePath(arcPath, juce::PathStrokeType(arc, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path pointer;
    pointer.addRectangle(-pointerThickness * 0.5f, -pointerLength, pointerThickness, pointerLength * 0.72f);
    pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
    g.setColour(
        theme->colors.accent1.withAlpha((visuallyLocked ? opk::POINTER_LOCKED : opk::POINTER_IDLE) * baseAlpha));
    g.fillPath(pointer);

    if (static_cast<bool>(sliderRef.getProperties().getWithDefault("bws_center_detent_enabled", false)))
    {
        const float detentProp =
            static_cast<float>(sliderRef.getProperties().getWithDefault("bws_center_detent_position", 0.5));
        // Round-cap visual overshoot: shift the tick to coincide with the
        // arc's perceived end.
        const float capOvershootAngle = (arc * 0.5f) / juce::jmax(1.0f, trackRadius);
        const float detentAngle =
            rotaryStartAngle + detentProp * (rotaryEndAngle - rotaryStartAngle) + capOvershootAngle;
        const float tickInner = trackRadius - arc * 0.6f;
        const float tickOuter = trackRadius + arc * 0.6f;
        const float sinA = std::sin(detentAngle);
        const float cosA = -std::cos(detentAngle);
        juce::Line<float> tick {centre.x + sinA * tickInner, centre.y + cosA * tickInner, centre.x + sinA * tickOuter,
                                centre.y + cosA * tickOuter};
        g.setColour(theme->colors.outline0.withAlpha((visuallyLocked ? 0.4f : 0.85f) * baseAlpha));
        g.drawLine(tick, juce::jmax(1.0f, arc * 0.35f));
    }
}

UiKnob::UiKnob(UiKnobRole roleIn, const UiThemeResolved& themeIn)
    : role(roleIn)
    , resolvedModel(kernel::resolveKnobModel(roleIn))
    , theme(themeIn)
    , kernelTheme(adapters::makeKernelThemeSnapshot(themeIn))
    , knobLnf(&theme, roleIn, &scale)
    , label(roleIn == UiKnobRole::Secondary ? UiTypographyRole::Annotation : UiTypographyRole::SectionLabel, themeIn)
    , readoutRole(roleIn == UiKnobRole::Hero ? UiReadoutRole::Standard : resolvedModel.companionReadoutRole)
    , readout(readoutRole, themeIn)
{
    label.setKernelTheme(kernelTheme);
    label.setTextRole(roleIn == UiKnobRole::Secondary ? kernel::TextRole::Annotation : kernel::TextRole::SectionLabel);
    slider.setResolvedKnobModel(resolvedModel);
    slider.setLookAndFeel(&knobLnf);
    addAndMakeVisible(slider);

    label.setUseSecondaryColour(roleIn != UiKnobRole::Secondary);
    addAndMakeVisible(label);

    readout.setUnit(unit);
    readout.setCompactAppearance(resolveCompanionCompactAppearance(role, readoutRole, resolvedModel));
    readout.setEditable(readout.isVisible() && role == UiKnobRole::Secondary &&
                        readout.getCompactAppearance() == UiReadout::CompactAppearance::CompanionCompact);
    addAndMakeVisible(readout);

    slider.onValueChange = [this]() {
        refreshReadout();
    };
    slider.onRequestReset = [this]() {
        resetToDefaultValue();
    };
    slider.onRequestTextEntry = [this]() {
        beginCompanionEditing();
    };
    slider.policy_.doubleClick.reset = interaction::CallbackReset {[this]() { resetToDefaultValue(); }};
    slider.policy_.doubleClick.onTextEntry = [this]() {
        beginCompanionEditing();
    };
    slider.onHoverChanged = [this](bool isHoveredNow) {
        sliderHovered = isHoveredNow;
        refreshInteractionState();
    };
    slider.onDragChanged = [this](bool draggingNow) {
        sliderDragging = draggingNow;
        refreshInteractionState();
    };
    slider.onFocusChanged = [this](bool focused) {
        sliderFocused = focused;
        refreshInteractionState();
    };
    readout.onHoverChanged = [this](bool isHoveredNow) {
        readoutHovered = isHoveredNow;
        refreshInteractionState();
    };
    readout.onRequestEditSeedText = [this]() {
        return makeEditableSeedText();
    };
    readout.onCommitEditText = [this](const juce::String& candidate, UiReadout::EditCommitTrigger trigger) {
        return commitEditedText(candidate, trigger);
    };
    readout.onEditingChanged = [this](bool editingNow) {
        readoutEditing = editingNow;
        if (!editingNow)
            activeEditSessionContext = {};
        refreshInteractionState();
    };
    refreshInteractionState();
}

UiKnob::~UiKnob()
{
    slider.setLookAndFeel(nullptr);
}

void UiKnob::setLabelText(const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
}

void UiKnob::setLabelVisible(bool shouldShow)
{
    showLabel = shouldShow;
    label.setVisible(shouldShow);
    resized();
}

void UiKnob::setUnit(ReadoutUnit newUnit)
{
    unit = newUnit;
    readout.setUnit(newUnit);
    refreshReadout();
}

void UiKnob::setDefaultValue(double value)
{
    defaultValue = value;
    slider.setDoubleClickReturnValue(false, value);
}

void UiKnob::setCenterDetent(bool enable, double centerProportion)
{
    auto& props = slider.getProperties();
    props.set("bws_center_detent_enabled", enable);
    props.set("bws_center_detent_position", juce::jlimit(0.0, 1.0, centerProportion));
    slider.repaint();
}

bool UiKnob::hasCenterDetent() const noexcept
{
    return static_cast<bool>(slider.getProperties().getWithDefault("bws_center_detent_enabled", false));
}

double UiKnob::getCenterDetentPosition() const noexcept
{
    return static_cast<double>(slider.getProperties().getWithDefault("bws_center_detent_position", 0.5));
}

void UiKnob::setScale(float newScale)
{
    scale = newScale;
    label.setScale(newScale);
    readout.setScale(newScale);
    repaint();
    resized();
}

void UiKnob::setTheme(const UiThemeResolved& newTheme)
{
    theme = newTheme;
    kernelTheme = adapters::makeKernelThemeSnapshot(newTheme);
    resolvedModel = kernel::resolveKnobModel(role, kernelTheme);
    slider.setResolvedKnobModel(resolvedModel);
    readoutRole = role == UiKnobRole::Hero ? UiReadoutRole::Standard : resolvedModel.companionReadoutRole;
    label.setTheme(newTheme);
    label.setKernelTheme(kernelTheme);
    label.setTextRole(role == UiKnobRole::Secondary ? kernel::TextRole::Annotation : kernel::TextRole::SectionLabel);
    label.setUseSecondaryColour(role != UiKnobRole::Secondary);
    readout.setTheme(newTheme);
    readout.setRole(readoutRole);
    readout.setCompactAppearance(resolveCompanionCompactAppearance(role, readoutRole, resolvedModel));
    readout.setEditable(readout.isVisible() && role == UiKnobRole::Secondary &&
                        readout.getCompactAppearance() == UiReadout::CompactAppearance::CompanionCompact);
    slider.repaint();
    refreshInteractionState();
    repaint();
}

void UiKnob::setFormatter(std::function<juce::String(double)> formatterFn)
{
    formatter = std::move(formatterFn);
    refreshReadout();
}

void UiKnob::setTooltip(const juce::String& text)
{
    slider.setTooltip(text);
}

void UiKnob::setDoubleClickAction(DoubleClickAction action)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    slider.policy_.doubleClick.action = action;
}

void UiKnob::setCustomDoubleClickCallback(std::function<void()> callback)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    slider.policy_.doubleClick.onCustom = std::move(callback);
}

void UiKnob::setInteractionPolicy(InteractionPolicy newPolicy)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    slider.policy_ = std::move(newPolicy);
}

const UiKnob::InteractionPolicy& UiKnob::getInteractionPolicy() const noexcept
{
    return slider.policy_;
}

UiKnob::DoubleClickAction UiKnob::getDoubleClickAction() const noexcept
{
    return slider.policy_.doubleClick.action;
}

void UiKnob::setLocked(bool shouldLock)
{
    locked = shouldLock;
    slider.setLocked(shouldLock);
    slider.getProperties().set("bws_locked_visual", shouldLock);
    slider.setAlpha(shouldLock ? 0.62f : 1.0f);
    label.setAlpha(shouldLock ? 0.72f : 1.0f);
    readout.setAlpha(shouldLock ? 0.78f : 1.0f);
    if (locked)
        readout.cancelEditing();
    refreshInteractionState();
    repaint();
}

void UiKnob::cancelUserInteraction()
{
    sliderDragging = false;
    readout.cancelEditing();
    refreshInteractionState();
}

void UiKnob::setDisplayValue(double newValue)
{
    if (!std::isfinite(newValue))
        return;
    slider.setValue(newValue, juce::dontSendNotification);
    refreshReadout();
}

void UiKnob::refreshReadout()
{
    if (formatter)
    {
        readout.setText(formatter(slider.getValue()));
        return;
    }

    switch (unit)
    {
    case ReadoutUnit::Decibels:
        readout.setValue((float)slider.getValue());
        break;
    case ReadoutUnit::Percent:
        readout.setValue((float)slider.getValue());
        break;
    case ReadoutUnit::Plain:
    default:
        readout.setValue((float)slider.getValue());
        break;
    }
}

void UiKnob::refreshInteractionState()
{
    const auto previousState = interactionState;
    if (readoutEditing)
        interactionState = InteractionState::Editing;
    else if (sliderDragging || sliderFocused)
        interactionState = InteractionState::Active;
    else if (sliderHovered || readoutHovered)
        interactionState = InteractionState::Hovered;
    else
        interactionState = InteractionState::Normal;

    slider.setInteractionSuppressed(readoutEditing);
    slider.getProperties().set("bws_force_hover", interactionState == InteractionState::Hovered);
    slider.getProperties().set("bws_force_active", interactionState == InteractionState::Active ||
                                                       interactionState == InteractionState::Editing);

    switch (interactionState)
    {
    case InteractionState::Hovered:
        readout.setEmphasis(UiReadout::Emphasis::Hovered);
        break;
    case InteractionState::Active:
    case InteractionState::Editing:
        readout.setEmphasis(UiReadout::Emphasis::Active);
        break;
    case InteractionState::Normal:
    default:
        readout.setEmphasis(UiReadout::Emphasis::Normal);
        break;
    }

    if (previousState != interactionState)
        slider.repaint();
}

namespace
{
UiReadout::CompactAppearance resolveCompanionCompactAppearance(UiKnobRole role, UiReadoutRole readoutRole,
                                                               const kernel::ResolvedKnobModel& model) noexcept
{
    const bool companionCompact = role == UiKnobRole::Secondary && readoutRole == UiReadoutRole::Compact &&
                                  model.readoutOwnership == kernel::KnobReadoutOwnership::ExternalCompanion;
    return companionCompact ? UiReadout::CompactAppearance::CompanionCompact
                            : UiReadout::CompactAppearance::DefaultCompact;
}
} // namespace

void UiKnob::resized()
{
    const auto bounds = getLocalBounds();
    if (bounds.getWidth() == 0 || bounds.getHeight() == 0)
        return;

    readoutRole = role == UiKnobRole::Hero ? UiReadoutRole::Standard : resolvedModel.companionReadoutRole;
    readout.setRole(readoutRole);
    readout.setCompactAppearance(resolveCompanionCompactAppearance(role, readoutRole, resolvedModel));
    readout.setEditable(readout.isVisible() && role == UiKnobRole::Secondary &&
                        readout.getCompactAppearance() == UiReadout::CompactAppearance::CompanionCompact);
    const auto metrics = UiKnobs::metrics(role, scale, theme);
    const auto readMetrics = UiReadouts::metrics(readoutRole, scale, theme);
    const auto stack = editor::knobStackLayout(role, scale, theme, kernelTheme, readoutRole, showLabel);
    // Hidden label/readout children release their reserved vertical space -
    // lets the rotary fill the slot in compact contexts (footer pill-row).
    const int labelHeight = label.isVisible() ? stack.labelHeight : 0;
    const int readoutHeight = readout.isVisible() ? stack.readoutHeight : 0;
    const int labelPadding = label.isVisible() ? stack.labelGap : 0;
    const int readoutPadding = readout.isVisible()
                                   ? juce::jmax(0, stack.totalHeight - stack.labelHeight - stack.labelGap -
                                                       stack.interactiveHeight - stack.readoutHeight)
                                   : 0;
    const int desiredKnob = stack.interactiveHeight;

    const int availHeight =
        juce::jmax(0, bounds.getHeight() - labelHeight - labelPadding - readoutHeight - readoutPadding);
    const int knobSide = juce::jmin(bounds.getWidth(), juce::jmin(availHeight, desiredKnob));

    auto labelArea = juce::Rectangle<int>(bounds.getX(), bounds.getY(), bounds.getWidth(), labelHeight);
    const int knobY = labelArea.getBottom() + labelPadding;
    auto knobBounds =
        juce::Rectangle<int>(0, 0, knobSide, knobSide).withCentre({bounds.getCentreX(), knobY + knobSide / 2});
    const int readoutY = knobBounds.getBottom() + readoutPadding;
    auto readoutArea = juce::Rectangle<int>(bounds.getX(), readoutY, bounds.getWidth(), readoutHeight);
    const int knobCenterX = knobBounds.getCentreX();

    if (layoutDebugEnabled() && (knobSide < desiredKnob || availHeight < desiredKnob))
    {
        juce::Logger::writeToLog(juce::String("[BWS_LAYOUT] UiKnob resized clamp role=") + juce::String((int)role) +
                                 " bounds=" + bounds.toString() + " labelH=" + juce::String(labelHeight) +
                                 " readoutH=" + juce::String(readoutHeight) +
                                 " availMidH=" + juce::String(availHeight) + " knobBounds=" + knobBounds.toString() +
                                 " desiredDia=" + juce::String(metrics.diameter, 2) +
                                 " effectiveDia=" + juce::String((double)knobSide, 2));
    }

    auto clampToBoundsCenter = [&](juce::Rectangle<int> area) {
        const int halfW = area.getWidth() / 2;
        const int minCenter = bounds.getX() + halfW;
        const int maxCenter = bounds.getRight() - halfW;
        const int centerX = juce::jlimit(minCenter, maxCenter, knobCenterX);
        return area.withCentre({centerX, area.getCentreY()});
    };

    label.setBounds(clampToBoundsCenter(labelArea));
    slider.setBounds(knobBounds);
    auto readoutBounds = readoutArea.withSizeKeepingCentre(
        (int)juce::jmax(readMetrics.minWidth, (float)readoutArea.getWidth()), readoutHeight);
    readout.setBounds(clampToBoundsCenter(readoutBounds));
}

juce::String UiKnob::makeEditableSeedText()
{
    if (!activeEditSessionContext.active)
        activeEditSessionContext = buildEditSessionContext();

    return activeEditSessionContext.seedText;
}

UiKnob::EditSessionContext UiKnob::buildEditSessionContext() const
{
    const auto displayText = readout.getText();
    const auto seed = makeEditableSeedFromDisplay(displayText, slider.getValue(), slider.getInterval());
    return EditSessionContext {displayText, seed.text, true};
}

bool UiKnob::commitEditedText(const juce::String& candidate, UiReadout::EditCommitTrigger trigger)
{
    juce::ignoreUnused(trigger);
    const auto context = activeEditSessionContext.active ? activeEditSessionContext : buildEditSessionContext();
    const auto seed = makeEditableSeedFromDisplay(context.displayText, slider.getValue(), slider.getInterval());
    const auto parsed = parseEditableValue(candidate, seed.unitHint);
    if (!parsed)
        return false;

    const double snapped =
        snapToInterval(parsed->first, slider.getMinimum(), slider.getMaximum(), slider.getInterval());
    slider.setValue(snapped, juce::sendNotificationSync);
    refreshReadout();
    return true;
}

void UiKnob::resetToDefaultValue()
{
    if (locked || readoutEditing)
        return;

    slider.setValue(defaultValue, juce::sendNotificationSync);
    refreshReadout();
}

void UiKnob::beginCompanionEditing()
{
    if (locked)
        return;

    // isVisible() is the local flag, headless-stable. Do NOT change to
    // isShowing() - that requires a peer + visible-ancestor chain absent
    // in headless tests.
    if (!readout.isVisible() || !readout.isEditable())
    {
#if JUCE_DEBUG
        juce::Logger::writeToLog("UiKnob '" + getName() +
                                 "': TextEntry dispatched to non-visible/non-editable readout. " +
                                 "Declare setDoubleClickAction(Reset|NoOp) on this knob to silence.");
        jassertfalse;
#endif
        return;
    }

    activeEditSessionContext = buildEditSessionContext();
    if (!readout.requestBeginEditing())
        activeEditSessionContext = {};
}

// Factory methods
std::unique_ptr<UiKnob> UiKnob::xs(UiKnobRole role, const UiThemeResolved& theme, const juce::String& label)
{
    auto knob = std::make_unique<UiKnob>(role, theme);
    knob->setSizeVariant(ComponentSize::XS);
    if (label.isNotEmpty())
        knob->setLabelText(label);
    return knob;
}

std::unique_ptr<UiKnob> UiKnob::sm(UiKnobRole role, const UiThemeResolved& theme, const juce::String& label)
{
    auto knob = std::make_unique<UiKnob>(role, theme);
    knob->setSizeVariant(ComponentSize::SM);
    if (label.isNotEmpty())
        knob->setLabelText(label);
    return knob;
}

std::unique_ptr<UiKnob> UiKnob::md(UiKnobRole role, const UiThemeResolved& theme, const juce::String& label)
{
    auto knob = std::make_unique<UiKnob>(role, theme);
    knob->setSizeVariant(ComponentSize::MD);
    if (label.isNotEmpty())
        knob->setLabelText(label);
    return knob;
}

std::unique_ptr<UiKnob> UiKnob::lg(UiKnobRole role, const UiThemeResolved& theme, const juce::String& label)
{
    auto knob = std::make_unique<UiKnob>(role, theme);
    knob->setSizeVariant(ComponentSize::LG);
    if (label.isNotEmpty())
        knob->setLabelText(label);
    return knob;
}

std::unique_ptr<UiKnob> UiKnob::xl(UiKnobRole role, const UiThemeResolved& theme, const juce::String& label)
{
    auto knob = std::make_unique<UiKnob>(role, theme);
    knob->setSizeVariant(ComponentSize::XL);
    if (label.isNotEmpty())
        knob->setLabelText(label);
    return knob;
}

void UiKnob::setSizeVariant(ComponentSize size)
{
    currentSize = size;
    applySizeVariant(size);
}

void UiKnob::applySizeVariant(ComponentSize size)
{
    const int diameter = editor::knobVariantDiameter(size);
    const int totalHeight = editor::knobVariantHeight(
        size, role, 1.0f, theme, kernelTheme,
        role == UiKnobRole::Hero ? UiReadoutRole::Standard : resolvedModel.companionReadoutRole, showLabel);
    Component::setSize(diameter, totalHeight);
}

} // namespace bws::ui

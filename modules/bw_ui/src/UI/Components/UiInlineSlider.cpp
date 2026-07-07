// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/Components/UiInlineSlider.h"
#include "bw_ui/generated/BwTokens.h"
#include "bw_ui/interaction/InteractionLogging.h"
#include "bw_ui/interaction/ResetExecutor.h"
#include "bw_ui/kernel/DoubleClickDecision.h"
#include "bw_ui/adapters/FineDragModifier.h"

#include <variant>

namespace bws::ui
{
namespace
{
constexpr float kHeroTrackHeight = 16.0f;
constexpr float kSupportingTrackHeight = 8.0f;
constexpr float kHeroLabelGap = 2.0f;
constexpr float kSupportingLabelGap = 0.0f;
constexpr uint32_t kSliderInkDeepest = 0xff0a0a0e;
constexpr uint32_t kSliderInkBase = 0xff0e0e14;
constexpr uint32_t kCoolSilverBlue = 0xff8090a0;
constexpr uint32_t kCoolGrey = 0xff606878;
constexpr uint32_t kWarmAmber = bws::tokens::shared::component::inline_slider::WARM_AMBER;
constexpr uint32_t kWarmBrass = 0xffd4a574;

float resolveDisabledVisualAlpha(const juce::Component& component) noexcept
{
    if (component.isEnabled())
        return 1.0f;

    return juce::jlimit(0.0f, 1.0f, component.getAlpha());
}

kernel::WiringState wiringStateFrom(const interaction::InteractionPolicy& policy) noexcept
{
    return {
        .hasTextEntryCallback = static_cast<bool>(policy.doubleClick.onTextEntry),
        .hasResetMechanism = !std::holds_alternative<interaction::NoReset>(policy.doubleClick.reset),
        .hasCustomCallback = static_cast<bool>(policy.doubleClick.onCustom),
        .locked = false,
        .textEntryDelegates = false,
    };
}
} // namespace

void UiInlineSlider::InputSlider::mouseDown(const juce::MouseEvent& e)
{
    // Sensitivity = visual width keeps cursor-thumb tracking 1:1 under
    // setSliderSnapsToMousePosition(false). Higher values decouple.
    const int w = juce::jmax(1, getWidth());
    const kernel::InteractionPolicy policy {
        .sensitivity = {.baseSensitivity = w, .fineSensitivity = w * 4},
    };
    const auto legacyReset = [this]() {
        if (auto* c = std::get_if<interaction::CallbackReset>(&policy_.doubleClick.reset))
            if (c->callback)
                c->callback();
    };

    if (kernel::applySliderInteraction(*this, e.mods, false, policy, legacyReset, policy_.doubleClick.onTextEntry,
                                       nullptr))
    {
        if (onVisualStateChanged)
            onVisualStateChanged();
        return;
    }

    juce::Slider::mouseDown(e);
    if (onVisualStateChanged)
        onVisualStateChanged();
}

void UiInlineSlider::InputSlider::mouseDrag(const juce::MouseEvent& e)
{
    const int w = juce::jmax(1, getWidth());
    const kernel::FineDragSensitivity sens {.baseSensitivity = w, .fineSensitivity = w * 4};
    kernel::applyFineDragModifier(*this, e.mods, sens);
    juce::Slider::mouseDrag(e);
}

void UiInlineSlider::InputSlider::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (!e.mods.isAnyModifierKeyDown())
    {
        JUCE_ASSERT_MESSAGE_THREAD;
        const auto outcome = kernel::decideBareDoubleClick(policy_.doubleClick.action, wiringStateFrom(policy_));
        const char* diag = policy_.diagnosticName ? policy_.diagnosticName->c_str() : getName().toRawUTF8();

        using O = kernel::BareDoubleClickOutcome;
        switch (outcome)
        {
        case O::InvokeTextEntry:
            policy_.doubleClick.onTextEntry();
            break;
        case O::InvokeReset:
            interaction::performReset(policy_.doubleClick.reset);
            break;
        case O::InvokeCustom:
            policy_.doubleClick.onCustom();
            break;
        case O::AutoDegradeTextEntry:
            bws::ui::logAutoDegrade(
                "UiInlineSlider", diag, "TextEntry",
                "onTextEntry not wired; setInteractionPolicy(...) or setTextEntryAction(...) to enable");
            break;
        case O::AutoDegradeCustom:
            bws::ui::logAutoDegrade(
                "UiInlineSlider", diag, "Custom",
                "onCustom not wired; setInteractionPolicy(...) or setCustomDoubleClickCallback(...) to enable");
            break;
        case O::BaseFallthrough:
            juce::Slider::mouseDoubleClick(e);
            break;
        case O::NoOp:
        case O::DelegateToModifierPath:
            break;
        }
        if (onVisualStateChanged)
            onVisualStateChanged();
        return;
    }

    // Modifier paths stay kernel-routed.
    const auto legacyReset = [this]() {
        if (auto* c = std::get_if<interaction::CallbackReset>(&policy_.doubleClick.reset))
            if (c->callback)
                c->callback();
    };
    if (kernel::applySliderInteraction(*this, e.mods, true, {}, legacyReset, policy_.doubleClick.onTextEntry, nullptr))
    {
        if (onVisualStateChanged)
            onVisualStateChanged();
        return;
    }

    juce::Slider::mouseDoubleClick(e);
    if (onVisualStateChanged)
        onVisualStateChanged();
}

void UiInlineSlider::InputSlider::mouseEnter(const juce::MouseEvent& e)
{
    juce::Slider::mouseEnter(e);
    if (onVisualStateChanged)
        onVisualStateChanged();
}

void UiInlineSlider::InputSlider::mouseExit(const juce::MouseEvent& e)
{
    juce::Slider::mouseExit(e);
    if (onVisualStateChanged)
        onVisualStateChanged();
}

void UiInlineSlider::InputSlider::mouseMove(const juce::MouseEvent& e)
{
    juce::Slider::mouseMove(e);
    if (onVisualStateChanged)
        onVisualStateChanged();
}

void UiInlineSlider::InputSlider::focusGained(FocusChangeType cause)
{
    juce::Slider::focusGained(cause);
    if (onVisualStateChanged)
        onVisualStateChanged();
}

void UiInlineSlider::InputSlider::focusLost(FocusChangeType cause)
{
    juce::Slider::focusLost(cause);
    if (onVisualStateChanged)
        onVisualStateChanged();
}

void UiInlineSlider::InputSlider::valueChanged()
{
    juce::Slider::valueChanged();
    if (onVisualStateChanged)
        onVisualStateChanged();
}

void UiInlineSlider::InputSlider::startedDragging()
{
    juce::Slider::startedDragging();
    if (onVisualStateChanged)
        onVisualStateChanged();
}

void UiInlineSlider::InputSlider::stoppedDragging()
{
    juce::Slider::stoppedDragging();
    if (onVisualStateChanged)
        onVisualStateChanged();
}

UiInlineSlider::UiInlineSlider(const UiThemeResolved& themeIn)
    : theme(themeIn)
    , kernelTheme(adapters::makeKernelThemeSnapshot(themeIn))
{
    slider.setLookAndFeel(&sliderLookAndFeel);
    slider.setSliderStyle(juce::Slider::LinearBar);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setSliderSnapsToMousePosition(false);
    slider.setWantsKeyboardFocus(true);
    slider.onVisualStateChanged = [this]() {
        repaint();
    };
    addAndMakeVisible(slider);
}

UiInlineSlider::~UiInlineSlider()
{
    slider.setLookAndFeel(nullptr);
}

void UiInlineSlider::setLabelText(const juce::String& text)
{
    labelText = text;
    repaint();
}

void UiInlineSlider::setLabelVisible(bool shouldShow)
{
    labelVisible = shouldShow;
    resized();
    repaint();
}

void UiInlineSlider::setFormatter(std::function<juce::String(double)> formatterFn)
{
    formatter = std::move(formatterFn);
    repaint();
}

void UiInlineSlider::setFormat(const kernel::FormatSpec& spec, double displayScale)
{
    setFormatter(
        [spec, displayScale](double value) { return juce::String(kernel::formatValue(value * displayScale, spec)); });
}

void UiInlineSlider::setTheme(const UiThemeResolved& newTheme)
{
    theme = newTheme;
    kernelTheme = adapters::makeKernelThemeSnapshot(newTheme);
    repaint();
}

void UiInlineSlider::setScale(float newScale)
{
    scale = newScale;
    resized();
    repaint();
}

void UiInlineSlider::setDefaultValue(double defaultValue)
{
    slider.setDoubleClickReturnValue(true, defaultValue);
}

void UiInlineSlider::setEmphasis(Emphasis newEmphasis)
{
    emphasis = newEmphasis;
    resized();
    repaint();
}

void UiInlineSlider::setModifierResetAction(std::function<void()> action)
{
    slider.policy_.doubleClick.reset = interaction::CallbackReset {std::move(action)};
}

void UiInlineSlider::setTextEntryAction(std::function<void()> action)
{
    slider.policy_.doubleClick.onTextEntry = std::move(action);
}

void UiInlineSlider::setDoubleClickAction(DoubleClickAction action)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    slider.policy_.doubleClick.action = action;
}

void UiInlineSlider::setCustomDoubleClickCallback(std::function<void()> callback)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    slider.policy_.doubleClick.onCustom = std::move(callback);
}

UiInlineSlider::DoubleClickAction UiInlineSlider::getDoubleClickAction() const noexcept
{
    return slider.policy_.doubleClick.action;
}

void UiInlineSlider::setInteractionPolicy(InteractionPolicy newPolicy)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    slider.policy_ = std::move(newPolicy);
}

const UiInlineSlider::InteractionPolicy& UiInlineSlider::getInteractionPolicy() const noexcept
{
    return slider.policy_;
}

juce::String UiInlineSlider::currentValueText() const
{
    if (formatter)
        return formatter(slider.getValue());

    return const_cast<InputSlider&>(slider).getTextFromValue(slider.getValue());
}

juce::Rectangle<int> UiInlineSlider::labelBounds() const
{
    auto bounds = getLocalBounds();
    if (!labelVisible || labelText.isEmpty() || emphasis != Emphasis::Hero)
        return {};

    const int labelHeight = (int)std::ceil(adapters::scaledFontSize(kernelTheme, kernel::TextRole::Annotation, scale));
    return bounds.removeFromTop(labelHeight);
}

float UiInlineSlider::compactLabelHeight() const
{
    if (!labelVisible || labelText.isEmpty() || emphasis != Emphasis::Supporting)
        return 0.0f;

    return std::ceil(adapters::scaledFontSize(kernelTheme, kernel::TextRole::Annotation, scale * 0.82f));
}

juce::Rectangle<int> UiInlineSlider::compactLabelBounds() const
{
    auto bounds = getLocalBounds();
    if (const auto height = compactLabelHeight(); height > 0.0f)
        return bounds.removeFromTop((int)height);

    return {};
}

float UiInlineSlider::trackHeight() const
{
    const float base = emphasis == Emphasis::Hero ? kHeroTrackHeight : kSupportingTrackHeight;
    return std::round(base * scale);
}

juce::Rectangle<float> UiInlineSlider::trackBounds() const
{
    auto bounds = getLocalBounds().toFloat();
    if (emphasis == Emphasis::Hero && labelVisible && !labelText.isEmpty())
        bounds.removeFromTop((float)labelBounds().getHeight() +
                             ((emphasis == Emphasis::Hero ? kHeroLabelGap : kSupportingLabelGap) * scale));
    else if (emphasis == Emphasis::Supporting)
        bounds.removeFromTop(compactLabelHeight());

    const float h = juce::jmin(trackHeight(), bounds.getHeight());
    const auto track = emphasis == Emphasis::Supporting ? bounds.withHeight(h).withY(bounds.getBottom() - h)
                                                        : bounds.withHeight(h).withCentre(bounds.getCentre());
    return track.reduced(
        emphasis == Emphasis::Hero ? theme.spacing.xs * scale * 0.65f : theme.spacing.xs * scale * 0.2f, 0.0f);
}

juce::Rectangle<int> UiInlineSlider::sliderHitBounds() const
{
    const auto track = trackBounds();
    const float padY = juce::jmax(4.0f, theme.spacing.xs * scale);
    return track.expanded(0.0f, padY).toNearestInt();
}

float UiInlineSlider::cornerRadius() const
{
    return emphasis == Emphasis::Hero ? theme.surfaces.radiusSm + (1.5f * scale)
                                      : theme.surfaces.radiusSm + (0.5f * scale);
}

void UiInlineSlider::paint(juce::Graphics& g)
{
    const float disabledVisualAlpha = resolveDisabledVisualAlpha(*this);
    const bool visuallyDisabled = disabledVisualAlpha < 0.999f;

    if (labelVisible && !labelText.isEmpty())
    {
        const auto drawBounds = emphasis == Emphasis::Hero ? labelBounds() : compactLabelBounds();
        const float labelScale = emphasis == Emphasis::Hero ? scale * 0.9f : scale * 0.82f;
        adapters::applyGraphicsFont(g, kernelTheme, kernel::TextRole::Annotation, labelScale);
        const float labelAlpha = visuallyDisabled ? disabledVisualAlpha * (emphasis == Emphasis::Hero ? 0.54f : 0.48f)
                                                  : (emphasis == Emphasis::Hero ? 0.38f : 0.34f);
        g.setColour(theme.colors.text1.withAlpha(labelAlpha));
        g.drawText(adapters::applyTextCasing(kernelTheme, kernel::TextRole::Annotation, labelText), drawBounds,
                   juce::Justification::centred, false);
    }

    const auto track = trackBounds();
    const bool hovered = !visuallyDisabled && (slider.isMouseOverOrDragging() || isMouseOver());
    const bool dragging = !visuallyDisabled && slider.isMouseButtonDown();
    const bool focused = !visuallyDisabled && slider.hasKeyboardFocus(true);
    const float radius = cornerRadius();

    const auto inkDeepest = juce::Colour(kSliderInkDeepest);
    const auto inkBase = juce::Colour(kSliderInkBase);
    const auto coolSilver = juce::Colour(kCoolSilverBlue);
    const auto coolGrey = juce::Colour(kCoolGrey);
    const auto warmAmber = juce::Colour(kWarmAmber);
    const auto warmBrass = juce::Colour(kWarmBrass);

    const bool energized = !visuallyDisabled && (dragging || focused);
    const auto laneBase = inkBase.interpolatedWith(inkDeepest, emphasis == Emphasis::Hero ? 0.50f : 0.62f);
    namespace opx = bws::tokens::shared::opacity::inline_slider;
    const auto laneTop =
        laneBase.brighter(visuallyDisabled ? 0.01f : (dragging ? 0.08f : 0.03f))
            .withAlpha(visuallyDisabled
                           ? disabledVisualAlpha * (emphasis == Emphasis::Hero ? opx::LANE_TOP_HERO_DISABLED
                                                                               : opx::LANE_TOP_SUPPORTING_DISABLED)
                           : (emphasis == Emphasis::Hero ? opx::LANE_TOP_HERO_IDLE : opx::LANE_TOP_SUPPORTING_IDLE));
    const auto laneBottom =
        laneBase.darker(visuallyDisabled ? 0.03f : 0.1f)
            .withAlpha(visuallyDisabled
                           ? disabledVisualAlpha * (emphasis == Emphasis::Hero ? opx::LABEL_HERO_DISABLED
                                                                               : opx::LABEL_SUPPORTING_DISABLED)
                           : (emphasis == Emphasis::Hero ? opx::LABEL_HERO_IDLE : opx::LABEL_SUPPORTING_IDLE));
    const auto coolBase = coolGrey.withAlpha(
        (visuallyDisabled ? opx::COOL_BASE_DISABLED
                          : (emphasis == Emphasis::Hero ? opx::COOL_BASE_HERO : opx::COOL_BASE_DEFAULT)) *
        (visuallyDisabled ? disabledVisualAlpha : 1.0f));
    const auto coolBright = coolSilver.withAlpha(
        (visuallyDisabled ? opx::COOL_BRIGHT_DISABLED
                          : (emphasis == Emphasis::Hero ? opx::COOL_BRIGHT_HERO : opx::COOL_BRIGHT_DEFAULT)) *
        (visuallyDisabled ? disabledVisualAlpha : 1.0f));
    const auto warmAccent = warmAmber.interpolatedWith(warmBrass, 0.28f);
    const auto outerGlow = energized
                               ? warmAccent.withAlpha(dragging ? opx::WARM_ACCENT_DRAGGING : opx::WARM_ACCENT_IDLE)
                               : juce::Colours::transparentBlack;
    const auto topHairline = coolSilver.withAlpha(
        (visuallyDisabled ? opx::TOP_HAIRLINE_DISABLED
                          : (emphasis == Emphasis::Hero ? opx::TOP_HAIRLINE_HERO : opx::TOP_HAIRLINE_DEFAULT)) *
        (visuallyDisabled ? disabledVisualAlpha : 1.0f));
    const auto lowerShade = inkDeepest.withAlpha(
        (visuallyDisabled ? opx::LOWER_SHADE_DISABLED
                          : (emphasis == Emphasis::Hero ? opx::LOWER_SHADE_HERO : opx::LOWER_SHADE_DEFAULT)) *
        (visuallyDisabled ? disabledVisualAlpha : 1.0f));

    juce::ColourGradient bg(laneTop, track.getX(), track.getY(), laneBottom, track.getX(), track.getBottom(), false);
    bg.addColour(0.52, laneBase);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(track, radius);

    if (outerGlow.getAlpha() > 0)
    {
        g.setColour(outerGlow);
        g.fillRoundedRectangle(track.expanded(0.5f, 0.5f), radius + 0.5f);
    }

    if (emphasis == Emphasis::Hero)
    {
        g.setColour(topHairline);
        g.drawHorizontalLine((int)std::round(track.getY()), track.getX() + radius * 0.7f,
                             track.getRight() - radius * 0.7f);
        g.setColour(lowerShade);
        g.drawHorizontalLine((int)std::round(track.getBottom() - 1.0f), track.getX() + radius * 0.8f,
                             track.getRight() - radius * 0.8f);
    }

    const float proportion = juce::jlimit(0.0f, 1.0f, (float)slider.valueToProportionOfLength(slider.getValue()));
    const auto laneInsetX = emphasis == Emphasis::Hero ? 8.0f * scale : 4.0f * scale;
    const auto laneInsetY = emphasis == Emphasis::Hero ? 4.0f * scale : 2.0f * scale;
    auto energyLane = track.reduced(laneInsetX, laneInsetY);
    const float fillWidth = juce::jmax(energyLane.getHeight() * 1.1f, energyLane.getWidth() * proportion);
    auto fill = energyLane.withWidth(fillWidth);
    if (emphasis == Emphasis::Supporting)
        fill = juce::Rectangle<float>(fill.getX(),
                                      energyLane.getCentreY() - juce::jmax(1.0f, energyLane.getHeight() * 0.18f),
                                      fill.getWidth(), juce::jmax(2.0f, energyLane.getHeight() * 0.36f));
    const auto fillLead =
        visuallyDisabled
            ? coolBase.withAlpha(emphasis == Emphasis::Hero ? opx::FILL_BASE_COOL_HERO : opx::FILL_BASE_COOL_DEFAULT)
        : energized ? warmAccent.withAlpha(dragging ? opx::FILL_BASE_WARM_DRAGGING : opx::FILL_BASE_WARM_DEFAULT)
                    : coolBase.withAlpha(emphasis == Emphasis::Hero ? opx::FILL_BASE_COOL_2_HERO
                                                                    : opx::FILL_BASE_COOL_2_DEFAULT);
    const auto fillTail =
        visuallyDisabled ? coolBright.withAlpha(emphasis == Emphasis::Hero ? opx::FILL_BRIGHT_COOL_HERO
                                                                           : opx::FILL_BRIGHT_COOL_DEFAULT)
        : energized ? warmBrass.withAlpha(dragging ? opx::FILL_BRIGHT_WARM_DRAGGING : opx::FILL_BRIGHT_WARM_DEFAULT)
                    : coolBright.withAlpha(emphasis == Emphasis::Hero ? opx::FILL_BRIGHT_COOL_2_HERO
                                                                      : opx::FILL_BRIGHT_COOL_2_DEFAULT);
    juce::ColourGradient fillGradient(fillLead, fill.getX(), fill.getCentreY(), fillTail, fill.getRight(),
                                      fill.getCentreY(), false);
    g.setGradientFill(fillGradient);
    g.fillRoundedRectangle(fill, juce::jmax(1.0f, fill.getHeight() * 0.5f));

    const float markerX = energyLane.getX() + (energyLane.getWidth() * proportion);
    const float markerH = energyLane.getHeight() + (emphasis == Emphasis::Hero ? 3.0f * scale : 1.0f * scale);
    const float markerY = track.getCentreY() - markerH * 0.5f;
    const auto markerColour =
        visuallyDisabled
            ? coolBright.withAlpha(emphasis == Emphasis::Hero ? opx::HANDLE_BG_COOL_HERO : opx::HANDLE_BG_COOL_DEFAULT)
        : energized ? warmBrass.withAlpha(focused ? opx::HANDLE_ACCENT_WARM_FOCUSED : opx::HANDLE_ACCENT_WARM_DEFAULT)
                    : coolBright.withAlpha(emphasis == Emphasis::Hero ? opx::HANDLE_ACCENT_COOL_HERO
                                                                      : opx::HANDLE_ACCENT_COOL_DEFAULT);
    g.setColour(markerColour);
    g.fillRoundedRectangle(juce::Rectangle<float>(markerX - 0.8f, markerY, 1.6f, markerH), 0.8f);

    const float readoutScale = emphasis == Emphasis::Hero ? scale * 0.92f : scale * 0.82f;
    adapters::applyGraphicsFont(g, kernelTheme, kernel::TextRole::Readout, readoutScale);
    const float valueAlpha = visuallyDisabled ? disabledVisualAlpha * (emphasis == Emphasis::Hero ? 0.52f : 0.46f)
                             : emphasis == Emphasis::Hero ? (dragging ? 0.96f : (hovered ? 0.90f : 0.84f))
                                                          : (dragging ? 0.88f : (hovered ? 0.82f : 0.76f));
    g.setColour(theme.colors.text0.withAlpha(valueAlpha));
    g.drawText(currentValueText(), track.reduced((int)std::round(theme.spacing.sm * scale * 0.8f), 0),
               juce::Justification::centred, false);
}

void UiInlineSlider::resized()
{
    slider.setBounds(sliderHitBounds());
    slider.setMouseDragSensitivity(juce::jmax(1, slider.getWidth()));
}

void UiInlineSlider::enablementChanged()
{
    repaint();
}

} // namespace bws::ui

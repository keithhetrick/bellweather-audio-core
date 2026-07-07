// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/weather/displays/ReadoutStrip.h"
#include "bw_ui/adapters/JuceTextMeasurer.h"
#include "bw_ui/adapters/UiThemeKernelAdapter.h"
#include "bw_ui/generated/BwTokens.h"
#include <bw_ui/interaction/InteractionLogging.h>
#include <bw_ui/adapters/FineDragModifier.h>
#include <bw_ui/kernel/DoubleClickDecision.h>
#include <bw_ui/kernel/InteractionContract.h>
#include <bw_ui/rendering/JuceRenderer.h>
#include <cmath>

namespace bws::weather
{

namespace
{
constexpr float kLockedAlpha = 0.55f;

bool isReadoutEditable(const bws::ui::kernel::AvailabilityState availability)
{
    return bws::ui::kernel::isInteractive(availability);
}

const bws::ui::UiThemeResolved& defaultTheme()
{
    static const auto kDefault = bws::ui::resolveTheme(bws::ui::defaultUiThemeBase(), {});
    return kDefault;
}

const bws::ui::kernel::ReadoutMetrics& activeReadoutMetrics(const bws::ui::kernel::ThemeSnapshot& theme)
{
    return theme.density == bws::ui::kernel::Density::Compact ? theme.readouts.compact : theme.readouts.standard;
}

float measureTextWidth(const juce::Font& font, const juce::String& text)
{
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(font, text, 0.0f, 0.0f);
    return glyphs.getBoundingBox(0, -1, true).getWidth();
}

bool shouldRunReadoutTimer(const ReadoutStrip& strip)
{
    return strip.isVisible() && strip.getParentComponent() != nullptr;
}
} // namespace

//==============================================================================
// CONSTRUCTION
//==============================================================================

ReadoutStrip::ReadoutStrip(juce::AudioProcessorValueTreeState& apvts)
    : apvts_(apvts)
    , kernelTheme_(bws::ui::adapters::makeKernelThemeSnapshot(defaultTheme()))
{
    setOpaque(false);

    // Create reveal button
    const auto& tc = theme_ ? *theme_ : defaultTheme();

    revealButton_ = std::make_unique<juce::TextButton>("ADVANCED");
    revealButton_->setButtonText(juce::CharPointer_UTF8("\xe2\x96\xbc ADVANCED")); // ▼ ADVANCED
    revealButton_->setColour(juce::TextButton::buttonColourId, tc.weatherColors.bgElevated);
    revealButton_->setColour(juce::TextButton::buttonOnColourId, tc.weatherColors.bgRaised);
    revealButton_->setColour(juce::TextButton::textColourOffId, tc.weatherColors.accent);
    revealButton_->setColour(juce::TextButton::textColourOnId, tc.weatherColors.accent.brighter(0.2f));
    revealButton_->setMouseCursor(juce::MouseCursor::PointingHandCursor);

    revealButton_->onClick = [this]() {
        if (onRevealClicked)
            onRevealClicked();
    };

    addAndMakeVisible(revealButton_.get());

    // Enable mouse wheel support
    setWantsKeyboardFocus(false);

    // Start timer for value updates (30Hz) only when visible in the hierarchy.
    if (shouldRunReadoutTimer(*this))
        startTimerHz(30);
}

ReadoutStrip::~ReadoutStrip()
{
    stopTimer();
    cancelInteraction();
    policy_.doubleClick.onCustom = nullptr;
}

//==============================================================================
// CONFIGURATION
//==============================================================================

void ReadoutStrip::addParameter(const juce::String& paramId, const juce::String& displayName,
                                bws::ui::kernel::FormatSpec spec, double displayScale)
{
    ParameterDisplay display;
    display.paramId = paramId;
    display.displayName = displayName;
    display.spec = spec;
    display.displayScale = displayScale;
    display.param = apvts_.getParameter(paramId);
    display.lastValue = display.param ? display.param->getValue() : 0.0f;

    if (display.param != nullptr)
    {
        display.adapter = std::make_unique<bws::ui::adapters::JuceParameterAdapter>(display.param);
        bws::ui::interaction::ValueInteractionConfig config;
        config.drag = {.baseSensitivity = 300, .fineSensitivity = 1200};
        config.wheelNormal = 0.01f;
        config.wheelFine = 0.002f;
        config.displayScale = displayScale;
        display.vi = std::make_unique<bws::ui::interaction::ValueInteraction>(
            *display.adapter, bws::ui::interaction::InteractionPolicy {}, spec, config);
    }

    parameters_.push_back(std::move(display));
    recomputeMaxPairWidth();
    repaint();
}

void ReadoutStrip::clearParameters()
{
    parameters_.clear();
    recomputeMaxPairWidth();
    repaint();
}

void ReadoutStrip::setRevealButtonText(const juce::String& text)
{
    if (revealButton_)
        revealButton_->setButtonText(text);
}

void ReadoutStrip::setRevealButtonVisible(bool visible)
{
    if (revealButton_)
    {
        revealButton_->setVisible(visible);
        resized();
        repaint();
    }
}

void ReadoutStrip::setTheme(const bws::ui::UiThemeResolved& t)
{
    theme_ = &t;
    kernelTheme_ = bws::ui::adapters::makeKernelThemeSnapshot(t);

    if (revealButton_)
    {
        revealButton_->setColour(juce::TextButton::buttonColourId, t.weatherColors.bgElevated);
        revealButton_->setColour(juce::TextButton::buttonOnColourId, t.weatherColors.bgRaised);
        revealButton_->setColour(juce::TextButton::textColourOffId, t.weatherColors.accent);
        revealButton_->setColour(juce::TextButton::textColourOnId, t.weatherColors.accent.brighter(0.2f));
    }

    recomputeMaxPairWidth();
    resized();
    repaint();
}

void ReadoutStrip::setScale(float scale)
{
    if (std::abs(scale_ - scale) > 0.001f)
    {
        scale_ = scale;
        recomputeMaxPairWidth();
        resized();
        repaint();
    }
}

void ReadoutStrip::setTextScaleMultiplier(float multiplier)
{
    const float clamped = juce::jlimit(0.6f, 1.0f, multiplier);
    if (std::abs(textScaleMultiplier_ - clamped) <= 0.001f)
        return;

    textScaleMultiplier_ = clamped;
    recomputeMaxPairWidth();
    repaint();
}

void ReadoutStrip::setItemLocked(int index, bool locked)
{
    if (index < 0 || index >= static_cast<int>(parameters_.size()))
        return;
    auto& item = parameters_[static_cast<size_t>(index)];
    const auto nextAvailability =
        locked ? bws::ui::kernel::AvailabilityState::Locked : bws::ui::kernel::AvailabilityState::Enabled;
    if (item.availability == nextAvailability)
        return;
    item.availability = nextAvailability;
    if (locked && draggingParameterIndex_ == index)
        cancelInteraction();
    repaint();
}

void ReadoutStrip::setItemTooltip(int index, const juce::String& tooltip)
{
    if (index < 0 || index >= static_cast<int>(parameters_.size()))
        return;
    parameters_[static_cast<size_t>(index)].tooltip = tooltip;
    if (hoveredParameterIndex_ == index)
        setTooltip(tooltip);
}

void ReadoutStrip::setItemDisplayOverride(int index, std::optional<float> realValue)
{
    if (index < 0 || index >= static_cast<int>(parameters_.size()))
        return;
    auto& display = parameters_[static_cast<size_t>(index)];
    if (display.displayOverride == realValue)
        return;
    display.displayOverride = realValue;
    repaint();
}

juce::String ReadoutStrip::formatItemValue(const ParameterDisplay& display) const
{
    if (display.displayOverride.has_value())
        return juce::String(bws::ui::kernel::formatValue(
            static_cast<double>(*display.displayOverride) * display.displayScale, display.spec));
    if (display.param == nullptr)
        return juce::String::fromUTF8("\xE2\x80\x94");
    const double real = display.param->convertFrom0to1(display.param->getValue()) * display.displayScale;
    return juce::String(bws::ui::kernel::formatValue(real, display.spec));
}

juce::String ReadoutStrip::getItemValueTextForTesting(int index) const
{
    if (index < 0 || index >= static_cast<int>(parameters_.size()))
        return {};
    return formatItemValue(parameters_[static_cast<size_t>(index)]);
}

float ReadoutStrip::dragSegmentForTesting(int index, int pixelsUp, bws::ui::kernel::ModSet mods)
{
    if (index < 0 || index >= static_cast<int>(parameters_.size()) || !parameters_[static_cast<size_t>(index)].vi)
        return 0.0f;
    auto& d = parameters_[static_cast<size_t>(index)];
    d.vi->pointerDown(mods, false);
    d.vi->pointerDrag(pixelsUp, mods);
    d.vi->pointerUp();
    return d.param->getValue();
}

float ReadoutStrip::wheelSegmentForTesting(int index, float deltaY, bws::ui::kernel::ModSet mods)
{
    if (index < 0 || index >= static_cast<int>(parameters_.size()) || !parameters_[static_cast<size_t>(index)].vi)
        return 0.0f;
    auto& d = parameters_[static_cast<size_t>(index)];
    d.vi->wheel(deltaY, mods);
    return d.param->getValue();
}

float ReadoutStrip::doubleClickSegmentForTesting(int index)
{
    if (index < 0 || index >= static_cast<int>(parameters_.size()) ||
        parameters_[static_cast<size_t>(index)].param == nullptr)
        return 0.0f;
    auto& d = parameters_[static_cast<size_t>(index)];
    bws::ui::adapters::JuceParameterAdapter adapter {d.param};
    bws::ui::kernel::setValueWithGesture(adapter, d.param->getDefaultValue()); // default action: Reset
    return d.param->getValue();
}

void ReadoutStrip::setItemCenterDetent(int index, bool enable, float position, float snapRadius)
{
    if (index < 0 || index >= static_cast<int>(parameters_.size()))
        return;
    auto& item = parameters_[static_cast<size_t>(index)];
    if (item.vi)
        item.vi->setDetent({enable, juce::jlimit(0.0f, 1.0f, position), juce::jmax(0.0f, snapRadius)});
}

void ReadoutStrip::cancelInteraction()
{
    if (draggingParameterIndex_ >= 0 && draggingParameterIndex_ < static_cast<int>(parameters_.size()))
    {
        auto& d = parameters_[static_cast<size_t>(draggingParameterIndex_)];
        if (d.vi)
            d.vi->pointerUp(); // drops any open gesture (RAII end)
    }

    draggingParameterIndex_ = -1;
    hoveredParameterIndex_ = -1;
    setMouseCursor(juce::MouseCursor::NormalCursor);
    setTooltip({});
    repaint();
}

int ReadoutStrip::getStripHeight() const
{
    return static_cast<int>(activeReadoutMetrics(kernelTheme_).stripHeight * scale_);
}

//==============================================================================
// TIMER CALLBACK
//==============================================================================

void ReadoutStrip::timerCallback()
{
    if (!shouldRunReadoutTimer(*this))
    {
        stopTimer();
        return;
    }

    // Check if any parameter values have changed
    bool changed = false;

    for (auto& display : parameters_)
    {
        if (display.param)
        {
            float currentValue = display.param->getValue();
            if (std::abs(currentValue - display.lastValue) > 0.0001f)
            {
                display.lastValue = currentValue;
                changed = true;
            }
        }
    }

    if (changed)
        repaint();
}

//==============================================================================
// COMPONENT OVERRIDES
//==============================================================================

void ReadoutStrip::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    paintBackground(g, bounds);

    // Calculate readout area (left portion, excluding button if visible)
    auto readoutBounds = bounds;
    if (revealButton_ && revealButton_->isVisible())
    {
        const auto& metrics = activeReadoutMetrics(kernelTheme_);
        readoutBounds.removeFromRight((metrics.buttonWidth + kernelTheme_.spacing.sm) * scale_);
    }

    paintParameterValues(g, readoutBounds);
}

void ReadoutStrip::paintBackground(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    const auto& t = theme_ ? *theme_ : defaultTheme();

    bws::ui::rendering::JuceRenderer renderer(g);
    const float cornerRadius = t.weatherColors.radiusMd * scale_;

    // Background with brass gradient
    auto bgH = renderer.createLinearGradient(bounds.getX(), bounds.getY(), bounds.getX(), bounds.getBottom());
    renderer.addGradientStop(bgH, 0.0f, t.weatherColors.bgElevated.brighter(0.02f).getARGB());
    renderer.addGradientStop(bgH, 1.0f, t.weatherColors.bgRaised.getARGB());
    renderer.applyGradient(bgH);
    renderer.fillRoundedRect(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), cornerRadius);
    renderer.destroyGradient(bgH);

    // Brass border
    const auto borderBounds = bounds.reduced(bws::tokens::shared::geometry::STROKE_HALF_PX);
    renderer.setColour(t.weatherColors.accent.withAlpha(bws::tokens::shared::opacity::outline::HAIRLINE).getARGB());
    renderer.drawRoundedRect(borderBounds.getX(), borderBounds.getY(), borderBounds.getWidth(),
                             borderBounds.getHeight(), cornerRadius, 1.0f * scale_);

    // Top brass accent line
    renderer.setColour(t.weatherColors.accent.withAlpha(bws::tokens::shared::opacity::strip::BORDER_DEFAULT).getARGB());
    renderer.fillRect(bounds.getX() + cornerRadius, bounds.getY(), bounds.getWidth() - cornerRadius * 2, 1.0f * scale_);
}

void ReadoutStrip::paintParameterValues(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    if (parameters_.empty())
        return;

    // Padding
    const auto& metrics = activeReadoutMetrics(kernelTheme_);
    float padding = metrics.paddingX * scale_;
    bounds = bounds.reduced(padding, 0.0f);

    // Calculate segment width for each parameter
    float segmentWidth = bounds.getWidth() / static_cast<float>(parameters_.size());

    // Font setup: fit to actual segment width, not just global editor scale.
    const auto& t = theme_ ? *theme_ : defaultTheme();
    auto stripFont =
        bws::ui::adapters::makeFont(kernelTheme_, bws::ui::kernel::TextRole::Readout, scale_ * textScaleMultiplier_);

    // cachedMaxPairWidth_ is recomputed by recomputeMaxPairWidth() whenever
    // parameters_ / scale_ / theme / textScaleMultiplier_ changes. It must
    // stay in sync with the formatDisplayValue lambda above - both evaluate
    // through the same null/formatter/percent/default cascade.
    const float availableTextWidth = juce::jmax(1.0f, segmentWidth - (6.0f * scale_));
    if (cachedMaxPairWidth_ > availableTextWidth)
    {
        const float fitScale = juce::jlimit(0.68f, 1.0f, availableTextWidth / cachedMaxPairWidth_);
        stripFont = stripFont.withHeight(stripFont.getHeight() * fitScale);
    }
    g.setFont(stripFont);

    float x = bounds.getX();

    for (size_t i = 0; i < parameters_.size(); ++i)
    {
        auto& display = parameters_[i];
        juce::Rectangle<float> segment(x, bounds.getY(), segmentWidth, bounds.getHeight());
        juce::Graphics::ScopedSaveState segmentState(g);

        // Cache bounds for hit testing (interactive readouts)
        display.bounds = segment;

        // Check if this parameter is being hovered/dragged
        bool isHovered = (static_cast<int>(i) == hoveredParameterIndex_);
        bool isDragging = (static_cast<int>(i) == draggingParameterIndex_);

        // Draw hover/drag highlight background
        const bool isLocked = display.availability == bws::ui::kernel::AvailabilityState::Locked;
        if (interactiveEnabled_ && isReadoutEditable(display.availability) && (isHovered || isDragging))
        {
            juce::Colour highlightColor =
                isDragging ? t.weatherColors.accent.withAlpha(bws::tokens::shared::opacity::strip::SELECTED_FILL)
                           : t.weatherColors.accent.withAlpha(bws::tokens::shared::opacity::strip::NORMAL_FILL);
            g.setColour(highlightColor);
            g.fillRoundedRectangle(segment.reduced(bws::tokens::shared::spacing::XXS, bws::tokens::shared::spacing::XS),
                                   4.0f * scale_);
        }

        // Get formatted value
        const juce::String valueText = formatItemValue(display);

        // Draw label - brighter when hovered/dragging unless locked
        g.setColour((isHovered || isDragging)
                        ? t.weatherColors.textPrimary.withAlpha(bws::tokens::shared::opacity::strip::SELECTED_TEXT)
                        : t.weatherColors.textSecondary);
        if (isLocked)
            g.setOpacity(kLockedAlpha);
        juce::String labelText = display.displayName + ":";

        // Calculate text widths using the fitted strip font.
        const float labelWidth = measureTextWidth(stripFont, labelText);

        // Draw label
        g.setFont(stripFont);
        g.drawText(labelText, segment.withWidth(labelWidth + 4.0f * scale_), juce::Justification::centredLeft);

        // Draw value.
        g.setColour((isHovered || isDragging) ? t.weatherColors.accent : t.weatherColors.textPrimary);
        g.setFont(stripFont);
        auto valueArea = segment;
        valueArea.removeFromLeft(labelWidth + 2.0f * scale_);
        g.drawText(valueText, valueArea, juce::Justification::centredLeft);

        // Draw separator line (except for last parameter)
        if (i < parameters_.size() - 1)
        {
            float sepX = x + segmentWidth - 1.0f * scale_;
            g.setColour(t.weatherColors.borderLight.withAlpha(bws::tokens::shared::opacity::strip::SEPARATOR));
            g.drawVerticalLine(static_cast<int>(sepX), bounds.getY() + bounds.getHeight() * 0.25f,
                               bounds.getY() + bounds.getHeight() * 0.75f);
        }

        x += segmentWidth;
    }
}

void ReadoutStrip::recomputeMaxPairWidth()
{
    cachedMaxPairWidth_ = 0.0f;
    if (parameters_.empty())
        return;

    const float roleScale = scale_ * textScaleMultiplier_;
    const auto stripFont = bws::ui::adapters::makeFont(kernelTheme_, bws::ui::kernel::TextRole::Readout, roleScale);
    const bws::ui::adapters::JuceTextMeasurer valueMeasurer(kernelTheme_);
    const float gap = 4.0f * scale_;

    for (const auto& display : parameters_)
    {
        const juce::String labelText = display.displayName + ":";
        const float labelWidth = measureTextWidth(stripFont, labelText);

        double minReal = 0.0;
        double maxReal = 0.0;
        if (display.param != nullptr)
        {
            const double a = display.param->convertFrom0to1(0.0f) * display.displayScale;
            const double b = display.param->convertFrom0to1(1.0f) * display.displayScale;
            minReal = juce::jmin(a, b);
            maxReal = juce::jmax(a, b);
        }
        const float valueWidth = bws::ui::kernel::reservedValueWidth(display.spec, minReal, maxReal, valueMeasurer,
                                                                     bws::ui::kernel::TextRole::Readout, roleScale);

        cachedMaxPairWidth_ = juce::jmax(cachedMaxPairWidth_, labelWidth + gap + valueWidth);
    }
}

void ReadoutStrip::resized()
{
    auto bounds = getLocalBounds();
    const auto& metrics = activeReadoutMetrics(kernelTheme_);

    // Button on right side
    int buttonWidth = static_cast<int>(metrics.buttonWidth * scale_);
    int buttonHeight = static_cast<int>((metrics.stripHeight - kernelTheme_.spacing.sm) * scale_);
    int buttonPadding = static_cast<int>(kernelTheme_.spacing.sm * scale_);

    auto buttonArea = bounds.removeFromRight(buttonWidth + buttonPadding);
    buttonArea = buttonArea.reduced(buttonPadding / bws::tokens::shared::spacing::XXS,
                                    (bounds.getHeight() - buttonHeight) / bws::tokens::shared::spacing::XXS +
                                        buttonPadding / bws::tokens::shared::spacing::XXS);

    if (revealButton_)
        revealButton_->setBounds(buttonArea.getX(), bounds.getY() + (getHeight() - buttonHeight) / 2, buttonWidth,
                                 buttonHeight);
}

//==============================================================================
// MOUSE INTERACTION (Pro-Level Interactive Readouts)
//==============================================================================

int ReadoutStrip::getParameterIndexAt(juce::Point<float> point) const
{
    if (!interactiveEnabled_)
        return -1;

    for (size_t i = 0; i < parameters_.size(); ++i)
    {
        if (parameters_[i].bounds.contains(point))
            return static_cast<int>(i);
    }

    return -1;
}

void ReadoutStrip::setHoveredParameter(int index)
{
    if (hoveredParameterIndex_ != index)
    {
        hoveredParameterIndex_ = index;

        // Update cursor
        if (index >= 0 && interactiveEnabled_)
        {
            const auto& param = parameters_[static_cast<size_t>(index)];
            setMouseCursor(isReadoutEditable(param.availability) ? juce::MouseCursor::UpDownResizeCursor
                                                                 : juce::MouseCursor::NormalCursor);

            // Editor-owned tooltip takes precedence when provided.
            if (param.tooltip.isNotEmpty())
            {
                setTooltip(param.tooltip);
            }
            else
            {
                if (isReadoutEditable(param.availability))
                {
                    setTooltip(param.displayName + "\n"
                                                   "• Drag up/down: Adjust\n"
                                                   "• Shift+Drag: Fine tune\n"
                                                   "• Scroll: Adjust\n"
                                                   "• Double-click: Reset");
                }
                else
                {
                    setTooltip(param.displayName + "\n"
                                                   "• Locked");
                }
            }
        }
        else
        {
            setMouseCursor(juce::MouseCursor::NormalCursor);
            setTooltip({});
        }

        repaint();
    }
}

void ReadoutStrip::mouseDown(const juce::MouseEvent& e)
{
    if (!interactiveEnabled_)
        return;

    int index = getParameterIndexAt(e.position);
    if (index >= 0)
    {
        auto& display = parameters_[static_cast<size_t>(index)];
        if (!isReadoutEditable(display.availability))
            return;

        if (e.mods.isAltDown() && display.param)
        {
            bws::ui::adapters::JuceParameterAdapter adapter {display.param};
            bws::ui::kernel::setValueWithGesture(adapter, display.param->getDefaultValue());
            repaint();
            return;
        }

        draggingParameterIndex_ = index;
        dragStartY_ = e.y;
        if (display.vi)
            display.vi->pointerDown(bws::ui::kernel::fromJuce(e.mods), false);

        repaint();
    }
}

void ReadoutStrip::mouseDrag(const juce::MouseEvent& e)
{
    if (draggingParameterIndex_ < 0 || !interactiveEnabled_)
        return;

    auto& display = parameters_[static_cast<size_t>(draggingParameterIndex_)];
    if (display.vi && isReadoutEditable(display.availability))
        display.vi->pointerDrag(dragStartY_ - e.y, bws::ui::kernel::fromJuce(e.mods));
}

void ReadoutStrip::mouseUp(const juce::MouseEvent& /*e*/)
{
    if (draggingParameterIndex_ >= 0)
    {
        auto& display = parameters_[static_cast<size_t>(draggingParameterIndex_)];
        if (display.vi)
            display.vi->pointerUp();
        draggingParameterIndex_ = -1;
        repaint();
    }
}

void ReadoutStrip::mouseMove(const juce::MouseEvent& e)
{
    int index = getParameterIndexAt(e.position);
    setHoveredParameter(index);
}

void ReadoutStrip::mouseExit(const juce::MouseEvent& /*e*/)
{
    setHoveredParameter(-1);
}

void ReadoutStrip::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (!interactiveEnabled_)
        return;

    int index = getParameterIndexAt(e.position);
    if (index >= 0)
    {
        auto& display = parameters_[static_cast<size_t>(index)];
        if (!isReadoutEditable(display.availability) || !display.vi)
            return;
        display.vi->wheel(wheel.deltaY, bws::ui::kernel::fromJuce(e.mods));
    }
}

void ReadoutStrip::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (!interactiveEnabled_)
        return;

    int index = getParameterIndexAt(e.position);
    if (index < 0 || index >= static_cast<int>(parameters_.size()))
        return;

    auto& display = parameters_[static_cast<size_t>(index)];
    if (display.param == nullptr || !isReadoutEditable(display.availability))
        return;

    JUCE_ASSERT_MESSAGE_THREAD;
    // hasResetMechanism is unconditional: the reset target is the param
    // under the cursor, resolved at dispatch time.
    const bws::ui::kernel::WiringState w {
        .hasTextEntryCallback = false,
        .hasResetMechanism = true,
        .hasCustomCallback = static_cast<bool>(policy_.doubleClick.onCustom),
    };
    const auto outcome = bws::ui::kernel::decideBareDoubleClick(policy_.doubleClick.action, w);
    const char* diag = policy_.diagnosticName ? policy_.diagnosticName->c_str() : getName().toRawUTF8();

    using O = bws::ui::kernel::BareDoubleClickOutcome;
    switch (outcome)
    {
    case O::InvokeReset:
    {
        bws::ui::adapters::JuceParameterAdapter adapter {display.param};
        bws::ui::kernel::setValueWithGesture(adapter, display.param->getDefaultValue());
        return;
    }
    case O::InvokeCustom:
        policy_.doubleClick.onCustom();
        return;
    case O::AutoDegradeTextEntry:
        bws::ui::logAutoDegrade("ReadoutStrip", diag, "TextEntry", "ReadoutStrip has no inline editor");
        return;
    case O::AutoDegradeCustom:
        bws::ui::logAutoDegrade(
            "ReadoutStrip", diag, "Custom",
            "onCustom not wired; setInteractionPolicy(...) or setCustomDoubleClickCallback(...) to enable");
        return;
    case O::NoOp:
    case O::InvokeTextEntry:
    case O::BaseFallthrough:
    case O::DelegateToModifierPath:
        return;
    }
}

void ReadoutStrip::setDoubleClickAction(DoubleClickAction action)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    policy_.doubleClick.action = action;
}

void ReadoutStrip::setCustomDoubleClickCallback(std::function<void()> callback)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    policy_.doubleClick.onCustom = std::move(callback);
}

void ReadoutStrip::setInteractionPolicy(InteractionPolicy newPolicy)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    policy_ = std::move(newPolicy);
}

void ReadoutStrip::visibilityChanged()
{
    if (!isVisible())
        cancelInteraction();
    if (shouldRunReadoutTimer(*this))
        startTimerHz(30);
    else
        stopTimer();
    juce::Component::visibilityChanged();
}

void ReadoutStrip::parentHierarchyChanged()
{
    if (getParentComponent() == nullptr)
        cancelInteraction();
    if (shouldRunReadoutTimer(*this))
        startTimerHz(30);
    else
        stopTimer();
    juce::Component::parentHierarchyChanged();
}

} // namespace bws::weather

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

/**
 * =============================================================================
 * WeatherSegmentedControl.cpp - Segmented Mode Selector Implementation
 * =============================================================================
 *
 * Bellweather Studios - Weather Instrument Design System
 *
 * Horizontal segmented control with selected segment highlighting.
 * Uses Brass design tokens for consistent visual language.
 *
 * =============================================================================
 */

#include "bw_ui/weather/controls/WeatherSegmentedControl.h"
#include "bw_ui/weather/controls/WeatherControlSurface.h"
#include "bw_ui/adapters/JuceParameterAdapter.h"
#include "bw_ui/adapters/UiThemeKernelAdapter.h"
#include "bw_ui/kernel/ParameterGestureScope.h"
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

struct SegmentLayout
{
    juce::Rectangle<float> bounds;
    bool roundLeft = false;
    bool roundRight = false;
};

std::vector<SegmentLayout> computeSegmentLayout(const juce::Rectangle<float>& bounds, int count)
{
    std::vector<SegmentLayout> layout;
    if (count <= 0)
        return layout;

    layout.reserve(static_cast<size_t>(count));
    const float widthPerSegment = bounds.getWidth() / static_cast<float>(count);

    for (int index = 0; index < count; ++index)
    {
        auto segmentBounds = juce::Rectangle<float>(
            bounds.getX() + (static_cast<float>(index) * widthPerSegment), bounds.getY(),
            index == count - 1 ? bounds.getRight() - (bounds.getX() + static_cast<float>(index) * widthPerSegment)
                               : widthPerSegment,
            bounds.getHeight());

        layout.push_back({segmentBounds, index == 0, index == count - 1});
    }

    return layout;
}

control_surface::ControlVisualState makeSegmentState(const WeatherSegmentedControl& control, int index,
                                                     int selectedIndex, int pressedIndex, int hoveredIndex)
{
    juce::ignoreUnused(control, index);
    return {control.isEnabled(), index == hoveredIndex, index == pressedIndex, index == selectedIndex,
            control.hasKeyboardFocus(true)};
}
} // namespace

// =============================================================================
// WeatherSegmentedControl Implementation
// =============================================================================

WeatherSegmentedControl::WeatherSegmentedControl()
    : kernelTheme_(bws::ui::adapters::makeKernelThemeSnapshot(defaultTheme()))
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus(true);
}

void WeatherSegmentedControl::addSegment(const juce::String& label)
{
    segments_.push_back(label);
    repaint();
}

void WeatherSegmentedControl::clearSegments()
{
    segments_.clear();
    selectedIndex_ = 0;
    repaint();
}

void WeatherSegmentedControl::setSelectedIndex(int index, bool notify)
{
    if (index >= 0 && index < static_cast<int>(segments_.size()) && index != selectedIndex_)
    {
        selectedIndex_ = index;
        repaint();
        if (notify && onSelectionChanged)
            onSelectionChanged(selectedIndex_);
    }
}

void WeatherSegmentedControl::setScaleFactor(float scale)
{
    scaleFactor_ = scale;
    repaint();
}

void WeatherSegmentedControl::setTheme(const bws::ui::UiThemeResolved& t)
{
    theme_ = &t;
    kernelTheme_ = bws::ui::adapters::makeKernelThemeSnapshot(t);
    repaint();
}

void WeatherSegmentedControl::paint(juce::Graphics& g)
{
    const auto& t = theme_ ? *theme_ : defaultTheme();

    if (segments_.empty())
        return;

    const auto bounds = getLocalBounds().toFloat();
    const auto metrics = control_surface::resolveSelectorMetrics(t, scaleFactor_);
    const auto containerState = control_surface::ControlVisualState {isEnabled(), hoveredIndex_ >= 0,
                                                                     pressedIndex_ >= 0, false, hasKeyboardFocus(true)};
    const auto containerPalette = control_surface::resolveSelectorPalette(t, containerState);
    const auto segmentLayout = computeSegmentLayout(bounds, static_cast<int>(segments_.size()));

    control_surface::paintContainer(g, bounds, metrics, containerPalette);

    for (int index = 0; index < static_cast<int>(segments_.size()); ++index)
    {
        const auto state = makeSegmentState(*this, index, selectedIndex_, pressedIndex_, hoveredIndex_);
        const auto palette = control_surface::resolveSelectorPalette(t, state);
        const auto& segment = segmentLayout[static_cast<size_t>(index)];

        if (state.selected || state.hovered || state.pressed)
        {
            auto path = control_surface::makeJoinedRoundedPath(segment.bounds, metrics.cornerRadius, segment.roundLeft,
                                                               segment.roundRight);
            g.setColour(palette.fill);
            g.fillPath(path);
        }

        if (index < static_cast<int>(segments_.size()) - 1)
        {
            g.setColour(palette.divider);
            g.drawLine(segment.bounds.getRight(), segment.bounds.getY() + metrics.dividerInset,
                       segment.bounds.getRight(), segment.bounds.getBottom() - metrics.dividerInset,
                       metrics.borderWidth);
        }

        g.setColour(palette.text);
        g.setFont(bws::ui::adapters::makeFont(kernelTheme_, bws::ui::kernel::TextRole::ControlText, scaleFactor_));
        g.drawText(segments_[static_cast<size_t>(index)], segment.bounds, juce::Justification::centred);
    }

    control_surface::paintFocusRing(g, bounds, metrics, containerPalette);
}

void WeatherSegmentedControl::mouseDown(const juce::MouseEvent& e)
{
    if (!isEnabled() || !e.mods.isLeftButtonDown())
        return;
    grabKeyboardFocus();
    pressedIndex_ = getSegmentIndexAt(e.getPosition());
    repaint();
}

void WeatherSegmentedControl::mouseUp(const juce::MouseEvent& e)
{
    const int clickedIndex = getSegmentIndexAt(e.getPosition());
    if (clickedIndex == pressedIndex_ && clickedIndex >= 0)
    {
        setSelectedIndex(clickedIndex);
    }
    pressedIndex_ = -1;
    repaint();
}

void WeatherSegmentedControl::mouseMove(const juce::MouseEvent& e)
{
    const int newHover = getSegmentIndexAt(e.getPosition());
    if (newHover != hoveredIndex_)
    {
        hoveredIndex_ = newHover;
        repaint();
    }
}

void WeatherSegmentedControl::mouseExit(const juce::MouseEvent& /*e*/)
{
    if (hoveredIndex_ != -1)
    {
        hoveredIndex_ = -1;
        repaint();
    }
}

bool WeatherSegmentedControl::keyPressed(const juce::KeyPress& key)
{
    if (!isEnabled())
        return false;

    if (key == juce::KeyPress::spaceKey || key == juce::KeyPress::returnKey)
    {
        if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(segments_.size()))
        {
            setSelectedIndex(selectedIndex_);
            return true;
        }
    }

    return false;
}

int WeatherSegmentedControl::getSegmentIndexAt(juce::Point<int> pos) const
{
    if (segments_.empty() || !getLocalBounds().contains(pos))
        return -1;

    const float segmentWidth = static_cast<float>(getWidth()) / static_cast<float>(segments_.size());
    return juce::jlimit(0, static_cast<int>(segments_.size()) - 1,
                        static_cast<int>(static_cast<float>(pos.x) / segmentWidth));
}

// =============================================================================
// SegmentedControlAttachment Implementation
// =============================================================================

SegmentedControlAttachment::SegmentedControlAttachment(juce::AudioProcessorValueTreeState& apvts,
                                                       const juce::String& paramID, WeatherSegmentedControl& control)
    : apvts_(apvts)
    , paramID_(paramID)
    , control_(control)
{
    // Initial sync from parameter to control
    if (auto* param = apvts_.getParameter(paramID_))
    {
        control_.setSelectedIndex(static_cast<int>(std::round(param->convertFrom0to1(param->getValue()))), false);
    }

    // Listen for parameter changes
    apvts_.addParameterListener(paramID_, this);

    // Listen for control changes
    control_.onSelectionChanged = [this](int newIndex) {
        if (ignoreCallback_.load())
            return;
        if (auto* param = apvts_.getParameter(paramID_))
        {
            bws::ui::adapters::JuceParameterAdapter adapter {param};
            bws::ui::kernel::setValueWithGesture(adapter, param->convertTo0to1(static_cast<float>(newIndex)));
        }
    };
}

SegmentedControlAttachment::~SegmentedControlAttachment()
{
    apvts_.removeParameterListener(paramID_, this);
}

void SegmentedControlAttachment::parameterChanged(const juce::String&, float newValue)
{
    // newValue is already denormalized by JUCE's APVTS listener (convertFrom0to1
    // is applied internally before calling this callback). For Choice parameters,
    // newValue is the float choice index (0.0, 1.0, 2.0, ...) with possible
    // floating-point imprecision, so we round to nearest int directly.
    ignoreCallback_.store(true);
    control_.setSelectedIndex(static_cast<int>(std::round(newValue)), false);
    ignoreCallback_.store(false);
}

} // namespace bws::weather

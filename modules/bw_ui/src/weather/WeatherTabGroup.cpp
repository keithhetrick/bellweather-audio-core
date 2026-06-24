// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include <bw_ui/weather/controls/WeatherTabGroup.h>
#include <bw_ui/weather/controls/WeatherControlSurface.h>
#include <bw_ui/adapters/UiThemeKernelAdapter.h>
#include <bw_ui/generated/BwTokens.h>

namespace bws::weather
{
namespace
{
const bws::ui::UiThemeResolved& defaultTheme()
{
    static const auto kDefault = bws::ui::resolveTheme(bws::ui::defaultUiThemeBase(), {});
    return kDefault;
}

bws::ui::kernel::ThemeSnapshot makeKernelTheme(const bws::ui::UiThemeResolved& theme)
{
    return bws::ui::adapters::makeKernelThemeSnapshot(theme);
}

juce::Rectangle<float> insetTabBounds(const juce::Rectangle<float>& bounds)
{
    return bounds.reduced(bws::tokens::shared::geometry::STROKE_FULL_PX, bws::tokens::shared::geometry::STROKE_FULL_PX);
}

control_surface::ControlVisualState makeTabState(const WeatherTabGroup& tabs, int index, int selectedIndex,
                                                 int pressedIndex, int hoveredIndex)
{
    return {tabs.isEnabled(), index == hoveredIndex, index == pressedIndex, index == selectedIndex,
            tabs.hasKeyboardFocus(true)};
}
} // namespace

WeatherTabGroup::WeatherTabGroup(Orientation orientation)
    : orientation_(orientation)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus(true);
}

int WeatherTabGroup::addTab(const juce::String& label)
{
    tabLabels_.push_back(label);

    if (tabLabels_.size() == 1)
        setSelectedTab(0, false);

    resized();
    repaint();
    return static_cast<int>(tabLabels_.size()) - 1;
}

void WeatherTabGroup::clearTabs()
{
    tabLabels_.clear();
    selectedIndex_ = -1;
    pressedIndex_ = -1;
    hoveredIndex_ = -1;
    repaint();
}

void WeatherTabGroup::setSelectedTab(int index, bool notify)
{
    if (index < 0 || index >= static_cast<int>(tabLabels_.size()))
        return;

    selectedIndex_ = index;
    repaint();

    if (notify && onTabChanged)
        onTabChanged(index);
}

void WeatherTabGroup::setTabSize(int width, int height)
{
    tabWidth_ = width;
    tabHeight_ = height;
    resized();
}

void WeatherTabGroup::setTabGap(int gap)
{
    tabGap_ = gap;
    resized();
}

void WeatherTabGroup::setScale(float scale)
{
    scale_ = scale;
    repaint();
}

juce::Font WeatherTabGroup::tabLabelFont() const
{
    const auto& theme = theme_ ? *theme_ : defaultTheme();
    const auto kernelTheme = makeKernelTheme(theme);
    return bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::ControlText, scale_);
}

juce::Font WeatherTabGroup::paintFontForTesting() const
{
    return tabLabelFont();
}

juce::Rectangle<int> WeatherTabGroup::getIdealBounds() const
{
    if (tabLabels_.empty())
        return {};

    const int count = static_cast<int>(tabLabels_.size());
    if (orientation_ == Orientation::Horizontal)
        return {0, 0, count * tabWidth_ + (count - 1) * tabGap_, tabHeight_};

    return {0, 0, tabWidth_, count * tabHeight_ + (count - 1) * tabGap_};
}

void WeatherTabGroup::resized() {}

void WeatherTabGroup::paint(juce::Graphics& g)
{
    if (tabLabels_.empty())
        return;

    const auto& theme = theme_ ? *theme_ : defaultTheme();
    auto metrics = control_surface::resolveSelectorMetrics(theme, scale_);
    metrics.cornerRadius = juce::jmax(4.0f, metrics.cornerRadius);
    metrics.dividerInset = 3.0f;
    const auto tabs = computeTabLayout();
    const auto font = tabLabelFont();

    for (int index = 0; index < static_cast<int>(tabLabels_.size()); ++index)
    {
        const auto state = makeTabState(*this, index, selectedIndex_, pressedIndex_, hoveredIndex_);
        auto palette = control_surface::resolveSelectorPalette(theme, state);
        const auto bounds = tabs[static_cast<size_t>(index)];
        const auto drawBounds = insetTabBounds(bounds);

        if (state.selected)
        {
            g.setColour(theme.weatherColors.bgRaised.brighter(0.04f));
            g.fillRoundedRectangle(drawBounds, metrics.cornerRadius);
            namespace opt = bws::tokens::shared::opacity::tab;
            g.setColour(theme.weatherColors.accent.withAlpha(state.focused ? opt::FOCUS_RING : opt::FOCUS_RING_IDLE));
            g.drawRoundedRectangle(drawBounds.reduced(bws::tokens::shared::geometry::STROKE_HALF_PX),
                                   metrics.cornerRadius, 1.0f);
            palette.text = theme.weatherColors.textPrimary;
        }
        else
        {
            namespace opt = bws::tokens::shared::opacity::tab;
            const auto fill = state.hovered ? theme.weatherColors.bgRaised.brighter(0.03f)
                                            : theme.weatherColors.bgRaised.withMultipliedAlpha(opt::BG_RAISED_MULT);
            const auto border = state.hovered
                                    ? theme.weatherColors.borderLight.withMultipliedAlpha(opt::BORDER_MULT_FOCUSED)
                                    : theme.weatherColors.borderLight.withMultipliedAlpha(opt::BORDER_MULT_IDLE);
            g.setColour(fill);
            g.fillRoundedRectangle(drawBounds, metrics.cornerRadius);
            g.setColour(border);
            g.drawRoundedRectangle(drawBounds.reduced(bws::tokens::shared::geometry::STROKE_HALF_PX),
                                   metrics.cornerRadius, 1.0f);
            palette.text = theme.weatherColors.textSecondary.withMultipliedAlpha(
                state.enabled ? opt::TEXT_MULT_ENABLED : opt::TEXT_MULT_DISABLED);
        }

        g.setColour(palette.text);
        g.setFont(font);
        g.drawText(tabLabels_[static_cast<size_t>(index)], drawBounds, juce::Justification::centred);

        if (state.focused && selectedIndex_ == index)
            control_surface::paintFocusRing(g, drawBounds, metrics, palette);
    }
}

void WeatherTabGroup::mouseDown(const juce::MouseEvent& e)
{
    grabKeyboardFocus();
    pressedIndex_ = getTabIndexAt(e.getPosition());
    repaint();
}

void WeatherTabGroup::mouseUp(const juce::MouseEvent& e)
{
    const int clickedIndex = getTabIndexAt(e.getPosition());
    if (clickedIndex >= 0 && clickedIndex == pressedIndex_)
        handleTabClick(clickedIndex, true);

    pressedIndex_ = -1;
    repaint();
}

void WeatherTabGroup::mouseMove(const juce::MouseEvent& e)
{
    const int nextHover = getTabIndexAt(e.getPosition());
    if (nextHover != hoveredIndex_)
    {
        hoveredIndex_ = nextHover;
        repaint();
    }
}

void WeatherTabGroup::mouseExit(const juce::MouseEvent& /*e*/)
{
    if (hoveredIndex_ != -1)
    {
        hoveredIndex_ = -1;
        repaint();
    }
}

bool WeatherTabGroup::keyPressed(const juce::KeyPress& key)
{
    if (!isEnabled())
        return false;

    if (key == juce::KeyPress::spaceKey || key == juce::KeyPress::returnKey)
    {
        if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(tabLabels_.size()))
        {
            handleTabClick(selectedIndex_, true);
            return true;
        }
    }

    return false;
}

int WeatherTabGroup::getTabIndexAt(juce::Point<int> pos) const
{
    if (!getLocalBounds().contains(pos))
        return -1;

    const auto layout = computeTabLayout();
    for (int index = 0; index < static_cast<int>(layout.size()); ++index)
        if (layout[static_cast<size_t>(index)].contains(pos.toFloat()))
            return index;

    return -1;
}

void WeatherTabGroup::handleTabClick(int index, bool notify)
{
    setSelectedTab(index, notify);
}

std::vector<juce::Rectangle<float>> WeatherTabGroup::computeTabLayout() const
{
    std::vector<juce::Rectangle<float>> layout;
    if (tabLabels_.empty())
        return layout;

    layout.reserve(tabLabels_.size());
    float x = 0.0f;
    float y = 0.0f;

    for (size_t index = 0; index < tabLabels_.size(); ++index)
    {
        layout.emplace_back(x, y, static_cast<float>(tabWidth_), static_cast<float>(tabHeight_));
        if (orientation_ == Orientation::Horizontal)
            x += static_cast<float>(tabWidth_ + tabGap_);
        else
            y += static_cast<float>(tabHeight_ + tabGap_);
    }

    return layout;
}

} // namespace bws::weather

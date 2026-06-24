// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/Components/UiTabBar.h"
#include "bw_ui/generated/BwTokens.h"

namespace bws::ui
{

UiTabBar::UiTabBar(const UiThemeResolved& themeIn, bool denseIn)
    : theme(themeIn)
    , kernelTheme(adapters::makeKernelThemeSnapshot(themeIn))
    , dense(denseIn)
{
    setWantsKeyboardFocus(true);
    setRepaintsOnMouseActivity(false);
}

void UiTabBar::setTheme(const UiThemeResolved& newTheme)
{
    theme = newTheme;
    kernelTheme = adapters::makeKernelThemeSnapshot(newTheme);
    repaint();
}

void UiTabBar::setScale(float newScale)
{
    scale = newScale;
    resized();
}

void UiTabBar::setTabs(const std::vector<juce::String>& titles)
{
    tabs.clear();
    tabs.reserve(titles.size());
    for (const auto& t : titles)
    {
        TabRect tab;
        tab.label = t;
        tabs.push_back(tab);
    }
    selectedIndex = juce::jlimit(0, (int)tabs.size() - 1, selectedIndex);
    updateLayout();
    repaint();
}

void UiTabBar::setSelectedIndex(int index)
{
    const int clamped = juce::jlimit(0, (int)tabs.size() - 1, index);
    if (clamped == selectedIndex)
        return;
    selectedIndex = clamped;
    repaint();
    if (onChange)
        onChange(selectedIndex);
}

void UiTabBar::setOnChange(std::function<void(int)> handler)
{
    onChange = std::move(handler);
}

void UiTabBar::setDense(bool shouldBeDense)
{
    dense = shouldBeDense;
    resized();
}

float UiTabBar::tabHeight() const
{
    const float base = dense ? kernelTheme.spacing.lg : kernelTheme.spacing.lg + kernelTheme.spacing.xs;
    return base * scale;
}

float UiTabBar::tabPaddingX() const
{
    return (dense ? kernelTheme.spacing.sm : kernelTheme.spacing.md) * scale;
}

float UiTabBar::tabGap() const
{
    return kernelTheme.spacing.xs * scale;
}

float UiTabBar::cornerRadius() const
{
    return theme.surfaces.radiusSm * scale;
}

juce::Font UiTabBar::labelFont() const
{
    return adapters::makeFont(kernelTheme, kernel::TextRole::Tab, scale);
}

juce::Rectangle<int> UiTabBar::contentBounds() const
{
    const int pad = (int)std::round(kernelTheme.spacing.sm * scale);
    auto bounds = getLocalBounds();
    bounds.reduce(pad, pad);
    return bounds;
}

void UiTabBar::updateLayout()
{
    auto area = contentBounds();
    const int h = (int)std::round(tabHeight());
    auto font = labelFont();
    const int gap = (int)std::round(tabGap());
    for (auto& tab : tabs)
    {
        const int textW = (int)std::round(juce::GlyphArrangement::getStringWidth(font, tab.label));
        const int w = textW + (int)std::round(tabPaddingX() * 2.0f);
        tab.bounds = {area.getX(), area.getY(), w, h};
        area.translate(w + gap, 0);
    }
}

int UiTabBar::hitTestIndex(juce::Point<int> pos) const
{
    for (size_t i = 0; i < tabs.size(); ++i)
    {
        if (tabs[i].bounds.contains(pos))
            return (int)i;
    }
    return -1;
}

void UiTabBar::paint(juce::Graphics& g)
{
    auto font = labelFont();
    g.setFont(font);

    for (size_t i = 0; i < tabs.size(); ++i)
    {
        const bool selected = (int)i == selectedIndex;
        const bool hover = (int)i == hoverIndex;
        const bool isPressed = (int)i == pressIndex;

        auto bounds = tabs[i].bounds.toFloat();
        juce::Colour bg;
        juce::Colour outline;
        juce::Colour text;

        if (selected)
        {
            bg = isPressed
                     ? theme.colors.accent0.darker(0.08f)
                     : (hover ? theme.colors.accent1
                              : theme.colors.accent0.withAlpha(bws::tokens::shared::opacity::tab_bar::SELECTED_FILL));
            outline = juce::Colours::transparentBlack;
            text = theme.colors.text0;
        }
        else
        {
            const auto base = theme.colors.bg1;
            namespace opb = bws::tokens::shared::opacity::tab_bar;
            const auto hoverBg = theme.colors.accent0.withAlpha(opb::PRESSED_FILL_BASE);
            bg = isPressed ? hoverBg.withAlpha(opb::PRESSED_FILL_OVERLAY) : (hover ? hoverBg : base);
            outline = theme.colors.outline0.withAlpha(opb::OUTLINE);
            text = theme.colors.text1;
        }

        g.setColour(bg);
        g.fillRoundedRectangle(bounds, cornerRadius());

        if (outline.getAlpha() > 0.0f)
        {
            g.setColour(outline);
            g.drawRoundedRectangle(bounds, cornerRadius(), theme.surfaces.strokeThin);
        }

        g.setColour(text);
        g.drawFittedText(adapters::applyTextCasing(kernelTheme, kernel::TextRole::Tab, tabs[i].label), tabs[i].bounds,
                         juce::Justification::centred, 1);
    }

    if (hasKeyboardFocus(true))
    {
        const int inset = (int)std::round(kernelTheme.spacing.xxs * scale);
        auto focus = contentBounds().toFloat().reduced((float)inset);
        g.setColour(theme.colors.accent1.withAlpha(bws::tokens::shared::opacity::tab_bar::FOCUS_RING));
        g.drawRoundedRectangle(focus, cornerRadius(), theme.surfaces.strokeThin);
    }
}

void UiTabBar::resized()
{
    updateLayout();
}

void UiTabBar::mouseMove(const juce::MouseEvent& e)
{
    if (tabs.empty())
        return;
    const int hit = hitTestIndex(e.position.toInt());
    if (hit != hoverIndex)
    {
        hoverIndex = hit;
        repaint();
    }
}

void UiTabBar::mouseExit(const juce::MouseEvent&)
{
    if (hoverIndex != -1 || pressIndex != -1)
    {
        hoverIndex = -1;
        pressIndex = -1;
        repaint();
    }
}

void UiTabBar::mouseDown(const juce::MouseEvent& e)
{
    if (!e.mods.isLeftButtonDown() || tabs.empty())
        return;
    pressIndex = hitTestIndex(e.position.toInt());
    repaint();
}

void UiTabBar::mouseUp(const juce::MouseEvent& e)
{
    if (pressIndex < 0)
    {
        repaint();
        return;
    }

    const int releasedOn = hitTestIndex(e.position.toInt());
    const int toSelect = (releasedOn == pressIndex) ? pressIndex : selectedIndex;
    pressIndex = -1;
    repaint();

    setSelectedIndex(toSelect);
}

bool UiTabBar::keyPressed(const juce::KeyPress& key)
{
    if (tabs.empty())
        return false;

    const bool left = key.getKeyCode() == juce::KeyPress::leftKey;
    const bool right = key.getKeyCode() == juce::KeyPress::rightKey;
    if (!left && !right)
        return false;

    int next = selectedIndex + (right ? 1 : -1);
    next = juce::jlimit(0, (int)tabs.size() - 1, next);
    if (next != selectedIndex)
        setSelectedIndex(next);
    return true;
}

void UiTabBar::focusGained(FocusChangeType)
{
    repaint();
}

void UiTabBar::focusLost(FocusChangeType)
{
    repaint();
}

} // namespace bws::ui

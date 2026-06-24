// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/Components/UiTabs.h"
#include "bw_ui/generated/BwTokens.h"

namespace bws::ui
{

UiTabs::TabsLookAndFeel::TabsLookAndFeel(const UiThemeResolved* themeRef, kernel::ThemeSnapshot* kernelThemeRef,
                                         UiTabRole* roleRef, float* scaleRef)
    : theme(themeRef)
    , kernelTheme(kernelThemeRef)
    , role(roleRef)
    , scale(scaleRef)
{}

void UiTabs::TabsLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                   const juce::Colour& backgroundColour,
                                                   bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused(backgroundColour);
    if (theme == nullptr || role == nullptr || scale == nullptr)
        return;

    const auto metrics = UiTabsTokens::metrics(*role, *scale, *theme);
    auto bounds = button.getLocalBounds().toFloat();
    const bool on = button.getToggleState();

    namespace op = bws::tokens::shared::opacity::ui_tabs;
    g.setColour(on ? theme->colors.accent0.withAlpha(op::FILL_ON) : theme->colors.bg2.withAlpha(op::FILL_OFF));
    g.fillRoundedRectangle(bounds, metrics.cornerRadius);

    g.setColour((on ? theme->colors.accent1 : theme->colors.outline0).withAlpha(op::OUTLINE));
    g.drawRoundedRectangle(bounds, metrics.cornerRadius, metrics.outlineThickness);

    if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown)
    {
        g.setColour(theme->colors.glowAccent);
        g.drawRoundedRectangle(bounds.reduced(bws::tokens::shared::geometry::STROKE_FULL_PX), metrics.cornerRadius,
                               metrics.outlineThickness);
    }
}

juce::Font UiTabs::TabsLookAndFeel::getTextButtonFont(juce::TextButton& b, int height)
{
    if (kernelTheme == nullptr || scale == nullptr)
        return LookAndFeel_V4::getTextButtonFont(b, height);
    juce::ignoreUnused(height);
    return adapters::makeFont(*kernelTheme, kernel::TextRole::Tab, *scale);
}

UiTabs::UiTabs(const UiThemeResolved& themeIn, UiTabRole roleIn)
    : theme(themeIn)
    , kernelTheme(adapters::makeKernelThemeSnapshot(themeIn))
    , role(roleIn)
    , lnf(&theme, &kernelTheme, &role, &scale)
{}

void UiTabs::setTabs(const std::vector<TabSpec>& tabs)
{
    entries.clear();
    for (const auto& spec : tabs)
    {
        auto btn = std::make_unique<juce::TextButton>(spec.label);
        btn->setLookAndFeel(&lnf);
        btn->setClickingTogglesState(true);
        btn->setRadioGroupId(0xB711);
        btn->onClick = [this, id = spec.id]() {
            setActive(id);
        };
        btn->setColour(juce::TextButton::textColourOffId, theme.colors.text1);
        btn->setColour(juce::TextButton::textColourOnId, theme.colors.text0);
        entries.push_back({std::move(btn), spec});
    }
    for (auto& entry : entries)
        addAndMakeVisible(*entry.button);

    if (!entries.empty())
        setActive(entries.front().spec.id);
    resized();
}

void UiTabs::setTheme(const UiThemeResolved& newTheme)
{
    theme = newTheme;
    kernelTheme = adapters::makeKernelThemeSnapshot(newTheme);
    repaint();
}

void UiTabs::setRole(UiTabRole newRole)
{
    role = newRole;
    repaint();
}

void UiTabs::setScale(float newScale)
{
    scale = newScale;
    repaint();
    resized();
}

void UiTabs::setOnChange(std::function<void(juce::String)> callback)
{
    onChange = std::move(callback);
}

void UiTabs::setActive(const juce::String& id)
{
    activeId = id;
    for (auto& entry : entries)
        entry.button->setToggleState(entry.spec.id == id, juce::dontSendNotification);
    if (onChange)
        onChange(id);
    repaint();
}

void UiTabs::paint(juce::Graphics& g)
{
    const auto metrics = UiTabsTokens::metrics(role, scale, theme);
    g.setColour(theme.colors.bg1.withAlpha(bws::tokens::shared::opacity::ui_tabs::BG));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), metrics.cornerRadius);
    g.setColour(theme.colors.outline0.withAlpha(bws::tokens::shared::opacity::ui_tabs::AMBIENT_OUTLINE));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), metrics.cornerRadius, metrics.outlineThickness);
}

void UiTabs::resized()
{
    if (entries.empty())
        return;
    const auto metrics = UiTabsTokens::metrics(role, scale, theme);
    auto area = getLocalBounds().reduced((int)metrics.paddingX / bws::tokens::shared::spacing::XXS);
    const int buttonCount = (int)entries.size();
    const int buttonWidth = juce::jmax(1, area.getWidth() / buttonCount);
    for (auto& entry : entries)
        entry.button->setBounds(area.removeFromLeft(buttonWidth).reduced((int)metrics.paddingX / 3));
}

} // namespace bws::ui

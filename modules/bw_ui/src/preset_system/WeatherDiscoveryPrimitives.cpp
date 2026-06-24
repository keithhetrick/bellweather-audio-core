// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/preset_system/WeatherDiscoveryPrimitives.h"
#include "bw_ui/foundation/Fonts.h"
#include "bw_ui/adapters/UiThemeKernelAdapter.h"
#include "bw_ui/generated/BwTokens.h"

namespace bws::weather
{
namespace
{
const bws::ui::UiThemeResolved& defaultTheme()
{
    static const auto kDefault = bws::ui::resolveTheme(bws::ui::defaultUiThemeBase(), {});
    return kDefault;
}

const bws::ui::UiThemeResolved& resolvedTheme(const bws::ui::UiThemeResolved* theme)
{
    return theme != nullptr ? *theme : defaultTheme();
}

juce::Font makeChipFont(float scaleFactor)
{
    return bw::Fonts::caption(9.0f * scaleFactor);
}

juce::Font makeTitleFont(float scaleFactor)
{
    return bw::Fonts::body(10.9f * scaleFactor);
}

juce::Font makeMetadataFont(float scaleFactor)
{
    return bw::Fonts::caption(8.9f * scaleFactor);
}

int measureTextWidth(const juce::Font& font, const juce::String& text)
{
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(font, text, 0.0f, 0.0f);
    return juce::roundToInt(glyphs.getBoundingBox(0, glyphs.getNumGlyphs(), true).getWidth());
}

juce::Colour resolveBadgeFill(const bws::ui::UiThemeResolved& theme, BadgeTone tone)
{
    const auto& colors = theme.weatherColors;
    namespace op = bws::tokens::shared::opacity::badge;
    switch (tone)
    {
    case BadgeTone::Neutral:
        return colors.bgRaised.withAlpha(op::NEUTRAL_FILL);
    case BadgeTone::Subtle:
        return colors.bgInput.withAlpha(op::SUBTLE_FILL);
    case BadgeTone::Accent:
        return colors.accent.withAlpha(op::ACCENT_FILL);
    case BadgeTone::Status:
        return colors.accent.withAlpha(op::STATUS_FILL);
    }

    return colors.bgRaised.withAlpha(op::NEUTRAL_FILL);
}

juce::Colour resolveBadgeBorder(const bws::ui::UiThemeResolved& theme, BadgeTone tone)
{
    const auto& colors = theme.weatherColors;
    namespace op = bws::tokens::shared::opacity::badge;
    switch (tone)
    {
    case BadgeTone::Neutral:
        return colors.borderLight.withAlpha(op::NEUTRAL_BORDER);
    case BadgeTone::Subtle:
        return colors.borderLight.withAlpha(op::SUBTLE_BORDER);
    case BadgeTone::Accent:
        return colors.accent.withAlpha(op::ACCENT_BORDER);
    case BadgeTone::Status:
        return colors.accent.withAlpha(op::STATUS_BORDER);
    }

    return colors.borderLight.withAlpha(op::NEUTRAL_BORDER);
}

juce::Colour resolveBadgeText(const bws::ui::UiThemeResolved& theme, BadgeTone tone)
{
    const auto& colors = theme.weatherColors;
    namespace op = bws::tokens::shared::opacity::badge;
    switch (tone)
    {
    case BadgeTone::Neutral:
        return colors.textBright.withAlpha(op::NEUTRAL_TEXT);
    case BadgeTone::Subtle:
        return colors.textBright.withAlpha(op::SUBTLE_TEXT);
    case BadgeTone::Accent:
        return colors.textBright.withAlpha(op::ACCENT_TEXT);
    case BadgeTone::Status:
        return colors.textBright.withAlpha(op::STATUS_TEXT);
    }

    return colors.textBright.withAlpha(op::NEUTRAL_TEXT);
}

} // namespace

WeatherChipButton::WeatherChipButton()
    : juce::Button({})
{
    setTriggeredOnMouseDown(false);
    setWantsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void WeatherChipButton::setTheme(const bws::ui::UiThemeResolved& theme)
{
    theme_ = &theme;
    repaint();
}

void WeatherChipButton::setScaleFactor(float scaleFactor)
{
    scaleFactor_ = scaleFactor;
    repaint();
}

void WeatherChipButton::paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown)
{
    const auto& theme = resolvedTheme(theme_);
    auto bounds = getLocalBounds().toFloat();
    const float radius = juce::jmax(6.0f, theme.weatherColors.radiusMd * scaleFactor_);

    namespace opc = bws::tokens::shared::opacity::chip;
    auto fill = getToggleState() ? theme.weatherColors.accent.withAlpha(opc::FILL_SELECTED)
                                 : theme.weatherColors.bgInput.withAlpha(opc::FILL_UNSELECTED);
    auto border = getToggleState()
                      ? theme.weatherColors.accent.withAlpha(hasKeyboardFocus(true) ? opc::BORDER_SELECTED_FOCUSED
                                                                                    : opc::BORDER_SELECTED_DEFAULT)
                      : theme.weatherColors.borderLight.withAlpha(isMouseOver ? opc::BORDER_UNSELECTED_HOVER
                                                                              : opc::BORDER_UNSELECTED_DEFAULT);
    auto text = getToggleState() ? theme.weatherColors.textBright.withAlpha(opc::TEXT_BRIGHT)
                                 : theme.weatherColors.textBright.withAlpha(isMouseOver ? opc::TEXT_UNSELECTED_HOVER
                                                                                        : opc::TEXT_UNSELECTED_DEFAULT);

    if (isButtonDown)
    {
        fill = fill.brighter(0.04f);
        border = border.withMultipliedAlpha(1.1f);
    }
    if (!isEnabled())
    {
        fill = fill.withMultipliedAlpha(opc::DISABLED_FILL_MULT);
        border = border.withMultipliedAlpha(opc::DISABLED_BORDER_MULT);
        text = text.withMultipliedAlpha(opc::DISABLED_TEXT_MULT);
    }

    g.setColour(fill);
    g.fillRoundedRectangle(bounds, radius);
    g.setColour(border);
    g.drawRoundedRectangle(bounds.reduced(bws::tokens::shared::geometry::STROKE_HALF_PX), radius, 1.0f);

    if (hasKeyboardFocus(true))
    {
        g.setColour(theme.weatherColors.accent.withAlpha(bws::tokens::shared::opacity::chip::ACCENT_STROKE));
        g.drawRoundedRectangle(bounds.reduced(bws::tokens::shared::geometry::STROKE_ONE_HALF_PX),
                               juce::jmax(1.0f, radius - 1.0f), 1.0f);
    }

    g.setColour(text);
    g.setFont(makeChipFont(scaleFactor_));
    g.drawFittedText(getButtonText(), getLocalBounds(), juce::Justification::centred, 1);
}

bool WeatherChipButton::keyPressed(const juce::KeyPress& key)
{
    if (!isEnabled())
        return false;

    if (key == juce::KeyPress::spaceKey || key == juce::KeyPress::returnKey)
    {
        triggerClick();
        return true;
    }

    return false;
}

void WeatherFloatingPanelShell::setTheme(const bws::ui::UiThemeResolved& theme)
{
    theme_ = &theme;
    repaint();
}

void WeatherFloatingPanelShell::setScaleFactor(float scaleFactor)
{
    scaleFactor_ = scaleFactor;
    repaint();
}

void WeatherFloatingPanelShell::setDividerY(int dividerY)
{
    dividerY_ = dividerY;
    repaint();
}

juce::Rectangle<int> WeatherFloatingPanelShell::getContentBounds() const
{
    const int inset = juce::roundToInt(14.0f * scaleFactor_);
    return getLocalBounds().reduced(inset);
}

void WeatherFloatingPanelShell::paint(juce::Graphics& g)
{
    const auto& theme = resolvedTheme(theme_);
    auto bounds = getLocalBounds().toFloat();
    g.fillAll(juce::Colours::transparentBlack);

    namespace opc = bws::tokens::shared::opacity::card;
    g.setColour(juce::Colour(bws::tokens::shared::discovery::SHADOW).withAlpha(opc::SHADOW));
    g.fillRoundedRectangle(bounds, 12.0f * scaleFactor_);

    g.setColour(juce::Colour(bws::tokens::shared::discovery::HAIRLINE).withAlpha(opc::HAIRLINE));
    g.fillRoundedRectangle(bounds.reduced(bws::tokens::shared::geometry::STROKE_THREE_QUARTER_PX),
                           11.0f * scaleFactor_);

    g.setColour(theme.weatherColors.panelNearBlack.withAlpha(opc::SURFACE));
    g.fillRoundedRectangle(bounds.reduced(bws::tokens::shared::geometry::STROKE_FULL_PX), 11.0f * scaleFactor_);

    auto panel = bounds.reduced(bws::tokens::shared::geometry::STROKE_THREE_QUARTER_PX);
    g.setColour(theme.weatherColors.borderLight.withAlpha(opc::BORDER));
    g.drawRoundedRectangle(panel, 12.0f * scaleFactor_, 1.0f);

    if (dividerY_ > 0 && dividerY_ < getHeight())
    {
        g.setColour(theme.weatherColors.textBright.withAlpha(opc::TEXTURE_STRIPE));
        g.fillRect(12.0f * scaleFactor_, static_cast<float>(dividerY_), getWidth() - 24.0f * scaleFactor_, 1.0f);
    }
}

WeatherFilterChipGroup::WeatherFilterChipGroup() = default;

void WeatherFilterChipGroup::setTheme(const bws::ui::UiThemeResolved& theme)
{
    theme_ = &theme;
    repaint();
}

void WeatherFilterChipGroup::setScaleFactor(float scaleFactor)
{
    scaleFactor_ = scaleFactor;
    resized();
    repaint();
}

void WeatherFilterChipGroup::setOptions(const std::vector<Option>& options)
{
    options_ = options;
    rebuildButtons();
}

void WeatherFilterChipGroup::setSelectedId(const juce::String& selectedId)
{
    selectedId_ = selectedId;
    syncButtonStates();
}

const bws::ui::UiThemeResolved& WeatherFilterChipGroup::getResolvedTheme() const
{
    return resolvedTheme(theme_);
}

int WeatherFilterChipGroup::getPreferredHeight() const
{
    return juce::roundToInt(22.0f * scaleFactor_);
}

int WeatherFilterChipGroup::getChipCount() const
{
    return buttons_.size();
}

void WeatherFilterChipGroup::resized()
{
    const int gap = juce::roundToInt(6.0f * scaleFactor_);
    const int rowHeight = getPreferredHeight();
    int x = 0;
    const auto chipFont = makeChipFont(scaleFactor_);

    for (auto* button : buttons_)
    {
        const int width =
            juce::jlimit(66, juce::roundToInt(150.0f * scaleFactor_),
                         measureTextWidth(chipFont, button->getButtonText()) + juce::roundToInt(22.0f * scaleFactor_));
        button->setBounds(x, 0, width, rowHeight);
        x += width + gap;
    }
}

void WeatherFilterChipGroup::rebuildButtons()
{
    while (buttons_.size() > static_cast<int>(options_.size()))
    {
        removeChildComponent(buttons_.getLast());
        buttons_.removeLast();
    }

    for (int i = 0; i < static_cast<int>(options_.size()); ++i)
    {
        auto* button = i < buttons_.size() ? buttons_[i] : nullptr;
        if (button == nullptr)
        {
            auto owned = std::make_unique<WeatherChipButton>();
            button = owned.get();
            addAndMakeVisible(*button);
            buttons_.add(owned.release());
        }

        button->setTheme(getResolvedTheme());
        button->setScaleFactor(scaleFactor_);

        const auto option = options_[static_cast<size_t>(i)];
        button->setComponentID(option.id);
        button->setButtonText(option.label);
        button->onClick = [this, optionId = option.id] {
            selectedId_ = optionId;
            syncButtonStates();
            if (onSelectionChanged)
                onSelectionChanged(optionId);
        };
    }

    syncButtonStates();
    resized();
}

void WeatherFilterChipGroup::syncButtonStates()
{
    for (auto* button : buttons_)
        button->setToggleState(button->getComponentID() == selectedId_, juce::dontSendNotification);

    repaint();
}

void WeatherStatusBadge::setTheme(const bws::ui::UiThemeResolved& theme)
{
    theme_ = &theme;
    repaint();
}

void WeatherStatusBadge::setScaleFactor(float scaleFactor)
{
    scaleFactor_ = scaleFactor;
    repaint();
}

void WeatherStatusBadge::setLabel(const juce::String& label)
{
    label_ = label;
    repaint();
}

void WeatherStatusBadge::setTone(BadgeTone tone)
{
    tone_ = tone;
    repaint();
}

void WeatherStatusBadge::setSizeVariant(BadgeSize size)
{
    size_ = size;
    repaint();
}

int WeatherStatusBadge::getPreferredWidth() const
{
    const auto font = bw::Fonts::caption((size_ == BadgeSize::Compact ? 8.4f : 8.9f) * scaleFactor_);
    const float horizontalPadding = (size_ == BadgeSize::Compact ? 12.0f : 16.0f) * scaleFactor_;
    return measureTextWidth(font, label_) + juce::roundToInt(horizontalPadding);
}

int WeatherStatusBadge::getPreferredHeight() const
{
    return juce::roundToInt((size_ == BadgeSize::Compact ? 16.0f : 18.0f) * scaleFactor_);
}

void WeatherStatusBadge::paint(juce::Graphics& g)
{
    const auto& theme = resolvedTheme(theme_);
    auto bounds = getLocalBounds().toFloat();
    const float radius = bounds.getHeight() * 0.48f;
    g.setColour(resolveBadgeFill(theme, tone_));
    g.fillRoundedRectangle(bounds, radius);
    g.setColour(resolveBadgeBorder(theme, tone_));
    g.drawRoundedRectangle(bounds.reduced(bws::tokens::shared::geometry::STROKE_HALF_PX), radius, 1.0f);
    g.setColour(resolveBadgeText(theme, tone_));
    g.setFont(bw::Fonts::caption((size_ == BadgeSize::Compact ? 8.4f : 8.9f) * scaleFactor_));
    g.drawFittedText(label_, getLocalBounds(), juce::Justification::centred, 1);
}

WeatherLibraryRow::WeatherLibraryRow()
{
    setOpaque(true);
    setWantsKeyboardFocus(false);
    setInterceptsMouseClicks(true, false);
}

void WeatherLibraryRow::setTheme(const bws::ui::UiThemeResolved& theme)
{
    theme_ = &theme;
    for (auto* badge : badges_)
        badge->setTheme(theme);
    repaint();
}

void WeatherLibraryRow::setScaleFactor(float scaleFactor)
{
    scaleFactor_ = scaleFactor;
    for (auto* badge : badges_)
        badge->setScaleFactor(scaleFactor);
    resized();
    repaint();
}

void WeatherLibraryRow::setDensity(LibraryRowDensity density)
{
    density_ = density;
    resized();
    repaint();
}

void WeatherLibraryRow::setModel(const Model& model)
{
    model_ = model;
    rebuildBadges();
    resized();
    repaint();
}

int WeatherLibraryRow::getPreferredHeight() const
{
    return juce::roundToInt((density_ == LibraryRowDensity::Compact ? 36.0f : 42.0f) * scaleFactor_);
}

int WeatherLibraryRow::getVisibleBadgeCount() const
{
    return badges_.size();
}

juce::Rectangle<int> WeatherLibraryRow::getFavouriteBounds() const
{
    return {juce::roundToInt(10.0f * scaleFactor_), juce::roundToInt(8.0f * scaleFactor_),
            juce::roundToInt(18.0f * scaleFactor_), getHeight() - juce::roundToInt(16.0f * scaleFactor_)};
}

void WeatherLibraryRow::resized()
{
    const int rightInset = juce::roundToInt(12.0f * scaleFactor_);
    const int gap = juce::roundToInt(6.0f * scaleFactor_);
    int x = getWidth() - rightInset;

    for (int i = badges_.size(); --i >= 0;)
    {
        auto* badge = badges_[i];
        const int width = badge->getPreferredWidth();
        const int height = badge->getPreferredHeight();
        x -= width;
        badge->setBounds(x, (getHeight() - height) / 2, width, height);
        x -= gap;
    }
}

void WeatherLibraryRow::paint(juce::Graphics& g)
{
    const auto& theme = resolvedTheme(theme_);
    auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(getWidth()), static_cast<float>(getHeight()));
    g.fillAll(theme.weatherColors.panelNearBlack.withAlpha(1.0f));
    const auto capsule = bounds.reduced(6.0f * scaleFactor_, 3.0f * scaleFactor_);

    namespace opr = bws::tokens::shared::opacity::row;
    if (model_.highlighted)
    {
        g.setColour(theme.weatherColors.accent.withAlpha(opr::HOVER_BG));
        g.fillRoundedRectangle(capsule, 8.0f * scaleFactor_);
        g.setColour(theme.weatherColors.accent.withAlpha(opr::PRESSED_BG));
        g.drawRoundedRectangle(capsule.reduced(bws::tokens::shared::geometry::STROKE_HALF_PX), 8.0f * scaleFactor_,
                               1.0f);
    }
    else if (model_.selected)
    {
        g.setColour(theme.weatherColors.textBright.withAlpha(opr::HAIRLINE));
        g.fillRoundedRectangle(capsule, 8.0f * scaleFactor_);
    }

    auto starBounds = getFavouriteBounds().toFloat();
    g.setFont(bw::Fonts::body(12.0f * scaleFactor_));
    g.setColour(model_.favourite ? theme.weatherColors.accent.withAlpha(opr::SELECTED_INDICATOR)
                                 : theme.weatherColors.textBright.withAlpha(
                                       model_.favouriteEnabled ? opr::FAVOURITE_ENABLED : opr::FAVOURITE_DISABLED));
    g.drawText(model_.favourite ? juce::String::fromUTF8("★") : juce::String::fromUTF8("☆"), starBounds,
               juce::Justification::centred);

    int badgeLeft = getWidth() - juce::roundToInt(12.0f * scaleFactor_);
    if (!badges_.isEmpty())
        badgeLeft = badges_.getFirst()->getX() - juce::roundToInt(10.0f * scaleFactor_);

    const int textX = juce::roundToInt(38.0f * scaleFactor_);
    const int textWidth = juce::jmax(40, badgeLeft - textX);
    auto titleArea = juce::Rectangle<int>(textX, juce::roundToInt(4.0f * scaleFactor_), textWidth, getHeight() / 2);
    auto detailArea = juce::Rectangle<int>(textX, getHeight() / 2 - 1, textWidth, getHeight() / 2);

    g.setFont(makeTitleFont(scaleFactor_));
    g.setColour(
        theme.weatherColors.textBright.withAlpha(model_.highlighted ? opr::TITLE_HIGHLIGHTED : opr::TITLE_DEFAULT));
    g.drawText(model_.title, titleArea, juce::Justification::centredLeft);

    g.setFont(makeMetadataFont(scaleFactor_));
    g.setColour(theme.weatherColors.textBright.withAlpha(opr::SECONDARY_TEXT));
    g.drawText(model_.metadata, detailArea, juce::Justification::centredLeft);
}

void WeatherLibraryRow::mouseUp(const juce::MouseEvent& event)
{
    if (model_.favouriteEnabled && getFavouriteBounds().contains(event.getPosition()))
    {
        if (onFavouritePressed)
            onFavouritePressed();
        return;
    }

    if (onRowPressed)
        onRowPressed();
}

void WeatherLibraryRow::mouseDoubleClick(const juce::MouseEvent& event)
{
    if (model_.favouriteEnabled && getFavouriteBounds().contains(event.getPosition()))
        return;

    if (onRowDoubleClicked)
        onRowDoubleClicked();
}

void WeatherLibraryRow::rebuildBadges()
{
    const int visibleCount = juce::jmin(2, static_cast<int>(model_.badges.size()));

    while (badges_.size() > visibleCount)
    {
        removeChildComponent(badges_.getLast());
        badges_.removeLast();
    }

    for (int i = 0; i < visibleCount; ++i)
    {
        auto* badge = i < badges_.size() ? badges_[i] : nullptr;
        if (badge == nullptr)
        {
            auto owned = std::make_unique<WeatherStatusBadge>();
            badge = owned.get();
            addAndMakeVisible(*badge);
            badges_.add(owned.release());
        }

        badge->setTheme(resolvedTheme(theme_));
        badge->setScaleFactor(scaleFactor_);
        badge->setSizeVariant(BadgeSize::Compact);
        badge->setLabel(model_.badges[static_cast<size_t>(i)].label);
        badge->setTone(model_.badges[static_cast<size_t>(i)].tone);
    }
}

} // namespace bws::weather

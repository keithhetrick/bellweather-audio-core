// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/interactions/WeatherNumericEntryDialog.h"
#include "bw_ui/generated/BwTokens.h"

namespace bws::ui
{
namespace
{
constexpr int kCardWidthPx = 360;
constexpr int kCardHeightPx = 220;
constexpr int kPaddingPx = 20;

int sf(int px, float scaleFactor)
{
    return juce::roundToInt(static_cast<float>(px) * scaleFactor);
}
} // namespace

WeatherNumericEntryDialog::WeatherNumericEntryDialog(const UiThemeResolved& theme, float scaleFactor,
                                                     juce::String title, juce::String message,
                                                     juce::String initialValue, OnCommit onCommit,
                                                     std::function<void()> onDismiss)
    : theme_(theme)
    , scaleFactor_(scaleFactor)
    , title_(std::move(title))
    , message_(std::move(message))
    , onCommit_(std::move(onCommit))
    , dismissCallback_(std::move(onDismiss))
{
    shell_.setTheme(theme_);
    shell_.setScaleFactor(scaleFactor_);
    addAndMakeVisible(shell_);

    titleLabel_.setText(title_, juce::dontSendNotification);
    titleLabel_.setFont(juce::Font(juce::FontOptions(18.0f * scaleFactor_, juce::Font::bold)));
    titleLabel_.setJustificationType(juce::Justification::centredLeft);
    titleLabel_.setColour(juce::Label::textColourId, theme_.weatherColors.textBright);
    titleLabel_.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(titleLabel_);

    messageLabel_.setText(message_, juce::dontSendNotification);
    messageLabel_.setFont(juce::Font(juce::FontOptions(13.0f * scaleFactor_)));
    messageLabel_.setJustificationType(juce::Justification::centredLeft);
    messageLabel_.setColour(juce::Label::textColourId, theme_.weatherColors.textSecondary);
    messageLabel_.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(messageLabel_);

    valueEditor_.setMultiLine(false);
    valueEditor_.setReturnKeyStartsNewLine(false);
    valueEditor_.setColour(juce::TextEditor::backgroundColourId, theme_.weatherColors.bgInput);
    valueEditor_.setColour(juce::TextEditor::textColourId, theme_.weatherColors.textBright);
    valueEditor_.setColour(
        juce::TextEditor::outlineColourId,
        theme_.weatherColors.borderLight.withAlpha(bws::tokens::shared::opacity::dialog::OUTLINE_SUBTLE));
    valueEditor_.setColour(juce::TextEditor::focusedOutlineColourId,
                           theme_.weatherColors.accent.withAlpha(bws::tokens::shared::opacity::dialog::OUTLINE_ACCENT));
    valueEditor_.setFont(juce::Font(juce::FontOptions(14.0f * scaleFactor_)));
    valueEditor_.setText(initialValue, juce::dontSendNotification);
    valueEditor_.onReturnKey = [this] {
        okButton_.triggerClick();
    };
    valueEditor_.onEscapeKey = [this] {
        cancelButton_.triggerClick();
    };
    addAndMakeVisible(valueEditor_);

    auto configureChip = [this](bws::weather::WeatherChipButton& button, const juce::String& text,
                                std::function<void()> handler) {
        button.setTheme(theme_);
        button.setScaleFactor(scaleFactor_);
        button.setButtonText(text);
        button.onClick = std::move(handler);
        addAndMakeVisible(button);
    };

    configureChip(cancelButton_, "Cancel", [this] { dismiss(); });
    configureChip(okButton_, "OK", [this] { commit(); });

    setWantsKeyboardFocus(true);
}

void WeatherNumericEntryDialog::grabInitialKeyboardFocus()
{
    valueEditor_.grabKeyboardFocus();
    valueEditor_.selectAll();
}

juce::Rectangle<int> WeatherNumericEntryDialog::getCardBounds() const
{
    const int w = sf(kCardWidthPx, scaleFactor_);
    const int h = sf(kCardHeightPx, scaleFactor_);
    return {(getWidth() - w) / 2, (getHeight() - h) / 2, w, h};
}

void WeatherNumericEntryDialog::paint(juce::Graphics& g)
{
    if (!isOnDesktop())
        g.fillAll(juce::Colours::black.withAlpha(bws::tokens::shared::opacity::dialog::BACKDROP));
}

void WeatherNumericEntryDialog::resized()
{
    const auto card = getCardBounds();
    shell_.setBounds(card);

    const int padding = sf(kPaddingPx, scaleFactor_);
    const int contentLeft = card.getX() + padding;
    const int contentWidth = card.getWidth() - 2 * padding;

    int y = card.getY() + sf(18, scaleFactor_);
    titleLabel_.setBounds(contentLeft, y, contentWidth, sf(28, scaleFactor_));
    y += sf(34, scaleFactor_);

    messageLabel_.setBounds(contentLeft, y, contentWidth, sf(20, scaleFactor_));
    y += sf(30, scaleFactor_);

    valueEditor_.setBounds(contentLeft, y, contentWidth, sf(34, scaleFactor_));

    const int buttonWidth = sf(88, scaleFactor_);
    const int buttonHeight = sf(30, scaleFactor_);
    const int buttonGap = sf(12, scaleFactor_);
    const int buttonY = card.getBottom() - buttonHeight - sf(18, scaleFactor_);
    const int pairWidth = buttonWidth * 2 + buttonGap;
    const int buttonLeft = card.getCentreX() - pairWidth / 2;

    cancelButton_.setBounds(buttonLeft, buttonY, buttonWidth, buttonHeight);
    okButton_.setBounds(buttonLeft + buttonWidth + buttonGap, buttonY, buttonWidth, buttonHeight);
}

void WeatherNumericEntryDialog::mouseDown(const juce::MouseEvent& e)
{
    if (!getCardBounds().contains(e.getPosition()))
        dismiss();
}

bool WeatherNumericEntryDialog::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        dismiss();
        return true;
    }
    if (key == juce::KeyPress::returnKey)
    {
        okButton_.triggerClick();
        return true;
    }
    return true;
}

void WeatherNumericEntryDialog::commit()
{
    if (onCommit_)
        onCommit_(valueEditor_.getText().trim());
    dismiss();
}

void WeatherNumericEntryDialog::dismiss()
{
    if (dismissCallback_)
        dismissCallback_();
}

void showNumericEntryDialog(const UiThemeResolved& theme, float scaleFactor, juce::String title, juce::String message,
                            juce::String initialValue, WeatherNumericEntryDialog::OnCommit onCommit,
                            juce::Component* parent, bool desktopPromote)
{
    if (parent == nullptr)
        return;

    auto* dialog = new WeatherNumericEntryDialog(theme, scaleFactor, std::move(title), std::move(message),
                                                 std::move(initialValue), std::move(onCommit), []() {});
    juce::Component::SafePointer<WeatherNumericEntryDialog> safe(dialog);

    if (desktopPromote)
    {
        const int cardW = sf(kCardWidthPx, scaleFactor);
        const int cardH = sf(kCardHeightPx, scaleFactor);
        dialog->setSize(cardW, cardH);
        dialog->addToDesktop(juce::ComponentPeer::windowIsTemporary | juce::ComponentPeer::windowHasDropShadow);
        const auto parentScreen = parent->getScreenBounds();
        dialog->setTopLeftPosition(parentScreen.getCentreX() - cardW / 2, parentScreen.getCentreY() - cardH / 2);
        dialog->setVisible(true);
    }
    else
    {
        dialog->setBounds(parent->getLocalBounds());
        parent->addAndMakeVisible(dialog);
    }

    dialog->toFront(true);
    dialog->enterModalState(false);
    dialog->grabInitialKeyboardFocus();
    dialog->setDismissCallback([safe]() {
        if (auto* d = safe.getComponent())
        {
            d->exitModalState(0);
            delete d;
        }
    });
}

} // namespace bws::ui

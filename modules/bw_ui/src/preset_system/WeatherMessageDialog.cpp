// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#include "bw_ui/preset_system/WeatherMessageDialog.h"

#include "bw_ui/adapters/UiThemeKernelAdapter.h"
#include "bw_ui/generated/BwTokens.h"

namespace bws::weather
{
namespace
{
constexpr int kCardWidthPx = 420;
constexpr int kCardMinHeightPx = 190;
constexpr int kPaddingPx = 20;
constexpr int kTitleHeightPx = 28;
constexpr int kButtonWidthPx = 88;
constexpr int kButtonHeightPx = 30;
constexpr float kBackdropAlpha = 0.68f;

int sf(int px, float scaleFactor)
{
    return juce::roundToInt(static_cast<float>(px) * scaleFactor);
}

juce::TextLayout makeMessageLayout(const bws::ui::UiThemeResolved& theme, float scaleFactor,
                                   const juce::String& message, float maxWidth)
{
    const auto kernelTheme = bws::ui::adapters::makeKernelThemeSnapshot(theme);
    const auto bodyFont =
        bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::ControlText, scaleFactor * 0.95f);
    juce::AttributedString body;
    body.setJustification(juce::Justification::topLeft);
    body.append(message, bodyFont, theme.weatherColors.textSecondary);

    juce::TextLayout layout;
    layout.createLayoutWithBalancedLineLengths(body, maxWidth);
    return layout;
}
} // namespace

class WeatherMessageDialog::MessageTextLayer : public juce::Component
{
public:
    MessageTextLayer(const bws::ui::UiThemeResolved& theme, float scaleFactor, WeatherMessageDialogTone tone)
        : theme_(theme)
        , scaleFactor_(scaleFactor)
        , tone_(tone)
    {
        setInterceptsMouseClicks(false, false);
    }

    void setResolvedText(juce::String title, juce::String message)
    {
        title_ = std::move(title);
        message_ = std::move(message);
        repaint();
    }

    void setButtonTopY(int buttonTopY)
    {
        buttonTopY_ = buttonTopY;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        const int pad = sf(kPaddingPx, scaleFactor_);
        const auto kernelTheme = bws::ui::adapters::makeKernelThemeSnapshot(theme_);
        const auto titleFont =
            bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::SectionLabel, scaleFactor_);
        const auto titleColor =
            tone_ == WeatherMessageDialogTone::Warning ? theme_.colors.warn : theme_.weatherColors.textBright;

        auto titleArea = juce::Rectangle<float>(static_cast<float>(pad), static_cast<float>(pad),
                                                static_cast<float>(getWidth() - pad * 2),
                                                static_cast<float>(sf(kTitleHeightPx, scaleFactor_)));
        g.setColour(titleColor);
        g.setFont(titleFont);
        g.drawText(title_, titleArea.toNearestInt(), juce::Justification::centredLeft, false);

        auto bodyArea = juce::Rectangle<float>(
            static_cast<float>(pad), titleArea.getBottom() + static_cast<float>(sf(12, scaleFactor_)),
            static_cast<float>(getWidth() - pad * 2),
            static_cast<float>(buttonTopY_ - sf(12, scaleFactor_) - pad - titleArea.getHeight()));
        auto layout = makeMessageLayout(theme_, scaleFactor_, message_, bodyArea.getWidth());
        layout.draw(g, bodyArea);
    }

private:
    const bws::ui::UiThemeResolved& theme_;
    const float scaleFactor_;
    const WeatherMessageDialogTone tone_;
    juce::String title_;
    juce::String message_;
    int buttonTopY_ = 0;
};

WeatherMessageDialog::WeatherMessageDialog(const bws::ui::UiThemeResolved& theme, float scaleFactor, juce::String title,
                                           juce::String message, WeatherMessageDialogTone tone,
                                           juce::String dismissLabel, std::function<void()> onDismiss)
    : WeatherMessageDialog(theme, scaleFactor, nullptr, bws::ui::localization::LocalizedText(std::move(title)),
                           bws::ui::localization::LocalizedText(std::move(message)), tone,
                           bws::ui::localization::LocalizedText(std::move(dismissLabel)), std::move(onDismiss))
{}

WeatherMessageDialog::WeatherMessageDialog(const bws::ui::UiThemeResolved& theme, float scaleFactor,
                                           bws::ui::localization::LocalizationManager* localization,
                                           bws::ui::localization::LocalizedText title,
                                           bws::ui::localization::LocalizedText message, WeatherMessageDialogTone tone,
                                           bws::ui::localization::LocalizedText dismissLabel,
                                           std::function<void()> onDismiss)
    : theme_(theme)
    , scaleFactor_(scaleFactor)
    , localization_(localization)
    , titleText_(std::move(title))
    , messageText_(std::move(message))
    , dismissText_(std::move(dismissLabel))
    , tone_(tone)
    , dismissCallback_(std::move(onDismiss))
{
    shell_.setTheme(theme_);
    shell_.setScaleFactor(scaleFactor_);
    addAndMakeVisible(shell_);

    textLayer_ = std::make_unique<MessageTextLayer>(theme_, scaleFactor_, tone_);
    addAndMakeVisible(*textLayer_);

    dismissButton_.setTheme(theme_);
    dismissButton_.setScaleFactor(scaleFactor_);
    dismissButton_.onClick = [this] {
        dismiss();
    };
    addAndMakeVisible(dismissButton_);

    if (localization_ != nullptr)
        localization_->addListener(*this);
    refreshText();

    setWantsKeyboardFocus(true);
}

WeatherMessageDialog::~WeatherMessageDialog()
{
    if (localization_ != nullptr)
        localization_->removeListener(*this);
}

void WeatherMessageDialog::refreshText()
{
    if (localization_ != nullptr)
    {
        resolvedTitle_ = localization_->resolve(titleText_);
        resolvedMessage_ = localization_->resolve(messageText_);
        dismissButton_.setButtonText(localization_->resolve(dismissText_));
    }
    else
    {
        resolvedTitle_ = titleText_.value();
        resolvedMessage_ = messageText_.value();
        dismissButton_.setButtonText(dismissText_.value());
    }

    if (textLayer_ != nullptr)
        textLayer_->setResolvedText(resolvedTitle_, resolvedMessage_);

    resized();
    repaint();
}

juce::Rectangle<int> WeatherMessageDialog::getCardBounds() const
{
    const int width = sf(kCardWidthPx, scaleFactor_);
    const int innerWidth = width - sf(kPaddingPx * 2, scaleFactor_);
    const auto layout = makeMessageLayout(theme_, scaleFactor_, resolvedMessage_, static_cast<float>(innerWidth));
    const int bodyHeight = static_cast<int>(std::ceil(layout.getHeight()));
    const int topPadding = sf(kPaddingPx, scaleFactor_);
    const int titleHeight = sf(kTitleHeightPx, scaleFactor_);
    const int bodyGap = sf(12, scaleFactor_);
    const int bottomPadding = sf(18, scaleFactor_);
    const int buttonHeight = sf(kButtonHeightPx, scaleFactor_);
    const int totalHeight =
        juce::jmax(sf(kCardMinHeightPx, scaleFactor_),
                   topPadding + titleHeight + bodyGap + bodyHeight + bottomPadding + buttonHeight + topPadding);

    return {(getWidth() - width) / 2, (getHeight() - totalHeight) / 2, width, totalHeight};
}

void WeatherMessageDialog::paint(juce::Graphics& g)
{
    if (!isOnDesktop())
        g.fillAll(juce::Colours::black.withAlpha(kBackdropAlpha));
}

void WeatherMessageDialog::resized()
{
    const auto card = getCardBounds();
    shell_.setBounds(card);

    const int buttonWidth = sf(kButtonWidthPx, scaleFactor_);
    const int buttonHeight = sf(kButtonHeightPx, scaleFactor_);
    const int buttonY = card.getBottom() - buttonHeight - sf(18, scaleFactor_);
    dismissButton_.setBounds(card.getCentreX() - buttonWidth / 2, buttonY, buttonWidth, buttonHeight);

    if (textLayer_ != nullptr)
    {
        textLayer_->setBounds(card);
        textLayer_->setButtonTopY(buttonY - card.getY());
    }
}

void WeatherMessageDialog::mouseDown(const juce::MouseEvent& e)
{
    if (!getCardBounds().contains(e.getPosition()))
        dismiss();
}

bool WeatherMessageDialog::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey || key == juce::KeyPress::returnKey)
    {
        dismiss();
        return true;
    }
    return true;
}

void WeatherMessageDialog::localizationLanguageChanged()
{
    refreshText();
}

void WeatherMessageDialog::dismiss()
{
    if (dismissCallback_)
        dismissCallback_();
}

namespace
{
void attachAndShowDialog(WeatherMessageDialog* dialog, juce::Component* parent, float scaleFactor, bool desktopPromote)
{
    juce::Component::SafePointer<WeatherMessageDialog> safe(dialog);

    if (desktopPromote)
    {
        dialog->setSize(sf(kCardWidthPx, scaleFactor), sf(kCardMinHeightPx, scaleFactor));
        dialog->addToDesktop(juce::ComponentPeer::windowIsTemporary | juce::ComponentPeer::windowHasDropShadow);
        const auto parentScreen = parent->getScreenBounds();
        dialog->setTopLeftPosition(parentScreen.getCentreX() - dialog->getWidth() / 2,
                                   parentScreen.getCentreY() - dialog->getHeight() / 2);
        dialog->setVisible(true);
    }
    else
    {
        dialog->setBounds(parent->getLocalBounds());
        parent->addAndMakeVisible(dialog);
    }

    dialog->toFront(true);
    dialog->enterModalState(true);
    dialog->grabKeyboardFocus();

    dialog->setDismissCallback([safe]() {
        if (auto* d = safe.getComponent())
        {
            d->exitModalState(0);
            delete d;
        }
    });
}
} // namespace

void showMessageDialog(const bws::ui::UiThemeResolved& theme, float scaleFactor, juce::String title,
                       juce::String message, juce::Component* parent, WeatherMessageDialogTone tone,
                       juce::String dismissLabel, bool desktopPromote)
{
    if (parent == nullptr)
        return;

    auto* dialog = new WeatherMessageDialog(theme, scaleFactor, std::move(title), std::move(message), tone,
                                            std::move(dismissLabel), []() {});
    attachAndShowDialog(dialog, parent, scaleFactor, desktopPromote);
}

void showLocalizedMessageDialog(const bws::ui::UiThemeResolved& theme, float scaleFactor,
                                bws::ui::localization::LocalizationManager& localization,
                                bws::ui::localization::LocalizedText title,
                                bws::ui::localization::LocalizedText message, juce::Component* parent,
                                WeatherMessageDialogTone tone, bws::ui::localization::LocalizedText dismissLabel,
                                bool desktopPromote)
{
    if (parent == nullptr)
        return;

    auto* dialog = new WeatherMessageDialog(theme, scaleFactor, &localization, std::move(title), std::move(message),
                                            tone, std::move(dismissLabel), []() {});
    attachAndShowDialog(dialog, parent, scaleFactor, desktopPromote);
}

} // namespace bws::weather

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#include "bw_ui/preset_system/WeatherSavePresetDialog.h"
#include "bw_ui/generated/BwTokens.h"

namespace bws::weather
{

namespace
{
constexpr int kCardWidthPx = 400;
constexpr int kCardHeightPx = 300;
constexpr int kPaddingPx = 20;

int sf(int px, float scaleFactor)
{
    return juce::roundToInt(static_cast<float>(px) * scaleFactor);
}
} // namespace

WeatherSavePresetDialog::WeatherSavePresetDialog(const bws::ui::UiThemeResolved& theme, float scaleFactor,
                                                 juce::String defaultName, juce::String defaultCategory,
                                                 OnCommit onCommit, std::function<void()> onDismiss)
    : WeatherSavePresetDialog(theme, scaleFactor, nullptr, std::move(defaultName), std::move(defaultCategory),
                              std::move(onCommit), std::move(onDismiss))
{}

WeatherSavePresetDialog::WeatherSavePresetDialog(const bws::ui::UiThemeResolved& theme, float scaleFactor,
                                                 bws::ui::localization::LocalizationManager* localization,
                                                 juce::String defaultName, juce::String defaultCategory,
                                                 OnCommit onCommit, std::function<void()> onDismiss)
    : theme_(theme)
    , scaleFactor_(scaleFactor)
    , localization_(localization)
    , titleText_(localization != nullptr ? bws::ui::localization::LocalizedText::key("dialog.savepreset.title")
                                         : bws::ui::localization::LocalizedText("Save Preset"))
    , subtitleText_(localization != nullptr
                        ? bws::ui::localization::LocalizedText::key("dialog.savepreset.subtitle")
                        : bws::ui::localization::LocalizedText("Save the current state as a user preset."))
    , nameFieldText_(localization != nullptr ? bws::ui::localization::LocalizedText::key("field.name")
                                             : bws::ui::localization::LocalizedText("Name"))
    , categoryFieldText_(localization != nullptr ? bws::ui::localization::LocalizedText::key("field.category")
                                                 : bws::ui::localization::LocalizedText("Category"))
    , cancelText_(localization != nullptr ? bws::ui::localization::LocalizedText::key("common.cancel")
                                          : bws::ui::localization::LocalizedText("Cancel"))
    , saveText_(localization != nullptr ? bws::ui::localization::LocalizedText::key("common.save")
                                        : bws::ui::localization::LocalizedText("Save"))
    , onCommit_(std::move(onCommit))
    , dismissCallback_(std::move(onDismiss))
{
    shell_.setTheme(theme_);
    shell_.setScaleFactor(scaleFactor_);
    addAndMakeVisible(shell_);

    titleLabel_.setFont(juce::Font(juce::FontOptions(18.0f * scaleFactor_, juce::Font::bold)));
    titleLabel_.setJustificationType(juce::Justification::centred);
    titleLabel_.setColour(juce::Label::textColourId, theme_.weatherColors.textBright);
    titleLabel_.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(titleLabel_);

    subtitleLabel_.setFont(juce::Font(juce::FontOptions(13.0f * scaleFactor_)));
    subtitleLabel_.setJustificationType(juce::Justification::centred);
    subtitleLabel_.setColour(juce::Label::textColourId, theme_.weatherColors.textSecondary);
    subtitleLabel_.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(subtitleLabel_);

    auto configureFieldLabel = [this](juce::Label& label, const juce::String& text) {
        label.setText(text, juce::dontSendNotification);
        label.setFont(juce::Font(juce::FontOptions(12.0f * scaleFactor_)));
        label.setJustificationType(juce::Justification::centredLeft);
        label.setColour(juce::Label::textColourId, theme_.weatherColors.textBright.withAlpha(
                                                       bws::tokens::shared::opacity::dialog::SECONDARY_TEXT));
        label.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(label);
    };

    configureFieldLabel(nameFieldLabel_, {});
    configureFieldLabel(categoryFieldLabel_, {});

    auto configureEditor = [this](juce::TextEditor& editor, const juce::String& defaultText) {
        editor.setMultiLine(false);
        editor.setReturnKeyStartsNewLine(false);
        editor.setColour(juce::TextEditor::backgroundColourId, theme_.weatherColors.bgInput);
        editor.setColour(juce::TextEditor::textColourId, theme_.weatherColors.textBright);
        editor.setColour(juce::TextEditor::outlineColourId, theme_.weatherColors.borderLight.withAlpha(
                                                                bws::tokens::shared::opacity::dialog::OUTLINE_SUBTLE));
        editor.setColour(juce::TextEditor::focusedOutlineColourId,
                         theme_.weatherColors.accent.withAlpha(bws::tokens::shared::opacity::dialog::OUTLINE_ACCENT));
        editor.setFont(juce::Font(juce::FontOptions(14.0f * scaleFactor_)));
        editor.setText(defaultText, juce::dontSendNotification);
        editor.onReturnKey = [this] {
            saveButton_.triggerClick();
        };
        editor.onEscapeKey = [this] {
            cancelButton_.triggerClick();
        };
        addAndMakeVisible(editor);
    };

    configureEditor(nameEditor_, defaultName);
    configureEditor(categoryEditor_, defaultCategory);

    // Live Save-button enabled state based on name non-emptiness.
    nameEditor_.onTextChange = [this] {
        updateSaveEnabled();
    };

    auto configureChip = [this](WeatherChipButton& b, const juce::String& text, std::function<void()> handler) {
        b.setTheme(theme_);
        b.setScaleFactor(scaleFactor_);
        b.setButtonText(text);
        b.onClick = std::move(handler);
        addAndMakeVisible(b);
    };

    configureChip(cancelButton_, {}, [this] { cancel(); });
    configureChip(saveButton_, {}, [this] { commit(); });

    if (localization_ != nullptr)
        localization_->addListener(*this);
    refreshText();

    setWantsKeyboardFocus(true);
    updateSaveEnabled();
    // Keyboard focus is requested after the dialog is parented and visible.
    // Grabbing focus during construction is a no-op because the component is
    // not on-screen yet.
}

WeatherSavePresetDialog::~WeatherSavePresetDialog()
{
    if (localization_ != nullptr)
        localization_->removeListener(*this);
}

void WeatherSavePresetDialog::refreshText()
{
    auto resolve = [this](const bws::ui::localization::LocalizedText& text) {
        return localization_ != nullptr ? localization_->resolve(text) : text.value();
    };

    titleLabel_.setText(resolve(titleText_), juce::dontSendNotification);
    subtitleLabel_.setText(resolve(subtitleText_), juce::dontSendNotification);
    nameFieldLabel_.setText(resolve(nameFieldText_), juce::dontSendNotification);
    categoryFieldLabel_.setText(resolve(categoryFieldText_), juce::dontSendNotification);
    cancelButton_.setButtonText(resolve(cancelText_));
    saveButton_.setButtonText(resolve(saveText_));
    resized();
    repaint();
}

void WeatherSavePresetDialog::grabInitialKeyboardFocus()
{
    nameEditor_.grabKeyboardFocus();
    nameEditor_.selectAll();
}

void WeatherSavePresetDialog::updateSaveEnabled()
{
    saveButton_.setEnabled(nameEditor_.getText().trim().isNotEmpty());
}

void WeatherSavePresetDialog::commit()
{
    const auto name = nameEditor_.getText().trim();
    if (name.isEmpty())
        return;

    const auto rawCategory = categoryEditor_.getText().trim();
    const auto category = rawCategory.isNotEmpty() ? rawCategory : juce::String("User");

    if (onCommit_)
        onCommit_(name, category);

    // Close after committing. dismissCallback_ is responsible for deletion.
    if (dismissCallback_)
        dismissCallback_();
}

void WeatherSavePresetDialog::cancel()
{
    if (dismissCallback_)
        dismissCallback_();
}

juce::Rectangle<int> WeatherSavePresetDialog::getCardBounds() const
{
    const int w = sf(kCardWidthPx, scaleFactor_);
    const int h = sf(kCardHeightPx, scaleFactor_);
    return {(getWidth() - w) / 2, (getHeight() - h) / 2, w, h};
}

void WeatherSavePresetDialog::paint(juce::Graphics& g)
{
    // Embedded overlay fills the plugin editor → paints a dark backdrop to
    // dim the plugin behind the card. Desktop-promoted (Reaper path, see
    // showSavePresetDialog helper) → dialog IS the card; shell paints it,
    // no backdrop needed. isOnDesktop() is a stock JUCE query reporting
    // whether the component has its own OS-level peer.
    if (!isOnDesktop())
        g.fillAll(juce::Colours::black.withAlpha(bws::tokens::shared::opacity::dialog::BACKDROP));
}

void WeatherSavePresetDialog::resized()
{
    const auto card = getCardBounds();
    shell_.setBounds(card);

    const int padding = sf(kPaddingPx, scaleFactor_);
    const int contentLeft = card.getX() + padding;
    const int contentWidth = card.getWidth() - 2 * padding;

    int y = card.getY() + sf(18, scaleFactor_);

    titleLabel_.setBounds(contentLeft, y, contentWidth, sf(28, scaleFactor_));
    y += sf(32, scaleFactor_);

    subtitleLabel_.setBounds(contentLeft, y, contentWidth, sf(20, scaleFactor_));
    y += sf(28, scaleFactor_);

    const int editorHeight = sf(32, scaleFactor_);
    const int labelHeight = sf(18, scaleFactor_);

    nameFieldLabel_.setBounds(contentLeft, y, contentWidth, labelHeight);
    y += labelHeight;
    nameEditor_.setBounds(contentLeft, y, contentWidth, editorHeight);
    y += editorHeight + sf(10, scaleFactor_);

    categoryFieldLabel_.setBounds(contentLeft, y, contentWidth, labelHeight);
    y += labelHeight;
    categoryEditor_.setBounds(contentLeft, y, contentWidth, editorHeight);

    // Button row pinned to bottom.
    const int buttonHeight = sf(30, scaleFactor_);
    const int buttonWidth = sf(88, scaleFactor_);
    const int buttonGap = sf(12, scaleFactor_);
    const int buttonY = card.getBottom() - buttonHeight - sf(18, scaleFactor_);
    const int pairWidth = 2 * buttonWidth + buttonGap;
    const int buttonLeft = card.getCentreX() - pairWidth / 2;

    cancelButton_.setBounds(buttonLeft, buttonY, buttonWidth, buttonHeight);
    saveButton_.setBounds(buttonLeft + buttonWidth + buttonGap, buttonY, buttonWidth, buttonHeight);
}

void WeatherSavePresetDialog::mouseDown(const juce::MouseEvent& e)
{
    if (!getCardBounds().contains(e.getPosition()))
        cancel();
}

bool WeatherSavePresetDialog::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        cancel();
        return true;
    }
    if (key == juce::KeyPress::returnKey)
    {
        if (saveButton_.isEnabled())
            saveButton_.triggerClick();
        return true;
    }
    // Absorb any other key that reaches us - keyPressed only fires for keys
    // NOT consumed by the focused child (TextEditor handles alphanumerics +
    // arrows; WeatherChipButton handles space/return). Returning true here
    // prevents stray keys from bubbling out of the modal dialog to the host
    // (defense-in-depth against Reaper's transport-spacebar hijack).
    return true;
}

void WeatherSavePresetDialog::localizationLanguageChanged()
{
    refreshText();
}

// ── Free helper ───────────────────────────────────────────────────────────

namespace
{
void attachAndShowSavePresetDialog(WeatherSavePresetDialog* dialog, float scaleFactor, juce::Component* parent,
                                   bool desktopPromote)
{
    if (parent == nullptr)
        return;

    juce::Component::SafePointer<WeatherSavePresetDialog> safe(dialog);

    if (desktopPromote)
    {
        // Desktop-promote the dialog (same mechanism juce::AlertWindow uses).
        // Required in Reaper on macOS to bypass host-level spacebar hijack -
        // top-level NSWindow is outside Reaper's window-hierarchy key-binding
        // scope, so the TextEditor receives typed spaces normally. Trade-off:
        // no dark backdrop dimming the plugin behind the dialog. See
        //  for the full
        // landscape + why embedded overlay is preferred when the host doesn't
        // hijack.
        const int cardW = sf(kCardWidthPx, scaleFactor);
        const int cardH = sf(kCardHeightPx, scaleFactor);
        dialog->setSize(cardW, cardH);
        dialog->addToDesktop(juce::ComponentPeer::windowIsTemporary | juce::ComponentPeer::windowHasDropShadow);
        // Center over the parent's screen-space bounds. `centreAroundComponent`
        // is not a base juce::Component method; computing the top-left from
        // parent screen bounds + card dimensions is the equivalent.
        const auto parentScreen = parent->getScreenBounds();
        dialog->setTopLeftPosition(parentScreen.getCentreX() - cardW / 2, parentScreen.getCentreY() - cardH / 2);
        dialog->setVisible(true);
    }
    else
    {
        // Embedded overlay - fills the plugin editor, paints a dark backdrop
        // dimming the plugin behind the card. Correct default for every
        // tested DAW except Reaper.
        dialog->setBounds(parent->getLocalBounds());
        parent->addAndMakeVisible(dialog);
    }
    dialog->toFront(true);
    // Modal, but do NOT grab keyboard focus onto the dialog component itself
    // - we want the Name TextEditor to own focus so typing (including space)
    // goes to the editor, not to the modal root. grabInitialKeyboardFocus()
    // routes focus onto nameEditor_ now that the component is on-screen.
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
} // namespace

void showSavePresetDialog(const bws::ui::UiThemeResolved& theme, float scaleFactor, juce::String defaultName,
                          juce::String defaultCategory, WeatherSavePresetDialog::OnCommit onCommit,
                          juce::Component* parent, bool desktopPromote)
{
    if (parent == nullptr)
        return;

    auto* dialog = new WeatherSavePresetDialog(theme, scaleFactor, std::move(defaultName), std::move(defaultCategory),
                                               std::move(onCommit), []() {});
    attachAndShowSavePresetDialog(dialog, scaleFactor, parent, desktopPromote);
}

void showLocalizedSavePresetDialog(const bws::ui::UiThemeResolved& theme, float scaleFactor,
                                   bws::ui::localization::LocalizationManager& localization, juce::String defaultName,
                                   juce::String defaultCategory, WeatherSavePresetDialog::OnCommit onCommit,
                                   juce::Component* parent, bool desktopPromote)
{
    if (parent == nullptr)
        return;

    auto* dialog = new WeatherSavePresetDialog(theme, scaleFactor, &localization, std::move(defaultName),
                                               std::move(defaultCategory), std::move(onCommit), []() {});
    attachAndShowSavePresetDialog(dialog, scaleFactor, parent, desktopPromote);
}

} // namespace bws::weather

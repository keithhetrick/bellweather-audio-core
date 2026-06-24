// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#include "bw_ui/preset_system/WeatherManagePresetsDialog.h"
#include "bw_ui/generated/BwTokens.h"

namespace bws::weather
{

namespace
{
constexpr int kCardWidthPx = 420;
constexpr int kCardHeightPx = 400;
constexpr int kPaddingPx = 20;
constexpr int kRowHeightPx = 40;

int sf(int px, float scaleFactor)
{
    return juce::roundToInt(static_cast<float>(px) * scaleFactor);
}

} // namespace

WeatherManagePresetsDialog::WeatherManagePresetsDialog(WeatherPresetManager& presetManager,
                                                       const bws::ui::UiThemeResolved& theme, float scaleFactor,
                                                       std::function<void()> onDismiss)
    : WeatherManagePresetsDialog(presetManager, theme, scaleFactor, nullptr, std::move(onDismiss))
{}

WeatherManagePresetsDialog::WeatherManagePresetsDialog(WeatherPresetManager& presetManager,
                                                       const bws::ui::UiThemeResolved& theme, float scaleFactor,
                                                       bws::ui::localization::LocalizationManager* localization,
                                                       std::function<void()> onDismiss)
    : controller_(presetManager)
    , theme_(theme)
    , scaleFactor_(scaleFactor)
    , localization_(localization)
    , dismissCallback_(std::move(onDismiss))
{
    controller_.onOpen(); // Contract §B1: cancel any active preview session

    // Shell - the card chrome.
    shell_.setTheme(theme_);
    shell_.setScaleFactor(scaleFactor_);
    addAndMakeVisible(shell_);

    // Title + mode prompt labels.
    titleLabel_.setFont(juce::Font(juce::FontOptions(18.0f * scaleFactor_, juce::Font::bold)));
    titleLabel_.setJustificationType(juce::Justification::centred);
    titleLabel_.setColour(juce::Label::textColourId, theme_.weatherColors.textBright);
    titleLabel_.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(titleLabel_);

    modePromptLabel_.setFont(juce::Font(juce::FontOptions(14.0f * scaleFactor_)));
    modePromptLabel_.setJustificationType(juce::Justification::centred);
    modePromptLabel_.setColour(juce::Label::textColourId, theme_.weatherColors.textSecondary);
    modePromptLabel_.setInterceptsMouseClicks(false, false);
    modePromptLabel_.setMinimumHorizontalScale(0.75f);
    addChildComponent(modePromptLabel_);

    // ListBox - transparent bg + no outline; shell provides the boundary.
    presetList_.setRowHeight(sf(kRowHeightPx, scaleFactor_));
    presetList_.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    presetList_.setColour(juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    presetList_.setOutlineThickness(0);
    addChildComponent(presetList_);

    auto wireChip = [this](WeatherChipButton& b, std::function<void()> handler) {
        b.setTheme(theme_);
        b.setScaleFactor(scaleFactor_);
        b.onClick = std::move(handler);
        addChildComponent(b);
    };

    wireChip(updateButton_, [this] {
        if (getSelectedRow() >= 0)
            enterMode(Mode::ConfirmUpdate);
    });
    wireChip(renameButton_, [this] {
        if (getSelectedRow() >= 0)
            enterMode(Mode::Rename);
    });
    wireChip(deleteButton_, [this] {
        if (getSelectedRow() >= 0)
            enterMode(Mode::ConfirmDelete);
    });
    wireChip(closeButton_, [this] { dismiss(); });

    wireChip(confirmYesButton_, [this] {
        switch (mode_)
        {
        case Mode::ConfirmDelete:
            commitDelete();
            break;
        case Mode::Rename:
            commitRename();
            break;
        case Mode::ConfirmUpdate:
            commitUpdate();
            break;
        default:
            break;
        }
    });
    wireChip(confirmNoButton_, [this] { enterMode(Mode::Main); });

    // Rename editor - themed to match browser search field.
    renameEditor_.setMultiLine(false);
    renameEditor_.setReturnKeyStartsNewLine(false);
    renameEditor_.setColour(juce::TextEditor::backgroundColourId, theme_.weatherColors.bgInput);
    renameEditor_.setColour(juce::TextEditor::textColourId, theme_.weatherColors.textBright);
    renameEditor_.setColour(
        juce::TextEditor::outlineColourId,
        theme_.weatherColors.borderLight.withAlpha(bws::tokens::shared::opacity::dialog::OUTLINE_SUBTLE));
    renameEditor_.setColour(
        juce::TextEditor::focusedOutlineColourId,
        theme_.weatherColors.accent.withAlpha(bws::tokens::shared::opacity::dialog::OUTLINE_ACCENT));
    renameEditor_.setFont(juce::Font(juce::FontOptions(14.0f * scaleFactor_)));
    renameEditor_.onReturnKey = [this] {
        confirmYesButton_.triggerClick();
    };
    renameEditor_.onEscapeKey = [this] {
        confirmNoButton_.triggerClick();
    };
    addChildComponent(renameEditor_);

    setWantsKeyboardFocus(true);
    if (localization_ != nullptr)
        localization_->addListener(*this);
    refreshList();
    enterMode(Mode::Main);
}

WeatherManagePresetsDialog::~WeatherManagePresetsDialog()
{
    if (localization_ != nullptr)
        localization_->removeListener(*this);
}

void WeatherManagePresetsDialog::refreshText()
{
    enterMode(mode_);
}

void WeatherManagePresetsDialog::localizationLanguageChanged()
{
    refreshText();
}

juce::String tr(bws::ui::localization::LocalizationManager* localization, const char* key, const char* fallback)
{
    return localization != nullptr ? localization->get(key) : juce::String(fallback);
}

juce::String formatPresetPrompt(bws::ui::localization::LocalizationManager* localization, const char* key,
                                const char* fallback, const juce::String& presetName)
{
    if (localization == nullptr)
        return juce::String(fallback).replace("{presetName}", presetName);

    bws::ui::localization::PlaceholderMap placeholders;
    placeholders.emplace("presetName", presetName);
    return localization->format(key, placeholders);
}

void WeatherManagePresetsDialog::refreshList()
{
    presets_ = controller_.listUserPresets();
    presetList_.updateContent();
    if (!presets_.empty())
        presetList_.selectRow(0);
}

void WeatherManagePresetsDialog::enterMode(Mode m)
{
    mode_ = m;
    const bool main = (m == Mode::Main);
    const bool confirmDelete = (m == Mode::ConfirmDelete);
    const bool rename = (m == Mode::Rename);
    const bool confirmUpdate = (m == Mode::ConfirmUpdate);

    presetList_.setVisible(main);
    updateButton_.setVisible(main);
    renameButton_.setVisible(main);
    deleteButton_.setVisible(main);
    closeButton_.setVisible(main);

    const bool anyConfirm = confirmDelete || rename || confirmUpdate;
    confirmYesButton_.setVisible(anyConfirm);
    confirmNoButton_.setVisible(anyConfirm);
    renameEditor_.setVisible(rename);
    modePromptLabel_.setVisible(!main);

    updateButton_.setButtonText(tr(localization_, "action.update", "Update"));
    renameButton_.setButtonText(tr(localization_, "action.rename", "Rename"));
    deleteButton_.setButtonText(tr(localization_, "action.delete", "Delete"));
    closeButton_.setButtonText(tr(localization_, "common.close", "Close"));
    confirmNoButton_.setButtonText(tr(localization_, "common.cancel", "Cancel"));

    confirmYesButton_.setButtonText(confirmDelete   ? tr(localization_, "action.delete", "Delete")
                                    : rename        ? tr(localization_, "action.rename", "Rename")
                                    : confirmUpdate ? tr(localization_, "action.update", "Update")
                                                    : tr(localization_, "common.ok", "OK"));

    switch (mode_)
    {
    case Mode::Main:
        titleLabel_.setText(tr(localization_, "dialog.managepresets.title.main", "Manage User Presets"),
                            juce::dontSendNotification);
        break;
    case Mode::ConfirmDelete:
        titleLabel_.setText(tr(localization_, "dialog.managepresets.title.delete", "Delete Preset?"),
                            juce::dontSendNotification);
        break;
    case Mode::Rename:
        titleLabel_.setText(tr(localization_, "dialog.managepresets.title.rename", "Rename Preset"),
                            juce::dontSendNotification);
        break;
    case Mode::ConfirmUpdate:
        titleLabel_.setText(tr(localization_, "dialog.managepresets.title.update", "Update Preset?"),
                            juce::dontSendNotification);
        break;
    }

    if (!main)
    {
        const auto name = selectedPresetName();
        juce::String prompt;
        switch (m)
        {
        case Mode::ConfirmDelete:
            prompt = formatPresetPrompt(localization_, "dialog.managepresets.prompt.delete",
                                        "Delete '{presetName}'?\nThis cannot be undone.", name);
            break;
        case Mode::Rename:
            prompt = formatPresetPrompt(localization_, "dialog.managepresets.prompt.rename",
                                        "Enter a new name for '{presetName}':", name);
            break;
        case Mode::ConfirmUpdate:
            prompt = formatPresetPrompt(localization_, "dialog.managepresets.prompt.update",
                                        "Overwrite '{presetName}' with current settings?", name);
            break;
        default:
            break;
        }
        modePromptLabel_.setText(prompt, juce::dontSendNotification);
    }

    if (rename)
    {
        renameEditor_.setText(selectedPresetName(), juce::dontSendNotification);
        renameEditor_.selectAll();
        renameEditor_.grabKeyboardFocus();
    }
    else
    {
        grabKeyboardFocus();
    }

    const bool hasSelection = getSelectedRow() >= 0;
    updateButton_.setEnabled(hasSelection);
    renameButton_.setEnabled(hasSelection);
    deleteButton_.setEnabled(hasSelection);

    resized();
    repaint();
}

int WeatherManagePresetsDialog::getSelectedRow() const
{
    return presetList_.getSelectedRow();
}

juce::String WeatherManagePresetsDialog::selectedPresetName() const
{
    const int row = getSelectedRow();
    if (row < 0 || row >= static_cast<int>(presets_.size()))
        return {};
    return presets_[static_cast<size_t>(row)].name;
}

void WeatherManagePresetsDialog::commitDelete()
{
    const auto name = selectedPresetName();
    if (name.isNotEmpty())
        controller_.requestDelete(name);
    refreshList();
    if (presets_.empty())
        dismiss();
    else
        enterMode(Mode::Main);
}

void WeatherManagePresetsDialog::commitRename()
{
    const auto oldName = selectedPresetName();
    const auto newName = renameEditor_.getText();
    if (oldName.isNotEmpty())
        controller_.requestRename(oldName, newName);
    refreshList();
    enterMode(Mode::Main);
}

void WeatherManagePresetsDialog::commitUpdate()
{
    const auto name = selectedPresetName();
    if (name.isNotEmpty())
        controller_.requestUpdate(name);
    refreshList();
    enterMode(Mode::Main);
}

void WeatherManagePresetsDialog::dismiss()
{
    if (dismissCallback_)
        dismissCallback_();
}

juce::Rectangle<int> WeatherManagePresetsDialog::getCardBounds() const
{
    const int w = sf(kCardWidthPx, scaleFactor_);
    const int h = sf(kCardHeightPx, scaleFactor_);
    return {(getWidth() - w) / 2, (getHeight() - h) / 2, w, h};
}

void WeatherManagePresetsDialog::paint(juce::Graphics& g)
{
    // Backdrop only - shell paints the card chrome.
    g.fillAll(juce::Colours::black.withAlpha(bws::tokens::shared::opacity::dialog::BACKDROP));
}

void WeatherManagePresetsDialog::resized()
{
    const auto card = getCardBounds();
    shell_.setBounds(card);

    const int padding = sf(kPaddingPx, scaleFactor_);
    const int contentLeft = card.getX() + padding;
    const int contentRight = card.getRight() - padding;
    const int contentWidth = contentRight - contentLeft;

    const int titleHeight = sf(28, scaleFactor_);
    const int titleTop = card.getY() + sf(18, scaleFactor_);
    titleLabel_.setBounds(contentLeft, titleTop, contentWidth, titleHeight);

    const int buttonHeight = sf(30, scaleFactor_);
    const int buttonWidth = sf(80, scaleFactor_);
    const int buttonRowY = card.getBottom() - buttonHeight - sf(18, scaleFactor_);

    const int bodyTop = titleTop + titleHeight + sf(10, scaleFactor_);
    const int bodyBottom = buttonRowY - sf(12, scaleFactor_);

    presetList_.setBounds(contentLeft, bodyTop, contentWidth, bodyBottom - bodyTop);

    // Main-mode buttons: four evenly spaced.
    const int mainGap = (contentWidth - 4 * buttonWidth) / 3;
    int x = contentLeft;
    for (auto* b : {&updateButton_, &renameButton_, &deleteButton_, &closeButton_})
    {
        b->setBounds(x, buttonRowY, buttonWidth, buttonHeight);
        x += buttonWidth + mainGap;
    }

    // Confirm-mode buttons: Yes / No centered.
    const int confirmGap = sf(16, scaleFactor_);
    const int confirmPairWidth = 2 * buttonWidth + confirmGap;
    const int confirmLeft = card.getCentreX() - confirmPairWidth / 2;
    confirmYesButton_.setBounds(confirmLeft, buttonRowY, buttonWidth, buttonHeight);
    confirmNoButton_.setBounds(confirmLeft + buttonWidth + confirmGap, buttonRowY, buttonWidth, buttonHeight);

    // Mode prompt + rename editor share the body area above the button row.
    const int promptY = bodyTop + sf(10, scaleFactor_);
    const int promptHeight = sf(52, scaleFactor_);
    modePromptLabel_.setBounds(contentLeft, promptY, contentWidth, promptHeight);

    const int renameY = promptY + promptHeight + sf(8, scaleFactor_);
    const int renameHeight = sf(32, scaleFactor_);
    renameEditor_.setBounds(contentLeft, renameY, contentWidth, renameHeight);
}

void WeatherManagePresetsDialog::mouseDown(const juce::MouseEvent& e)
{
    // Click outside the card dismisses (Main mode only - avoid accidental
    // loss of a half-typed rename or a hovered confirmation).
    if (mode_ == Mode::Main && !getCardBounds().contains(e.getPosition()))
        dismiss();
}

bool WeatherManagePresetsDialog::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        if (mode_ == Mode::Main)
            dismiss();
        else
            enterMode(Mode::Main);
        return true;
    }
    // Absorb any other key that reaches us - keyPressed only fires for keys
    // NOT consumed by the focused child (rename editor handles text entry in
    // Rename mode; chip buttons handle space/return). Preventing stray keys
    // from bubbling out stops host-level keyboard bindings (e.g., Reaper's
    // spacebar-transport) from firing while the dialog is open.
    return true;
}

// ── ListBoxModel ──────────────────────────────────────────────────────────

int WeatherManagePresetsDialog::getNumRows()
{
    return static_cast<int>(presets_.size());
}

void WeatherManagePresetsDialog::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height,
                                                  bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= static_cast<int>(presets_.size()))
        return;

    const auto& info = presets_[static_cast<size_t>(rowNumber)];

    // Render using WeatherLibraryRow's paint - gold-accent selection highlight
    // + browser-matching typography. Transient component; no children.
    WeatherLibraryRow row;
    row.setTheme(theme_);
    row.setScaleFactor(scaleFactor_);
    row.setDensity(LibraryRowDensity::Default);
    row.setModel({
        info.name,     // title
        info.category, // metadata subtitle
        {},            // badges - Manage doesn't use them
        rowIsSelected, // selected
        false,         // highlighted
        false,         // favourite
        false          // favouriteEnabled - Manage isn't favourite surface
    });
    row.setBounds(0, 0, width, height);
    row.paintEntireComponent(g, false);
}

void WeatherManagePresetsDialog::listBoxItemClicked(int /*row*/, const juce::MouseEvent& /*e*/)
{
    // Selection updates enable-state of the main-mode buttons.
    updateButton_.setEnabled(true);
    renameButton_.setEnabled(true);
    deleteButton_.setEnabled(true);
}

// ── Free helpers ──────────────────────────────────────────────────────────

namespace
{
void attachAndShowManagePresetsDialog(WeatherManagePresetsDialog* dialog, juce::Component* parent)
{
    if (parent == nullptr)
        return;

    juce::Component::SafePointer<WeatherManagePresetsDialog> safe(dialog);

    dialog->setBounds(parent->getLocalBounds());
    parent->addAndMakeVisible(dialog);
    dialog->toFront(true);
    dialog->grabKeyboardFocus();

    dialog->setDismissCallback([safe]() {
        if (auto* d = safe.getComponent())
            delete d;
    });
}
} // namespace

void showManagePresetsDialog(WeatherPresetManager& presetManager, const bws::ui::UiThemeResolved& theme,
                             float scaleFactor, juce::Component* parent)
{
    if (parent == nullptr)
        return;

    auto* dialog = new WeatherManagePresetsDialog(presetManager, theme, scaleFactor, []() {});
    attachAndShowManagePresetsDialog(dialog, parent);
}

void showLocalizedManagePresetsDialog(WeatherPresetManager& presetManager, const bws::ui::UiThemeResolved& theme,
                                      float scaleFactor, bws::ui::localization::LocalizationManager& localization,
                                      juce::Component* parent)
{
    if (parent == nullptr)
        return;

    auto* dialog = new WeatherManagePresetsDialog(presetManager, theme, scaleFactor, &localization, []() {});
    attachAndShowManagePresetsDialog(dialog, parent);
}

void appendManagePresetsMenuItem(juce::PopupMenu& menu, WeatherPresetManager& presetManager,
                                 const bws::ui::UiThemeResolved& theme, float scaleFactor, juce::Component* parent)
{
    juce::Component::SafePointer<juce::Component> safeParent(parent);
    menu.addItem("Manage User Presets...", presetManager.getUserPresetCount() > 0, false,
                 [&presetManager, &theme, scaleFactor, safeParent]() {
                     if (auto* p = safeParent.getComponent())
                         showManagePresetsDialog(presetManager, theme, scaleFactor, p);
                 });
}

void appendLocalizedManagePresetsMenuItem(juce::PopupMenu& menu, WeatherPresetManager& presetManager,
                                          const bws::ui::UiThemeResolved& theme, float scaleFactor,
                                          bws::ui::localization::LocalizationManager& localization,
                                          juce::Component* parent)
{
    juce::Component::SafePointer<juce::Component> safeParent(parent);
    menu.addItem(localization.get("dialog.managepresets.menuitem"), presetManager.getUserPresetCount() > 0, false,
                 [&presetManager, &theme, scaleFactor, &localization, safeParent]() {
                     if (auto* p = safeParent.getComponent())
                         showLocalizedManagePresetsDialog(presetManager, theme, scaleFactor, localization, p);
                 });
}

} // namespace bws::weather

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "bw_ui/adapters/UiThemeKernelAdapter.h"
#include "bw_ui/foundation/UiTheme.h"

namespace bws::ui
{

enum class ReadoutUnit
{
    Plain,
    Decibels,
    Percent
};

class UiReadout : public juce::Component
{
public:
    enum class EditCommitTrigger
    {
        EnterKey,
        FocusLost
    };

    enum class CompactAppearance
    {
        DefaultCompact,
        CompanionCompact
    };

    enum class Emphasis
    {
        Normal,
        Hovered,
        Active
    };

    UiReadout(UiReadoutRole role, const UiThemeResolved& theme);

    void setTheme(const UiThemeResolved& newTheme);
    void setRole(UiReadoutRole newRole);
    UiReadoutRole getRole() const noexcept { return role; }
    void setCompactAppearance(CompactAppearance newAppearance);
    CompactAppearance getCompactAppearance() const noexcept { return compactAppearance; }
    void setScale(float newScale);
    void setUnit(ReadoutUnit newUnit);
    void setEmphasis(Emphasis newEmphasis);
    Emphasis getEmphasis() const noexcept { return emphasis; }
    /// Render value text in the theme accent instead of text0.
    void setAccentText(bool shouldUseAccent);
    bool isAccentText() const noexcept { return accentText; }
    void setText(const juce::String& newText);
    void setValue(float value);
    const juce::String& getText() const noexcept { return text; }

    /// Size the font to fit `widest` and render every value at that size, so the
    /// readout does not refit per value. Empty = fit-to-current-text.
    void setWidthReservationText(const juce::String& widest);
    const juce::String& getWidthReservationText() const noexcept { return widthReservationText_; }
    void setEditable(bool shouldAllowEdit);
    bool isEditable() const noexcept { return editable; }
    bool isEditing() const noexcept { return editing; }
    bool requestBeginEditing();
    void cancelEditing();
    void commitEditing(EditCommitTrigger trigger);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

    void testSetEditText(const juce::String& newText);
    bool testIsEditorPopupEnabled() const;

    std::function<void(bool)> onHoverChanged;
    std::function<juce::String()> onRequestEditSeedText;
    std::function<bool(const juce::String&, EditCommitTrigger)> onCommitEditText;
    std::function<void(bool)> onEditingChanged;

private:
    class HiddenTextEditor : public juce::TextEditor
    {
    public:
        std::function<void()> onVisualStateChanged;

        bool keyPressed(const juce::KeyPress& key) override
        {
            const bool handled = juce::TextEditor::keyPressed(key);
            if (onVisualStateChanged)
                onVisualStateChanged();
            return handled;
        }

        void focusGained(FocusChangeType cause) override
        {
            juce::TextEditor::focusGained(cause);
            if (onVisualStateChanged)
                onVisualStateChanged();
        }

        void focusLost(FocusChangeType cause) override
        {
            juce::TextEditor::focusLost(cause);
            if (onVisualStateChanged)
                onVisualStateChanged();
        }
    };

    juce::String formatValue(float value) const;
    void beginEditingInternal();
    void endEditing(bool committedSuccessfully);
    void resolveDeferredFocusLoss(int sessionId);
    void finishEditing(bool committedSuccessfully);
    void updateEditorStyle();

    UiReadoutRole role;
    UiThemeResolved theme;
    kernel::ThemeSnapshot kernelTheme;
    juce::String text;
    juce::String widthReservationText_;
    float lastValue {0.0f};
    float scale {1.0f};
    ReadoutUnit unit {ReadoutUnit::Plain};
    CompactAppearance compactAppearance {CompactAppearance::DefaultCompact};
    Emphasis emphasis {Emphasis::Normal};
    bool editable {false};
    bool editing {false};
    bool accentText {false};
    bool resolvingEditSession {false};
    int editSessionId {0};
    HiddenTextEditor editor;
};

} // namespace bws::ui

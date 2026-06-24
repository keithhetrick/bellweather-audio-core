// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include <cmath>
#include "bw_ui/Components/UiReadout.h"
#include "bw_ui/generated/BwTokens.h"
#include "bw_ui/Components/UiReadoutGeometry.h"
#include "bw_ui/rendering/JuceRenderer.h"

namespace bws::ui
{
namespace
{
float resolveCompanionReadoutEmphasis(const UiThemeResolved& theme,
                                      UiReadout::CompactAppearance compactAppearance) noexcept
{
    if (compactAppearance != UiReadout::CompactAppearance::CompanionCompact)
        return 1.0f;

    return juce::jlimit(0.9f, 1.15f, theme.secondaryKnobStyle.readoutEmphasis);
}

float measureTextWidth(const juce::Font& font, const juce::String& text) noexcept
{
    if (text.isEmpty())
        return 0.0f;

    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(font, text, 0.0f, 0.0f);
    return glyphs.getBoundingBox(0, text.length(), true).getWidth();
}

void drawTextRun(juce::Graphics& g, const juce::Font& font, juce::Colour colour, const juce::String& text, float x,
                 const juce::Rectangle<int>& bounds)
{
    if (text.isEmpty())
        return;

    const int width = (int)std::ceil(measureTextWidth(font, text) + 2.0f);
    g.setColour(colour);
    g.setFont(font);
    g.drawText(text, juce::Rectangle<int>((int)std::floor(x), bounds.getY(), juce::jmax(1, width), bounds.getHeight()),
               juce::Justification::centredLeft, false);
}
} // namespace

UiReadout::UiReadout(UiReadoutRole roleIn, const UiThemeResolved& themeIn)
    : role(roleIn)
    , theme(themeIn)
    , kernelTheme(adapters::makeKernelThemeSnapshot(themeIn))
{
    editor.setMultiLine(false);
    editor.setReturnKeyStartsNewLine(false);
    editor.setEscapeAndReturnKeysConsumed(true);
    editor.setJustification(juce::Justification::centred);
    editor.setScrollbarsShown(false);
    editor.setPopupMenuEnabled(false);
    editor.setSelectAllWhenFocused(true);
    editor.setWantsKeyboardFocus(true);
    editor.setVisible(false);
    editor.setInterceptsMouseClicks(false, false);
    editor.setAlpha(0.0f);
    editor.onReturnKey = [this] {
        commitEditing(EditCommitTrigger::EnterKey);
    };
    editor.onEscapeKey = [this] {
        cancelEditing();
    };
    editor.onTextChange = [this] {
        repaint();
    };
    editor.onVisualStateChanged = [this] {
        repaint();
    };
    editor.onFocusLost = [this] {
        if (editing)
            resolveDeferredFocusLoss(editSessionId);
    };
    addAndMakeVisible(editor);
    updateEditorStyle();
}

void UiReadout::setTheme(const UiThemeResolved& newTheme)
{
    theme = newTheme;
    kernelTheme = adapters::makeKernelThemeSnapshot(newTheme);
    updateEditorStyle();
    repaint();
}

void UiReadout::setRole(UiReadoutRole newRole)
{
    role = newRole;
    updateEditorStyle();
    repaint();
}

void UiReadout::setCompactAppearance(CompactAppearance newAppearance)
{
    if (compactAppearance == newAppearance)
        return;

    compactAppearance = newAppearance;
    if (compactAppearance != CompactAppearance::CompanionCompact)
        setEditable(false);
    repaint();
}

void UiReadout::setScale(float newScale)
{
    scale = newScale;
    updateEditorStyle();
    resized();
    repaint();
}

void UiReadout::setUnit(ReadoutUnit newUnit)
{
    unit = newUnit;
    repaint();
}

void UiReadout::setEmphasis(Emphasis newEmphasis)
{
    if (emphasis == newEmphasis)
        return;

    emphasis = newEmphasis;
    repaint();
}

void UiReadout::setText(const juce::String& newText)
{
    text = newText;
    if (!editing)
        editor.setText(newText, juce::dontSendNotification);
    repaint();
}

void UiReadout::setValue(float value)
{
    lastValue = value;
    text = formatValue(value);
    if (!editing)
        editor.setText(text, juce::dontSendNotification);
    repaint();
}

void UiReadout::setEditable(bool shouldAllowEdit)
{
    const bool nextEditable =
        shouldAllowEdit && role == UiReadoutRole::Compact && compactAppearance == CompactAppearance::CompanionCompact;
    if (editable == nextEditable)
        return;

    editable = nextEditable;
    if (!editable)
        cancelEditing();

    setMouseCursor(editable ? juce::MouseCursor::IBeamCursor : juce::MouseCursor::NormalCursor);
}

void UiReadout::cancelEditing()
{
    if (!editing || resolvingEditSession)
        return;

    resolvingEditSession = true;
    editor.setText(text, juce::dontSendNotification);
    finishEditing(false);
}

void UiReadout::commitEditing(EditCommitTrigger trigger)
{
    if (!editing || resolvingEditSession)
        return;

    resolvingEditSession = true;
    const auto candidate = editor.getText().trim();
    const bool committed = onCommitEditText ? onCommitEditText(candidate, trigger) : false;
    if (!committed)
        editor.setText(text, juce::dontSendNotification);
    finishEditing(committed);
}

juce::String UiReadout::formatValue(float value) const
{
    const auto metrics = UiReadouts::metrics(role, scale, theme);
    int decimals = (metrics.decimals == ReadoutDecimalPolicy::Auto1 ? 1 : 2);
    const float absValue = std::abs(value);
    if (metrics.decimals == ReadoutDecimalPolicy::Auto1 && absValue < 1.0f)
        decimals = 2;

    juce::String suffix;
    switch (unit)
    {
    case ReadoutUnit::Decibels:
        suffix = " dB";
        break;
    case ReadoutUnit::Percent:
        suffix = " %";
        break;
    case ReadoutUnit::Plain:
    default:
        break;
    }
    return juce::String(value, decimals) + suffix;
}

void UiReadout::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto metrics = UiReadouts::metrics(role, scale, theme);

    auto pillBounds = bounds.reduced(bws::tokens::shared::readout_derivation::PILL_INSET);
    const bool compact = role == UiReadoutRole::Compact;
    const auto geo = resolveReadoutGeometry(metrics, compact);
    const float readoutEmphasis = resolveCompanionReadoutEmphasis(theme, compactAppearance);

    namespace opr = bws::tokens::shared::opacity::ui_readout;
    auto fill = theme.colors.bg2.withAlpha(opr::FILL_NORMAL);
    auto outline = theme.colors.outline0.withAlpha(bws::tokens::shared::opacity::outline::STANDARD);
    auto textColour = theme.colors.text0.withAlpha(
        compact ? juce::jlimit(0.0f, 1.0f, opr::TEXT_COMPACT_BASE * readoutEmphasis) : 1.0f);
    auto underline = theme.colors.outline0.withAlpha(
        compact ? juce::jlimit(0.0f, 1.0f, opr::UNDERLINE_COMPACT_BASE * readoutEmphasis) : 0.0f);

    switch (emphasis)
    {
    case Emphasis::Hovered:
        fill = fill.brighter(0.02f).withAlpha(opr::FILL_HOVER);
        outline = theme.colors.accent0.withAlpha(opr::OUTLINE_HOVER);
        textColour = theme.colors.text0;
        underline = compact ? theme.weatherColors.surfaceHighlight.interpolatedWith(theme.colors.accent0, 0.35f)
                                  .withAlpha(juce::jlimit(0.0f, 1.0f, opr::UNDERLINE_HOVER_COEFF * readoutEmphasis))
                            : underline;
        break;
    case Emphasis::Active:
        fill = theme.colors.bg2.brighter(0.04f).withAlpha(opr::FILL_ACTIVE);
        outline = theme.colors.accent0.withAlpha(opr::OUTLINE_ACTIVE);
        textColour = theme.colors.text0;
        underline = compact ? theme.weatherColors.surfaceHighlight.interpolatedWith(theme.colors.accent0, 0.22f)
                                  .withAlpha(juce::jlimit(0.0f, 1.0f, opr::UNDERLINE_ACTIVE_COEFF * readoutEmphasis))
                            : underline;
        break;
    case Emphasis::Normal:
        break;
    }

    if (compact && editing)
    {
        auto font = adapters::makeFont(kernelTheme, kernel::TextRole::Readout, scale);
        const float maxFontHeight = juce::jmax(1.0f, pillBounds.getHeight() - geo.verticalSafety);
        font.setHeight(juce::jmin(font.getHeight(), maxFontHeight));
        const auto editText = editor.getText();
        const float textWidth = measureTextWidth(font, editText);
        const float desiredWidth = textWidth + geo.shellPaddingX * 2.0f;
        const float shellWidth = juce::jlimit(geo.minShellWidth, pillBounds.getWidth(), desiredWidth);
        pillBounds = juce::Rectangle<float>(shellWidth, pillBounds.getHeight()).withCentre(pillBounds.getCentre());
    }

    if (!compact || editing)
    {
        rendering::JuceRenderer renderer(g);
        paintReadoutPill(renderer, pillBounds.getX(), pillBounds.getY(), pillBounds.getWidth(), pillBounds.getHeight(),
                         fill.getARGB(), outline.getARGB(), geo.cornerRadius, geo.outlineThickness);
    }

    auto font = adapters::makeFont(kernelTheme, kernel::TextRole::Readout, scale);
    const float maxFontHeight = juce::jmax(1.0f, pillBounds.getHeight() - geo.verticalSafety);
    font.setHeight(juce::jmin(font.getHeight(), maxFontHeight));
    if (!editing)
    {
        g.setColour(textColour);
        g.setFont(font);
        const int verticalPad = (int)(compact ? geo.compactVerticalPad : geo.standardVerticalPad);
        g.drawFittedText(text, pillBounds.toNearestInt().reduced((int)metrics.paddingX, verticalPad),
                         juce::Justification::centred, 1);

        if (compact)
        {
            const float underlineWidth = pillBounds.getWidth() * geo.underlineWidthFactor;
            const float lineY =
                pillBounds.getBottom() - juce::jmax(1.0f, metrics.paddingY * geo.underlineYOffsetFactor);
            juce::Line<float> underlineLine(pillBounds.getCentreX() - underlineWidth * 0.5f, lineY,
                                            pillBounds.getCentreX() + underlineWidth * 0.5f, lineY);
            g.setColour(underline);
            g.drawLine(underlineLine, juce::jmax(0.8f, metrics.outlineThickness * geo.underlineThicknessFactor));
        }
    }
    else
    {
        const auto editText = editor.getText();
        const auto selection = editor.getHighlightedRegion();
        const int caretPosition = editor.getCaretPosition();
        const int verticalPad = (int)(compact ? geo.compactVerticalPad : geo.standardVerticalPad);
        const auto textBounds = pillBounds.toNearestInt().reduced((int)metrics.paddingX, verticalPad);
        const float totalWidth = measureTextWidth(font, editText);
        const float originX = juce::jmax((float)textBounds.getX(), textBounds.getCentreX() - totalWidth * 0.5f);
        const int selectionStart = juce::jlimit(0, editText.length(), selection.getStart());
        const int selectionEnd = juce::jlimit(0, editText.length(), selection.getEnd());

        if (selectionEnd > selectionStart)
        {
            const auto beforeSelection = editText.substring(0, selectionStart);
            const auto selectedText = editText.substring(selectionStart, selectionEnd);
            const auto afterSelection = editText.substring(selectionEnd);
            const float beforeWidth = measureTextWidth(font, beforeSelection);
            const float selectedWidth = measureTextWidth(font, selectedText);
            const float selectionX = originX + beforeWidth;
            auto selectionBounds =
                juce::Rectangle<float>(selectionX - 1.0f, (float)textBounds.getY() + 1.0f,
                                       juce::jmax(2.0f, selectedWidth + 2.0f), (float)textBounds.getHeight() - 2.0f);
            g.setColour(theme.colors.accent0.withAlpha(bws::tokens::shared::opacity::ui_readout::SELECTION_HIGHLIGHT));
            g.fillRoundedRectangle(selectionBounds,
                                   juce::jmax(3.0f, selectionBounds.getHeight() * geo.selectionCornerFactor));

            drawTextRun(g, font, theme.colors.text0, beforeSelection, originX, textBounds);
            drawTextRun(g, font, theme.colors.text0, selectedText, selectionX, textBounds);
            drawTextRun(g, font, theme.colors.text0, afterSelection, selectionX + selectedWidth, textBounds);
        }
        else
        {
            drawTextRun(g, font, theme.colors.text0, editText, originX, textBounds);
        }

        if (editor.hasKeyboardFocus(true) && selectionStart == selectionEnd)
        {
            const auto caretText = editText.substring(0, juce::jlimit(0, editText.length(), caretPosition));
            const float caretOffset = measureTextWidth(font, caretText);
            const float caretHeight = juce::jmax(8.0f, font.getHeight() * geo.caretHeightFactor);
            const float caretX = originX + caretOffset + 1.0f;
            const float caretY = textBounds.getCentreY() - caretHeight * 0.5f;
            g.setColour(theme.colors.text0.withAlpha(bws::tokens::shared::opacity::ui_readout::CARET));
            g.drawLine(caretX, caretY, caretX, caretY + caretHeight, 1.1f);
        }
    }
}

void UiReadout::resized()
{
    const auto metrics = UiReadouts::metrics(role, scale, theme);
    const auto bounds = getLocalBounds();
    const int verticalPad =
        role == UiReadoutRole::Compact ? (int)std::floor(metrics.paddingY * 0.35f) : (int)std::floor(metrics.paddingY);
    editor.setBounds(bounds.reduced((int)std::round(metrics.paddingX), verticalPad));
}

void UiReadout::mouseEnter(const juce::MouseEvent& e)
{
    if (onHoverChanged)
        onHoverChanged(true);
    juce::Component::mouseEnter(e);
}

void UiReadout::mouseExit(const juce::MouseEvent& e)
{
    if (!editing && onHoverChanged)
        onHoverChanged(false);
    juce::Component::mouseExit(e);
}

void UiReadout::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (editable && e.eventComponent == this)
        beginEditingInternal();
    juce::Component::mouseDoubleClick(e);
}

bool UiReadout::requestBeginEditing()
{
    if (!editable || editing)
        return false;

    beginEditingInternal();
    return true;
}

void UiReadout::beginEditingInternal()
{
    if (!editable || editing)
        return;

    ++editSessionId;
    editing = true;
    resolvingEditSession = false;
    updateEditorStyle();
    editor.setText(onRequestEditSeedText ? onRequestEditSeedText() : text, juce::dontSendNotification);
    editor.setVisible(true);
    editor.toFront(false);
    editor.grabKeyboardFocus();
    editor.selectAll();
    if (onEditingChanged)
        onEditingChanged(true);
    repaint();
}

void UiReadout::endEditing(bool committedSuccessfully)
{
    juce::ignoreUnused(committedSuccessfully);
    editing = false;
    resolvingEditSession = false;
    editor.setVisible(false);
    if (onHoverChanged)
        onHoverChanged(isMouseOver(true));
    if (onEditingChanged)
        onEditingChanged(false);
    repaint();
}

void UiReadout::resolveDeferredFocusLoss(int sessionId)
{
    juce::Component::SafePointer<UiReadout> safeThis(this);
    juce::MessageManager::callAsync([safeThis, sessionId] {
        if (safeThis == nullptr)
            return;

        if (!safeThis->editing || safeThis->resolvingEditSession || safeThis->editSessionId != sessionId)
            return;

        if (safeThis->editor.hasKeyboardFocus(true))
            return;

        safeThis->commitEditing(EditCommitTrigger::FocusLost);
    });
}

void UiReadout::finishEditing(bool committedSuccessfully)
{
    endEditing(committedSuccessfully);
}

void UiReadout::updateEditorStyle()
{
    auto font = adapters::makeFont(kernelTheme, kernel::TextRole::Readout, scale);
    const auto metrics = UiReadouts::metrics(role, scale, theme);
    const float verticalSafety = role == UiReadoutRole::Compact ? juce::jmax(2.0f, metrics.paddingY * 1.25f)
                                                                : juce::jmax(2.0f, metrics.paddingY * 2.0f);
    const float maxFontHeight = juce::jmax(1.0f, metrics.height - verticalSafety);
    font.setHeight(juce::jmin(font.getHeight(), maxFontHeight));
    editor.setFont(font);
    editor.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    editor.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    editor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    editor.setColour(juce::TextEditor::shadowColourId, juce::Colours::transparentBlack);
    editor.setColour(juce::TextEditor::textColourId, juce::Colours::transparentBlack);
    editor.setColour(juce::CaretComponent::caretColourId, juce::Colours::transparentBlack);
    editor.setColour(juce::TextEditor::highlightColourId, juce::Colours::transparentBlack);
    editor.setColour(juce::TextEditor::highlightedTextColourId, juce::Colours::transparentBlack);
    editor.setBorder(juce::BorderSize<int>(0));
    editor.setIndents(0, 0);
    editor.setPopupMenuEnabled(false);
    editor.setMouseCursor(juce::MouseCursor::IBeamCursor);
    editor.setTextToShowWhenEmpty({},
                                  theme.colors.text1.withAlpha(bws::tokens::shared::opacity::ui_readout::PLACEHOLDER));
}


void UiReadout::testSetEditText(const juce::String& newText)
{
    editor.setText(newText, juce::dontSendNotification);
    repaint();
}

bool UiReadout::testIsEditorPopupEnabled() const
{
    return editor.isPopupMenuEnabled();
}

} // namespace bws::ui

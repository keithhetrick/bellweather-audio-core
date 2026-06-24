// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

/**
 * =============================================================================
 * WeatherKnob.cpp - Implementation
 * =============================================================================
 */

#include "bw_ui/weather/controls/WeatherKnob.h"
#include "bw_ui/foundation/UiTheme.h"
#include "bw_ui/generated/BwTokens.h"
#include <cmath>
#include <regex>

namespace bws
{
namespace weather
{

// =============================================================================
// WeatherKnob Implementation
// =============================================================================

WeatherKnob::WeatherKnob()
{
    // Set default behaviour
    setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
}

// =============================================================================
// Text Parsing
// =============================================================================

double WeatherKnob::getValueFromText(const juce::String& text)
{
    // Check if a custom valueFromTextFunction is set (from juce::Slider)
    if (valueFromTextFunction)
        return valueFromTextFunction(text);

    auto trimmed = text.trim();

    if (trimmed.isEmpty())
        return getValue();

    // Try percentage first (universal)
    if (percentageMode_ && trimmed.endsWithIgnoreCase("%"))
    {
        return parsePercentageText(trimmed);
    }

    // Try frequency mode
    if (frequencyMode_)
    {
        // Check for musical notes first
        if (isMusicalNote(trimmed))
        {
            return noteToFrequency(trimmed);
        }

        // Check for k suffix (1k, 10k, etc.)
        if (trimmed.endsWithIgnoreCase("k") || trimmed.endsWithIgnoreCase("khz"))
        {
            return parseFrequencyText(trimmed);
        }

        // Check for Hz suffix
        if (trimmed.endsWithIgnoreCase("hz"))
        {
            return parseFrequencyText(trimmed);
        }
    }

    // Try gain mode
    if (gainMode_)
    {
        // Check for multiplier suffix (2x, 0.5x, etc.)
        if (trimmed.endsWithIgnoreCase("x"))
        {
            return parseGainText(trimmed);
        }

        // Check for dB suffix
        if (trimmed.endsWithIgnoreCase("db"))
        {
            auto numText = trimmed.dropLastCharacters(2).trim();
            return numText.getDoubleValue();
        }
    }

    // Fall back to base class parsing
    return UiArcKnob::getValueFromText(text);
}

juce::String WeatherKnob::getTextFromValue(double value)
{
    // Check if a custom textFromValueFunction is set (from juce::Slider)
    if (textFromValueFunction)
        return textFromValueFunction(value) + getTextValueSuffix();

    // Use base class formatting
    return UiArcKnob::getTextFromValue(value);
}

// =============================================================================
// Frequency Parsing
// =============================================================================

double WeatherKnob::parseFrequencyText(const juce::String& text) const
{
    auto lower = text.toLowerCase();

    // Handle "k" or "khz" suffix
    if (lower.endsWith("khz"))
    {
        auto numText = text.dropLastCharacters(3).trim();
        return numText.getDoubleValue() * 1000.0;
    }
    else if (lower.endsWith("k"))
    {
        auto numText = text.dropLastCharacters(1).trim();
        return numText.getDoubleValue() * 1000.0;
    }
    else if (lower.endsWith("hz"))
    {
        auto numText = text.dropLastCharacters(2).trim();
        return numText.getDoubleValue();
    }

    // Just a number
    return text.getDoubleValue();
}

// =============================================================================
// Gain Parsing
// =============================================================================

double WeatherKnob::parseGainText(const juce::String& text) const
{
    // Handle multiplier suffix (2x -> +6.02 dB, 0.5x -> -6.02 dB)
    if (text.endsWithIgnoreCase("x"))
    {
        auto numText = text.dropLastCharacters(1).trim();
        double multiplier = numText.getDoubleValue();

        if (multiplier > 0.0)
        {
            // Convert multiplier to dB: 20 * log10(multiplier)
            return 20.0 * std::log10(multiplier);
        }
    }

    // Just parse as dB value
    return text.getDoubleValue();
}

// =============================================================================
// Percentage Parsing
// =============================================================================

double WeatherKnob::parsePercentageText(const juce::String& text) const
{
    // Remove % suffix and convert to value in range
    auto numText = text.dropLastCharacters(1).trim();
    double percentage = numText.getDoubleValue() / 100.0;

    // Map percentage to slider range
    double minVal = getMinimum();
    double maxVal = getMaximum();

    return minVal + percentage * (maxVal - minVal);
}

// =============================================================================
// Musical Note Parsing
// =============================================================================

bool WeatherKnob::isMusicalNote(const juce::String& text)
{
    if (text.isEmpty())
        return false;

    // First character should be a note letter (A-G)
    auto firstChar = text[0];
    if (firstChar < 'A' || firstChar > 'G')
    {
        // Also check lowercase
        if (firstChar < 'a' || firstChar > 'g')
            return false;
    }

    // Should have at least 2 characters (note + octave)
    if (text.length() < 2)
        return false;

    // Check for octave number somewhere in the string
    for (int i = 1; i < text.length(); ++i)
    {
        auto c = text[i];
        if (c >= '0' && c <= '9')
            return true;
    }

    return false;
}

double WeatherKnob::noteToFrequency(const juce::String& noteText) const
{
    int semitone = parseNoteToSemitone(noteText);
    double cents = parseCentsOffset(noteText);

    // Calculate frequency: A4 = 440 Hz, each semitone is 2^(1/12)
    // semitone is relative to A4 (A4 = 0)
    double semitonesFromA4 = semitone + cents / 100.0;
    return a4Frequency_ * std::pow(2.0, semitonesFromA4 / 12.0);
}

int WeatherKnob::parseNoteToSemitone(const juce::String& noteText)
{
    if (noteText.isEmpty())
        return 0;

    // Note letter to semitone offset from C (C=0, D=2, E=4, F=5, G=7, A=9, B=11)
    static const int noteOffsets[7] = {9, 11, 0, 2, 4, 5, 7}; // A, B, C, D, E, F, G

    auto upper = noteText.toUpperCase();
    char noteLetter = upper[0];

    if (noteLetter < 'A' || noteLetter > 'G')
        return 0;

    int noteOffset = noteOffsets[noteLetter - 'A'];

    // Parse sharps/flats
    int sharpFlatOffset = 0;
    int index = 1;

    while (index < upper.length())
    {
        char c = upper[index];
        if (c == '#')
            sharpFlatOffset++;
        else if (c == 'B' && index > 0) // 'b' as flat (but not at start)
            sharpFlatOffset--;
        else
            break;
        index++;
    }

    // Parse octave number
    int octave = 4; // Default to octave 4
    juce::String octaveStr;

    while (index < upper.length())
    {
        char c = upper[index];
        if (c >= '0' && c <= '9')
            octaveStr += c;
        else if (c == '+' || c == '-')
            break; // Cents offset starts
        index++;
    }

    if (octaveStr.isNotEmpty())
        octave = octaveStr.getIntValue();

    // Calculate semitone relative to A4
    // A4 = octave 4, semitone 9 from C
    // Semitone from A4 = (octave - 4) * 12 + (noteOffset - 9) + sharpFlat
    int semitone = (octave - 4) * 12 + (noteOffset - 9) + sharpFlatOffset;

    return semitone;
}

double WeatherKnob::parseCentsOffset(const juce::String& noteText)
{
    // Look for +/- followed by numbers at the end
    auto plusIndex = noteText.lastIndexOf("+");
    auto minusIndex = noteText.lastIndexOf("-");

    int offsetIndex = std::max(plusIndex, minusIndex);

    if (offsetIndex > 0)
    {
        // Make sure it's after the octave number
        auto afterOffset = noteText.substring(offsetIndex);
        if (afterOffset.length() > 1)
        {
            return afterOffset.getDoubleValue();
        }
    }

    return 0.0;
}

// =============================================================================
// Mouse Handling
// =============================================================================

void WeatherKnob::mouseDown(const juce::MouseEvent& e)
{
    // Right-click: Show WeatherKnob-specific context menu
    if (e.mods.isPopupMenu())
    {
        showContextMenu();
        return;
    }

    // Cmd/Ctrl+click: Reset to default value (FabFilter-style quick reset)
    if (e.mods.isCommandDown() && !e.mods.isAltDown())
    {
        // Reset to default value
        setValue(getDoubleClickReturnValue(), juce::sendNotificationAsync);

        // Trigger brief golden glow feedback (100ms)
        isShowingResetFeedback_ = true;
        repaint();
        startTimer(100); // Timer callback will clear the feedback

        return; // Don't start a drag
    }

    // Initialize velocity tracking
    lastDragPos_ = e.getPosition();
    lastDragTime_ = juce::Time::getCurrentTime();

    // Let base class handle everything (now uses correct modifiers globally)
    UiArcKnob::mouseDown(e);
}

void WeatherKnob::mouseDrag(const juce::MouseEvent& e)
{
    if (isLocked())
    {
        UiArcKnob::mouseDrag(e);
        return;
    }
    if (velocitySensitive_)
    {
        lastDragPos_ = e.getPosition();
        lastDragTime_ = juce::Time::getCurrentTime();
    }

    // Update tooltip while dragging if mouse is over
    if (isMouseOver())
    {
        updateTooltipText();
    }

    // Let base class handle the drag (now uses correct modifiers globally)
    UiArcKnob::mouseDrag(e);
}

void WeatherKnob::mouseEnter(const juce::MouseEvent& e)
{
    UiArcKnob::mouseEnter(e);

    // Generate tooltip with current value on hover
    if (!isLocked())
        updateTooltipText();
}

void WeatherKnob::mouseExit(const juce::MouseEvent& e)
{
    UiArcKnob::mouseExit(e);

    // Clear tooltip on exit (optional)
    // setTooltip({});
}

void WeatherKnob::paint(juce::Graphics& g)
{
    // Call base class paint first
    UiArcKnob::paint(g);

    // Reset feedback: golden glow overlay when Cmd+click resets to default
    if (isShowingResetFeedback_)
    {
        static const auto kDefault = bws::ui::resolveTheme(bws::ui::defaultUiThemeBase(), {});
        const auto& t = theme_ ? *theme_ : kDefault;

        auto bounds = getLocalBounds().toFloat();
        auto cx = bounds.getCentreX();
        auto cy = bounds.getCentreY();
        auto diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());

        // Golden glow overlay (30% alpha, expanded slightly)
        auto goldenAccent = t.weatherColors.accent;
        g.setColour(goldenAccent.withAlpha(bws::tokens::shared::opacity::knob::ACCENT_WASH));
        g.fillEllipse(cx - diameter / 2.0f - 4.0f, cy - diameter / 2.0f - 4.0f, diameter + 8.0f, diameter + 8.0f);
    }
}

// =============================================================================
// Tooltip Support
// =============================================================================

void WeatherKnob::updateTooltipText()
{
    if (isLocked())
        return;
    // Get formatted value string using the textFromValue function
    // This respects custom formatters (textFromValueFunction) if set
    auto valueText = getTextFromValue(getValue());

    // Combine parameter name with value for informative tooltip
    auto paramName = getName();
    juce::String tooltipText;

    if (paramName.isNotEmpty())
    {
        tooltipText = paramName + ": " + valueText;
    }
    else
    {
        tooltipText = valueText;
    }

    setTooltip(tooltipText);
}

// =============================================================================
// Context Menu
// =============================================================================

void WeatherKnob::showContextMenu()
{
    juce::PopupMenu menu;

    // Get current value for display
    auto currentValueText = getTextFromValue(getValue());
    auto paramName = getName();

    // Header showing current value (disabled item for display only)
    if (paramName.isNotEmpty())
    {
        menu.addItem(-1, paramName + ": " + currentValueText, false, false);
    }
    else
    {
        menu.addItem(-1, "Value: " + currentValueText, false, false);
    }
    menu.addSeparator();

    // Reset to Default
    menu.addItem(1, "Reset to Default");

    // Enter Value (opens text entry)
    menu.addItem(2, "Enter Value...");

    // Show keyboard shortcuts
    menu.addSeparator();
    menu.addItem(-1, "Double-click: Reset", false, false);
    menu.addItem(-1, "Shift+Drag: Fine Control (5x)", false, false);
    menu.addItem(-1, "Cmd+Drag: Alt Mode", false, false);

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this).withMinimumWidth(180), [this](int result) {
        switch (result)
        {
        case 1: // Reset to Default
            setValue(getDoubleClickReturnValue(), juce::sendNotificationAsync);
            DBG("WeatherKnob: Context menu reset to default: " << getDoubleClickReturnValue());
            break;

        case 2: // Enter Value
            // Use the base class text entry functionality
            showTextEntry();
            break;

        default:
            break;
        }
    });
}

} // namespace weather
} // namespace bws

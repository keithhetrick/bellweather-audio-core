// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/Components/InteractiveReadout.h"

#include "bw_ui/adapters/FineDragModifier.h"     // kernel::fromJuce
#include "bw_ui/adapters/UiThemeKernelAdapter.h" // makeKernelThemeSnapshot, makeFont
#include "bw_ui/kernel/ReadoutValue.h"           // formatValue

namespace bws::ui
{

InteractiveReadout::InteractiveReadout(juce::RangedAudioParameter& param, const UiThemeResolved& theme,
                                       kernel::FormatSpec spec, interaction::ValueInteractionConfig config)
    : readout_(UiReadoutRole::Compact, theme)
    , adapter_(&param)
    , spec_(std::move(spec))
    , config_(config)
    , kernelTheme_(adapters::makeKernelThemeSnapshot(theme))
{
    model_ = std::make_unique<interaction::ValueInteraction>(adapter_, policy_, spec_, config_);

    readout_.setCompactAppearance(UiReadout::CompactAppearance::CompanionCompact);
    readout_.setEditable(true);                      // native inline editor, same as the precision-row readouts
    readout_.setInterceptsMouseClicks(false, false); // this shell owns drag/scroll; the editor grabs keyboard focus
    readout_.onRequestEditSeedText = [this] {
        return juce::String(model_->displayText());
    };
    readout_.onCommitEditText = [this](const juce::String& candidate, UiReadout::EditCommitTrigger) {
        model_->commitText(candidate.toStdString());
        refresh();
        return true;
    };
    readout_.onEditingChanged = [this](bool nowEditing) {
        editing_ = nowEditing;
    };
    addAndMakeVisible(readout_);
    refreshReservation();
    refresh();
}

void InteractiveReadout::setTheme(const UiThemeResolved& theme)
{
    kernelTheme_ = adapters::makeKernelThemeSnapshot(theme);
    readout_.setTheme(theme);
    refreshReservation();
}

void InteractiveReadout::setScale(float scale)
{
    readout_.setScale(scale);
}

void InteractiveReadout::rebind(juce::RangedAudioParameter& param)
{
    adapter_ = adapters::JuceParameterAdapter {&param};
    model_->rebind(adapter_);
    refreshReservation();
    refresh();
}

void InteractiveReadout::refresh()
{
    readout_.setText(model_->displayText());
}

void InteractiveReadout::refreshReservation()
{
    const juce::Font probe = adapters::makeFont(kernelTheme_, kernel::TextRole::Readout, 1.0f);
    constexpr int kSamples = 48;
    juce::String widest;
    float widestWidth = -1.0f;
    for (int i = 0; i <= kSamples; ++i)
    {
        const double real =
            adapter_.convertToReal(static_cast<float>(i) / static_cast<float>(kSamples)) * config_.displayScale;
        const juce::String candidate {kernel::formatValue(real, spec_)};
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(probe, candidate, 0.0f, 0.0f);
        const float w = glyphs.getBoundingBox(0, -1, true).getWidth();
        if (w > widestWidth)
        {
            widestWidth = w;
            widest = candidate;
        }
    }
    readout_.setWidthReservationText(widest);
}

juce::String InteractiveReadout::currentText() const
{
    return model_->displayText();
}

void InteractiveReadout::commitText(const juce::String& text)
{
    model_->commitText(text.toStdString());
    refresh();
}

void InteractiveReadout::resized()
{
    readout_.setBounds(getLocalBounds());
}

void InteractiveReadout::mouseDown(const juce::MouseEvent& e)
{
    if (editing_)
    {
        return;
    }
    dragAnchorY_ = e.getPosition().getY();
    model_->pointerDown(kernel::fromJuce(e.mods), false);
}

void InteractiveReadout::mouseDrag(const juce::MouseEvent& e)
{
    if (editing_)
    {
        return;
    }
    model_->pointerDrag(dragAnchorY_ - e.getPosition().getY(), kernel::fromJuce(e.mods)); // up = increase
    refresh();
}

void InteractiveReadout::mouseUp(const juce::MouseEvent& e)
{
    if (editing_)
    {
        return;
    }
    model_->pointerUp();
    refresh();
    juce::ignoreUnused(e);
}

void InteractiveReadout::mouseDoubleClick(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    readout_.requestBeginEditing(); // types in place via UiReadout's native editor
}

void InteractiveReadout::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    juce::ignoreUnused(e);
    if (editing_)
    {
        return;
    }
    model_->wheel(wheel.deltaY, kernel::fromJuce(e.mods));
    refresh();
}

void InteractiveReadout::dragByForTesting(int pixelsUp)
{
    if (editing_)
    {
        return;
    }
    model_->pointerDown(kernel::ModSet {}, false);
    model_->pointerDrag(pixelsUp, kernel::ModSet {});
    model_->pointerUp();
    refresh();
}

void InteractiveReadout::commitEditTextForTesting(const juce::String& text)
{
    readout_.testSetEditText(text);
    readout_.commitEditing(UiReadout::EditCommitTrigger::EnterKey);
}

} // namespace bws::ui

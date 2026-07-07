// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "bw_ui/Components/UiReadout.h"
#include "bw_ui/adapters/JuceParameterAdapter.h"
#include "bw_ui/foundation/UiTheme.h"
#include "bw_ui/interaction/ValueInteraction.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

namespace bws::ui
{

// A single value readout with full drag + scroll + inline-type interaction. The
// action logic is the native ValueInteraction model; this shell is the JUCE
// boundary that renders + edits via UiReadout (the same readout the precision
// row uses) and forwards drag/scroll to the model. Double-click types in place
// through UiReadout's native editor - no dialog, so it reads and feels identical
// to the rest of the plugin's readouts.
class InteractiveReadout : public juce::Component
{
public:
    InteractiveReadout(juce::RangedAudioParameter& param, const UiThemeResolved& theme, kernel::FormatSpec spec,
                       interaction::ValueInteractionConfig config = {});

    void setTheme(const UiThemeResolved& theme);
    void setScale(float scale);
    void setAccentText(bool useAccent) { readout_.setAccentText(useAccent); }
    void rebind(juce::RangedAudioParameter& param); // repoint to a new parameter (e.g. slot reselect)
    void refresh();                                 // pull the model's display text into the readout

    [[nodiscard]] juce::String currentText() const;
    void commitText(const juce::String& text); // text -> model -> refresh

    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    // ForTesting - drive interactions through the model/editor without a live mouse.
    void dragByForTesting(int pixelsUp);
    [[nodiscard]] bool beginEditForTesting() { return readout_.requestBeginEditing(); }
    [[nodiscard]] bool isEditingForTesting() const { return editing_; }
    void commitEditTextForTesting(const juce::String& text);
    [[nodiscard]] juce::String textForTesting() const { return readout_.getText(); }
    [[nodiscard]] juce::String widthReservationForTesting() const { return readout_.getWidthReservationText(); }

private:
    void refreshReservation();

    UiReadout readout_;
    adapters::JuceParameterAdapter adapter_;
    interaction::InteractionPolicy policy_;
    kernel::FormatSpec spec_;
    interaction::ValueInteractionConfig config_;
    kernel::ThemeSnapshot kernelTheme_;
    std::unique_ptr<interaction::ValueInteraction> model_;
    int dragAnchorY_ {0};
    bool editing_ {false};
};

} // namespace bws::ui

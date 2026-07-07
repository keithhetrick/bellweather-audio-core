// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "bw_ui/interaction/InteractionPolicy.h"
#include "bw_ui/kernel/IValueParameter.h"
#include "bw_ui/kernel/InteractionContract.h"
#include "bw_ui/kernel/ParameterGestureScope.h"
#include "bw_ui/kernel/ReadoutValue.h"

#include <memory>
#include <string>
#include <string_view>

namespace bws::ui::interaction
{

// Magnetic snap during drag at a normalised position (e.g. unity at 0.5).
struct CenterDetent
{
    bool enabled {false};
    float position {0.5f};
    float snapRadius {0.0f};
};

struct ValueInteractionConfig
{
    kernel::FineDragSensitivity drag {}; // pixels per full range (base / fine)
    float wheelNormal {0.01f};
    float wheelFine {0.002f};
    double displayScale {1.0};
    CenterDetent detent {};
};

// JUCE-free model owning the gesture -> edit orchestration for one parameter
// value: drag (axis-agnostic, signed pixels from the start), wheel, double-click
// dispatch (reset / text-entry / custom via InteractionPolicy), and typed commit.
// The visual component forwards pointer facts + pulls displayText(); JUCE stays in
// the component shell + adapters.
class ValueInteraction
{
public:
    ValueInteraction(kernel::IValueParameter& param, InteractionPolicy policy, kernel::FormatSpec spec,
                     ValueInteractionConfig config = {});

    void pointerDown(kernel::ModSet mods, bool isDoubleClick);  // begin drag, or dispatch double-click
    void pointerDrag(int pixelsFromStart, kernel::ModSet mods); // up/right = increase; component projects its axis
    void pointerUp();
    void wheel(float deltaY, kernel::ModSet mods);
    void rebind(kernel::IValueParameter& param); // repoint + drop any open gesture
    void setDetent(CenterDetent detent) { config_.detent = detent; }

    [[nodiscard]] std::string displayText() const; // formatValue(real * displayScale)
    void commitText(std::string_view text);        // parse -> convert -> clamp -> one-shot gesture

private:
    void applyDoubleClick();
    void oneShotWrite(float normalized);
    [[nodiscard]] float snap(float normalized) const;

    kernel::IValueParameter* param_;
    InteractionPolicy policy_;
    kernel::FormatSpec spec_;
    ValueInteractionConfig config_;

    std::unique_ptr<kernel::ParameterGestureScope> drag_; // opened lazily on first move
    float dragStartNorm_ {0.0f};
    bool armed_ {false}; // pointerDown seen; gesture begins on the first pointerDrag
};

} // namespace bws::ui::interaction

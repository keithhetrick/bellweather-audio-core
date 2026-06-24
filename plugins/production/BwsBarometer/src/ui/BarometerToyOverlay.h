// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../toy/SignalLockModel.h"
#include "../toy/SignalLockTypes.h"
#include "SignalLockView.h"

#include <bw_toy_shell/HintCard.h>
#include <bw_toy_shell/TimerPump.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace bws::barometer::toy
{

/// Modal-style game host. Owns the SignalLockState by value; the view holds
/// a non-owning ref into it; both share lifetime with the overlay. Invokes
/// `onClose` on Escape - the editor's close lambda is responsible for the
/// async reset of the unique_ptr that owns this component.
class BarometerToyOverlay : public juce::Component
{
public:
    explicit BarometerToyOverlay(std::function<void()> onClose, float scale = 1.0f);
    ~BarometerToyOverlay() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;
    bool keyStateChanged(bool isKeyDown) override;
    void focusLost(FocusChangeType cause) override;
    void parentHierarchyChanged() override;
    void visibilityChanged() override;

    [[nodiscard]] bool wasKonamiSolvedForTesting() const noexcept { return state_.solvedFlash > 0.0; }

private:
    void tick(double dt);
    [[nodiscard]] SignalLockInput sampleInput() const noexcept;
    void refreshHeldKeysFromOs() noexcept;
    bool feedKonami(int keyCode) noexcept;

    SignalLockState state_;
    SignalLockView view_;
    bws::toy_shell::TimerPump timer_;
    bws::toy_shell::HintCard hint_;
    std::function<void()> onClose_;
    double errorSampleAccum_ = 0.0;
    float scale_;

    struct HeldKeys
    {
        bool left_ = false, right_ = false, up_ = false, down_ = false;
    };
    HeldKeys held_;

    std::size_t konamiMatched_ = 0;
};

} // namespace bws::barometer::toy

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace bws::toy_shell
{

/// First-launch hint card. Shows a few lines of uppercase text plus a
/// "click to dismiss" CTA; auto-dismisses after timeoutSeconds (default 8s)
/// or on mouseDown anywhere on the component.
class HintCard
    : public juce::Component
    , private juce::Timer
{
public:
    explicit HintCard(juce::StringArray lines, int timeoutSeconds = 6, float scale = 1.0f);
    ~HintCard() override;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;

    [[nodiscard]] float getScale() const noexcept { return scale_; }

private:
    void timerCallback() override;
    void dismiss();

    juce::StringArray lines_;
    int remainingSeconds_;
    float scale_;
};

} // namespace bws::toy_shell

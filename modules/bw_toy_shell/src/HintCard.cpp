// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_toy_shell/HintCard.h"

namespace bws::toy_shell
{

namespace
{
constexpr float kCardPadding = 18.0f;
constexpr float kLineHeight = 22.0f;
constexpr float kCtaGap = 10.0f;
constexpr float kCornerRadius = 6.0f;
constexpr float kBodyFontSize = 13.0f;
constexpr float kCtaFontSize = 11.0f;
} // namespace

HintCard::HintCard(juce::StringArray lines, int timeoutSeconds, float scale)
    : lines_(std::move(lines))
    , remainingSeconds_(juce::jmax(1, timeoutSeconds))
    , scale_(juce::jmax(0.25f, scale))
{
    setInterceptsMouseClicks(true, false);
    startTimer(1000);
}

HintCard::~HintCard()
{
    stopTimer();
}

void HintCard::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float pad = kCardPadding * scale_;
    const float lineH = kLineHeight * scale_;
    const float corner = kCornerRadius * scale_;

    g.setColour(juce::Colour::fromFloatRGBA(0.0f, 0.0f, 0.0f, 0.72f));
    g.fillRoundedRectangle(bounds, corner);
    g.setColour(juce::Colour::fromFloatRGBA(1.0f, 1.0f, 1.0f, 0.18f));
    g.drawRoundedRectangle(bounds, corner, 1.0f * scale_);

    g.setFont(juce::FontOptions(kBodyFontSize * scale_).withName(juce::Font::getDefaultMonospacedFontName()));
    g.setColour(juce::Colour::fromFloatRGBA(0.92f, 0.92f, 0.92f, 1.0f));

    const int textW = getWidth() - static_cast<int>(2.0f * pad);
    float y = pad;
    for (const auto& line : lines_)
    {
        g.drawText(line, static_cast<int>(pad), static_cast<int>(y), textW, static_cast<int>(lineH),
                   juce::Justification::centred, false);
        y += lineH;
    }

    y += kCtaGap * scale_;
    g.setColour(juce::Colour::fromFloatRGBA(0.65f, 0.65f, 0.65f, 1.0f));
    g.setFont(juce::FontOptions(kCtaFontSize * scale_)
                  .withName(juce::Font::getDefaultMonospacedFontName())
                  .withStyle("Italic"));
    const auto cta = "click to dismiss  (" + juce::String(remainingSeconds_) + "s)";
    g.drawText(cta, static_cast<int>(pad), static_cast<int>(y), textW, static_cast<int>(lineH),
               juce::Justification::centred, false);
}

void HintCard::mouseDown(const juce::MouseEvent&)
{
    dismiss();
}

void HintCard::timerCallback()
{
    if (--remainingSeconds_ <= 0)
    {
        dismiss();
    }
    else
    {
        repaint();
    }
}

void HintCard::dismiss()
{
    stopTimer();
    setVisible(false);
}

} // namespace bws::toy_shell

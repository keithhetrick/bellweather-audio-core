// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
//
#include "SignalLockView.h"

#include "../toy/SignalLockModel.h"
#include "../toy/SignalLockTuning.h"

#include <bw_toy_shell/HudUtils.h>
#include <bw_ui/foundation/Fonts.h>
#include <bw_ui/generated/BwTokens.h>

#include <algorithm>
#include <cmath>

namespace bws::barometer::toy
{

namespace
{

// Barometer design tokens for the Signal Lock palette.
// Source: bds/tokens/products/barometer.tokens.json
constexpr juce::uint32 kBg = 0xff1a1a1a;               // bg.base
constexpr juce::uint32 kGridDim = 0xff252525;          // bg.readout
constexpr juce::uint32 kAxisLine = 0xff444444;         // text.outlineBorder
constexpr juce::uint32 kReferenceWave = 0xff888888;    // text.branding
constexpr juce::uint32 kActiveLocked = 0xff00aa00;     // meter.green
constexpr juce::uint32 kActiveDrifting = 0xffd4a84b;   // accent.brassActive
constexpr juce::uint32 kActiveUnstable = 0xffff3333;   // meter.red
constexpr juce::uint32 kActiveLost = 0xffff0000;       // meter.clip
constexpr juce::uint32 kToleranceBand = 0xffd4af37;    // accent.brass
constexpr juce::uint32 kTrimIndicator = 0xffe5c47a;    // slider.thumbBright
constexpr juce::uint32 kTrimIndicatorDim = 0xffa87f3f; // slider.thumbDark
constexpr juce::uint32 kCursor = 0xffd4af37;           // accent.brass
constexpr juce::uint32 kHudText = 0xfffffef0;          // text.readout
constexpr juce::uint32 kHudLabel = 0xffaaaaaa;         // text.meterLabel
constexpr juce::uint32 kScanline = 0xfffffef0;         // text.readout

constexpr float kBaseFreqCycles = 6.0f;
constexpr float kWaveAmplitudePx = 70.0f;
constexpr float kFreqDisplayScale = 1.5f;
constexpr float kTrailDecayAlpha = 0.22f;
constexpr float kScanlineAlpha = 0.04f;
constexpr float kNoiseVisualAmplPx = 1.6f;
constexpr float kNoiseRamp = 0.008f;
constexpr float kScopeW = 160.0f;
constexpr float kScopeH = 48.0f;
constexpr float kScopeMargin = 18.0f;
const float kScopeMaxError = static_cast<float>(ERROR_DANGER_THRESHOLD) * 1.2f;

[[nodiscard]] juce::Colour col(juce::uint32 argb, float alpha) noexcept
{
    return juce::Colour(argb).withAlpha(std::clamp(alpha, 0.0f, 1.0f));
}

[[nodiscard]] juce::uint32 activeColor(SignalCondition c) noexcept
{
    switch (c)
    {
    case SignalCondition::Locked:
        return kActiveLocked;
    case SignalCondition::Drifting:
        return kActiveDrifting;
    case SignalCondition::Unstable:
        return kActiveUnstable;
    case SignalCondition::Lost:
        return kActiveLost;
    }
    return kActiveDrifting;
}

[[nodiscard]] float deterministicNoise(float x, float t) noexcept
{
    return (0.5f * std::sin(x * 47.3f + t * 13.7f)) + (0.5f * std::sin(x * 31.1f - t * 19.2f + 1.4f));
}

} // namespace

SignalLockView::SignalLockView()
{
    setOpaque(true);
    setInterceptsMouseClicks(false, false);
}

void SignalLockView::pushErrorSample(double error) noexcept
{
    errorHistory_[errorCursor_] = error;
    errorCursor_ = (errorCursor_ + 1) % kErrorHistoryCapacity;
    if (errorCursor_ == 0)
        errorFilled_ = true;
}

void SignalLockView::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.fillAll(juce::Colour(kBg));

    if (state_ == nullptr)
    {
        return;
    }
    const SignalLockState& s = *state_;

    const float width = bounds.getWidth();
    const float height = bounds.getHeight();
    const float midY = height * 0.5f;
    const float t = static_cast<float>(s.timeSeconds);

    const float tolerance =
        std::max(static_cast<float>(TOLERANCE_MIN),
                 static_cast<float>(ERROR_LOCK_THRESHOLD) * (1.0f - t * static_cast<float>(TOLERANCE_RAMP)));

    // Trail fade - overlay bg at low alpha
    g.setColour(col(kBg, kTrailDecayAlpha));
    g.fillRect(bounds);

    // Oscilloscope grid (8 horizontal × 12 vertical divisions)
    g.setColour(juce::Colour(kGridDim));
    for (int i = 1; i < 8; ++i)
    {
        const float y = (height / 8.0f) * static_cast<float>(i);
        g.drawLine(0.0f, y, width, y, 1.0f);
    }
    for (int i = 1; i < 12; ++i)
    {
        const float x = (width / 12.0f) * static_cast<float>(i);
        g.drawLine(x, 0.0f, x, height, 1.0f);
    }

    // Center axis
    g.setColour(juce::Colour(kAxisLine));
    g.drawLine(0.0f, midY, width, midY, 1.0f);

    // Tolerance band - alpha rises with lockAmount
    {
        const float bandPx = std::max(8.0f, tolerance * kWaveAmplitudePx * 2.0f);
        const float bandAlpha = 0.08f + 0.12f * static_cast<float>(s.lockAmount);
        g.setColour(col(kToleranceBand, bandAlpha));
        g.fillRect(0.0f, midY - bandPx * 0.5f, width, bandPx);
    }

    // Reference waveform (dim, fixed)
    {
        g.setColour(juce::Colour(kReferenceWave));
        juce::Path ref;
        bool first = true;
        for (float px = 0.0f; px <= width; px += 2.0f)
        {
            const float u = px / width;
            const float y =
                midY - kWaveAmplitudePx * std::sin(2.0f * juce::MathConstants<float>::pi * kBaseFreqCycles * u);
            if (first)
            {
                ref.startNewSubPath(px, y);
                first = false;
            }
            else
                ref.lineTo(px, y);
        }
        g.strokePath(ref, juce::PathStrokeType(1.5f));
    }

    // Active waveform (drift + trim applied, color tracks condition)
    {
        const float noiseScale = s.godMode ? 0.0f : kNoiseVisualAmplPx * (1.0f + (kNoiseRamp * t));
        g.setColour(juce::Colour(activeColor(s.condition)));
        juce::Path active;
        bool first = true;
        for (float px = 0.0f; px <= width; px += 2.0f)
        {
            const float u = px / width;
            const float arg = 2.0f * juce::MathConstants<float>::pi * kBaseFreqCycles * u +
                              static_cast<float>(s.phase) +
                              static_cast<float>(s.frequency) * kFreqDisplayScale * u * juce::MathConstants<float>::pi;
            const float y = midY - kWaveAmplitudePx * std::sin(arg) + deterministicNoise(u * 8.0f, t) * noiseScale;
            if (first)
            {
                active.startNewSubPath(px, y);
                first = false;
            }
            else
                active.lineTo(px, y);
        }
        g.strokePath(active, juce::PathStrokeType(2.0f));
    }

    // Phase cursor - thin vertical at canvas midpoint
    {
        const float alpha = 0.4f + 0.4f * static_cast<float>(s.lockAmount);
        g.setColour(col(kCursor, alpha));
        g.drawLine(width * 0.5f, midY - kWaveAmplitudePx - 8.0f, width * 0.5f, midY + kWaveAmplitudePx + 8.0f, 1.0f);
    }

    // Trim indicators (left edge - top half = phase, bottom half = freq)
    {
        const float trimX = 14.0f;
        const float trimBarLen = 60.0f;
        const float phaseTrimNorm = static_cast<float>(s.phaseTrim / juce::MathConstants<double>::pi);

        g.setColour(juce::Colour(kTrimIndicatorDim));
        g.drawLine(trimX, midY - 70.0f - trimBarLen * 0.5f, trimX, midY - 70.0f + trimBarLen * 0.5f, 2.0f);
        g.setColour(juce::Colour(kTrimIndicator));
        const float phaseY = midY - 70.0f + (phaseTrimNorm * trimBarLen * 0.5f);
        g.fillEllipse(trimX - 3.0f, phaseY - 3.0f, 6.0f, 6.0f);

        g.setColour(juce::Colour(kTrimIndicatorDim));
        g.drawLine(trimX, midY + 70.0f - trimBarLen * 0.5f, trimX, midY + 70.0f + trimBarLen * 0.5f, 2.0f);
        g.setColour(juce::Colour(kTrimIndicator));
        const float freqY = midY + 70.0f + (static_cast<float>(s.frequencyTrim) * trimBarLen * 0.5f);
        g.fillEllipse(trimX - 3.0f, freqY - 3.0f, 6.0f, 6.0f);
    }

    // Subtle CRT scanlines - fine horizontal lines every 3px
    g.setColour(col(kScanline, kScanlineAlpha));
    for (float y = 0.0f; y < height; y += 3.0f)
    {
        g.fillRect(0.0f, y, width, 1.0f);
    }

    // Rolling error-history scope (bottom-right corner)
    {
        const float scopeX = width - kScopeW - kScopeMargin;
        const float scopeY = height - kScopeH - kScopeMargin;

        g.setColour(col(kBg, 0.7f));
        g.fillRect(scopeX, scopeY, kScopeW, kScopeH);
        g.setColour(juce::Colour(kAxisLine));
        g.drawRect(scopeX, scopeY, kScopeW, kScopeH, 1.0f);

        const float dangerY =
            scopeY + kScopeH - (static_cast<float>(ERROR_DANGER_THRESHOLD) / kScopeMaxError) * kScopeH;
        g.setColour(col(kActiveUnstable, 0.35f));
        g.drawLine(scopeX, dangerY, scopeX + kScopeW, dangerY, 1.0f);

        const float lockY = scopeY + kScopeH - (tolerance / kScopeMaxError) * kScopeH;
        g.setColour(col(kActiveLocked, 0.35f));
        g.drawLine(scopeX, lockY, scopeX + kScopeW, lockY, 1.0f);

        // Error trace - walk the ring buffer chronologically.
        const std::size_t n = errorFilled_ ? kErrorHistoryCapacity : errorCursor_;
        if (n > 1)
        {
            g.setColour(juce::Colour(activeColor(s.condition)));
            juce::Path trace;
            const std::size_t start = errorFilled_ ? errorCursor_ : 0;
            bool first = true;
            for (std::size_t i = 0; i < n; ++i)
            {
                const std::size_t idx = (start + i) % kErrorHistoryCapacity;
                const float e = static_cast<float>(errorHistory_[idx]);
                const float frac = static_cast<float>(i) / static_cast<float>(n - 1);
                const float px = scopeX + frac * kScopeW;
                const float py = scopeY + kScopeH - std::min(1.0f, e / kScopeMaxError) * kScopeH;
                if (first)
                {
                    trace.startNewSubPath(px, py);
                    first = false;
                }
                else
                    trace.lineTo(px, py);
            }
            g.strokePath(trace, juce::PathStrokeType(1.5f));
        }

        g.setColour(juce::Colour(kHudLabel));
        g.setFont(juce::FontOptions(9.0f));
        g.drawText("ERROR / 5s", static_cast<int>(scopeX), static_cast<int>(scopeY - 14.0f), static_cast<int>(kScopeW),
                   12, juce::Justification::topLeft, false);
    }

    // HUD - top row (title + condition), bottom row (LOCK / BEST / STABILITY / ERR).
    {
        const auto conditionStr = [](SignalCondition c) -> const char* {
            switch (c)
            {
            case SignalCondition::Locked:
                return "LOCKED";
            case SignalCondition::Drifting:
                return "DRIFTING";
            case SignalCondition::Unstable:
                return "UNSTABLE";
            case SignalCondition::Lost:
                return "LOST";
            }
            return "DRIFTING";
        };

        const int padX = static_cast<int>(18.0f * scale_);
        const int padTY = static_cast<int>(14.0f * scale_);
        const int titleW = static_cast<int>(160.0f * scale_);
        const int titleH = static_cast<int>(18.0f * scale_);
        const int condY = static_cast<int>(16.0f * scale_);
        const int condW = static_cast<int>(92.0f * scale_);
        const int condH = static_cast<int>(16.0f * scale_);
        const int condRightInset = static_cast<int>(110.0f * scale_);

        g.setColour(col(kHudText, 0.92f));
        g.setFont(bw::Fonts::mono(13.0f * scale_));
        g.drawText("SIGNAL LOCK", padX, padTY, titleW, titleH, juce::Justification::centredLeft, false);

        g.setColour(
            juce::Colour(activeColor(s.condition)).withAlpha(bws::tokens::barometer::opacity::METER_TEXT_PRIMARY));
        g.setFont(bw::Fonts::mono(11.0f * scale_));
        g.drawText(conditionStr(s.condition), juce::jmax(0, static_cast<int>(width) - condRightInset), condY, condW,
                   condH, juce::Justification::centredRight, false);

        const int rowY = static_cast<int>(height) - static_cast<int>(28.0f * scale_);
        const int colW = static_cast<int>(90.0f * scale_);
        const int labelH = static_cast<int>(10.0f * scale_);
        const int valueH = static_cast<int>(16.0f * scale_);
        const int valueYOffset = static_cast<int>(10.0f * scale_);
        int x = padX;

        const auto stat = [&](const char* label, const juce::String& value, juce::uint32 valueArgb) {
            g.setColour(juce::Colour(kHudLabel));
            g.setFont(bw::Fonts::mono(9.0f * scale_));
            g.drawText(label, x, rowY, colW, labelH, juce::Justification::centredLeft, false);
            g.setColour(juce::Colour(valueArgb).withAlpha(bws::tokens::barometer::opacity::METER_TEXT_PRIMARY));
            g.setFont(bw::Fonts::mono(12.0f * scale_));
            g.drawText(value, x, rowY + valueYOffset, colW, valueH, juce::Justification::centredLeft, false);
            x += colW;
        };

        stat("LOCK", bws::toy_shell::fmtTime4(s.lockSeconds), kHudText);
        stat("BEST", bws::toy_shell::fmtTime4(s.bestSeconds), kCursor);

        // STABILITY - label then drawn dots (filled = remaining reserve).
        g.setColour(juce::Colour(kHudLabel));
        g.setFont(bw::Fonts::mono(9.0f * scale_));
        g.drawText("STABILITY", x, rowY, colW, labelH, juce::Justification::centredLeft, false);
        const float kDotR = 3.5f * scale_;
        const float kDotStride = 12.0f * scale_;
        const float dotsCY = static_cast<float>(rowY) + static_cast<float>(valueYOffset) + (8.0f * scale_);
        const float dotsBaseX = static_cast<float>(x) + (4.0f * scale_);
        for (int i = 0; i < MAX_RESERVES; ++i)
        {
            const float dx = dotsBaseX + static_cast<float>(i) * kDotStride;
            if (i < s.reserves)
            {
                g.setColour(juce::Colour(activeColor(s.condition))
                                .withAlpha(bws::tokens::barometer::opacity::METER_TEXT_PRIMARY));
                g.fillEllipse(dx - kDotR, dotsCY - kDotR, kDotR * 2, kDotR * 2);
            }
            else
            {
                g.setColour(juce::Colour(kHudLabel).withAlpha(bws::tokens::barometer::opacity::SIGNAL_LOCK_DOT_EMPTY));
                g.drawEllipse(dx - kDotR, dotsCY - kDotR, kDotR * 2, kDotR * 2, 1.0f);
            }
        }
        x += colW;

        const int errPct = juce::jlimit(0, 999, static_cast<int>(std::round(s.error * 100.0)));
        stat("ERR", juce::String(errPct) + "%", kHudText);
    }

    // Transition flash overlays
    if (s.reserveFlash > 0.0)
    {
        const float a = static_cast<float>(s.reserveFlash / FLASH_RESERVE_LOST) * 0.3f;
        g.setColour(col(kActiveUnstable, a));
        g.fillRect(bounds);
    }
    if (s.bestFlash > 0.0)
    {
        const float a = static_cast<float>(s.bestFlash / FLASH_NEW_BEST) * 0.2f;
        g.setColour(col(kToleranceBand, a));
        g.fillRect(bounds);
    }
    if (s.lockFlash > 0.0)
    {
        const float a = static_cast<float>(s.lockFlash / FLASH_LOCK_ENTER) * 0.22f;
        const float bandPx = std::max(8.0f, tolerance * kWaveAmplitudePx * 2.0f);
        g.setColour(col(kActiveLocked, a));
        g.fillRect(0.0f, midY - bandPx * 0.5f, width, bandPx);
    }
    if (s.milestoneFlash > 0.0)
    {
        const float a = static_cast<float>(s.milestoneFlash / FLASH_MILESTONE) * 0.35f;
        const float bandPx = std::max(8.0f, tolerance * kWaveAmplitudePx * 2.0f);
        g.setColour(col(kToleranceBand, a));
        g.fillRect(0.0f, midY - bandPx * 0.5f, width, bandPx);
    }
    if (s.solvedFlash > 0.0)
    {
        bws::toy_shell::paintSolvedOverlay(g, bounds, juce::Colour(kToleranceBand), juce::Colour(kHudText),
                                           static_cast<float>(s.solvedFlash / FLASH_SOLVED),
                                           s.solvedFlashIsDisable ? "BACK TO MORTAL" : "GOD MODE UNLOCKED");
    }
}

} // namespace bws::barometer::toy

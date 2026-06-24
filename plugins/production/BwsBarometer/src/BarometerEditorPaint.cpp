// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

// BarometerEditorPaint.cpp - paint methods for BarometerEditor

#include "BarometerEditor.h"
#include "BarometerPaint.h"
#include "ui/BarometerToyOverlay.h"
#include <bw_ui/rendering/JuceRenderer.h>
#include <bw_ui/adapters/UiThemeKernelAdapter.h>
#include <bw_ui/generated/BwTokens.h>
#include <bw_juce_adapters/HostEnvironment.h>
#include <bw_ui/preset_system/WeatherManagePresetsDialog.h>
#include <bw_ui/preset_system/WeatherSavePresetDialog.h>
#include <bw_ui/foundation/UiTheme.h>
#include <array>
#include <vector>

namespace bws::barometer
{

juce::String BarometerEditor::formatGainValue(float gainDb) const
{
    // Handle -infinity display
    if (gainDb <= BarometerProcessor::kGainMinDb)
        return "-\u221E dB"; // -∞ dB

    if (gainDb >= 0.0f)
        return "+" + juce::String(gainDb, 1) + " dB";
    else
        return juce::String(gainDb, 1) + " dB";
}

juce::String BarometerEditor::formatBalanceValue(float balance) const
{
    if (balance < -0.5f)
        return "L " + juce::String(static_cast<int>(std::abs(balance)));
    else if (balance > 0.5f)
        return "R " + juce::String(static_cast<int>(balance));
    else
        return "C";
}

void BarometerEditor::paintInputMeter(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    namespace bp = bws::barometer::paint;
    namespace tok = bws::tokens::barometer;
    bws::ui::rendering::JuceRenderer r(g);

    const auto kernelTheme = makeEditorKernelTheme(getTheme());
    const float leftPeakDb = processor_.getInputLeftPeakDb();
    const float rightPeakDb = processor_.getInputRightPeakDb();
    const bool soloLActive =
        processor_.getApvts().getRawParameterValue(BarometerProcessor::kSoloLParamId)->load() > 0.5f;
    const bool swapActive =
        processor_.getApvts().getRawParameterValue(BarometerProcessor::kSwapLRParamId)->load() > 0.5f;

    constexpr float minDb = -60.0f;
    constexpr float maxDb = 6.0f;
    constexpr float rangeDb = maxDb - minDb;

    const int meterPadding = scaled(2);
    auto innerBounds = bounds.reduced(meterPadding);

    const auto fb = [](juce::Rectangle<int> b) {
        return b.toFloat();
    };
    const auto toRect = [](bp::BarRect br) {
        return juce::Rectangle<int>(juce::roundToInt(br.x), juce::roundToInt(br.y), juce::roundToInt(br.w),
                                    juce::roundToInt(br.h));
    };
    const auto bf = fb(bounds);
    const auto barRects = bp::meterBarRects(bf.getX(), bf.getY(), bf.getWidth(), bf.getHeight(),
                                            static_cast<float>(meterPadding), kMeterBarWidth, kMeterGap);
    auto leftBarBounds = toRect(barRects.left);
    auto rightBarBounds = toRect(barRects.right);

    r.setColour(tok::bg::BASE);
    r.fillRoundedRect(fb(bounds).getX(), fb(bounds).getY(), fb(bounds).getWidth(), fb(bounds).getHeight(), 3.0f);

    // Per-bar hover glow
    if (inputMeterHover_ == MeterBarHover::Left)
    {
        r.setColour(bp::withAlpha(tok::accent::BRASS, tok::opacity::METER_HOVER_GLOW));
        auto e = fb(leftBarBounds).expanded(1.0f);
        r.fillRoundedRect(e.getX(), e.getY(), e.getWidth(), e.getHeight(), 2.0f);
    }
    if (inputMeterHover_ == MeterBarHover::Right)
    {
        r.setColour(bp::withAlpha(tok::accent::BRASS, tok::opacity::METER_HOVER_GLOW));
        auto e = fb(rightBarBounds).expanded(1.0f);
        r.fillRoundedRect(e.getX(), e.getY(), e.getWidth(), e.getHeight(), 2.0f);
    }

    if (inputMeterHovered_ || soloLActive || swapActive)
    {
        r.setColour(bp::withAlpha(tok::accent::BRASS,
                                  inputMeterHovered_ ? tok::opacity::METER_HOVER : tok::opacity::METER_NORMAL));
        auto b = fb(bounds).reduced(0.5f);
        r.drawRoundedRect(b.getX(), b.getY(), b.getWidth(), b.getHeight(), 3.0f, 1.5f);
    }
    else
    {
        r.setColour(tok::text::OUTLINE_BORDER);
        r.drawRoundedRect(fb(bounds).getX(), fb(bounds).getY(), fb(bounds).getWidth(), fb(bounds).getHeight(), 3.0f,
                          1.0f);
    }

    const bp::MeterColors mc {tok::meter::GREEN, tok::meter::YELLOW, tok::meter::RED, tok::meter::CLIP};
    const auto bar = [&](juce::Rectangle<int> bb, float db) {
        auto f = fb(bb);
        bp::meterBar(r, f.getX(), f.getY(), f.getWidth(), f.getHeight(), db, minDb, maxDb, mc);
    };
    bar(leftBarBounds, leftPeakDb);
    bar(rightBarBounds, rightPeakDb);

    const auto rmsArgb = bp::withAlpha(0xFFFFFFu, tok::opacity::METER_TEXT_SECONDARY);
    const auto rms = [&](juce::Rectangle<int> bb, float db) {
        auto f = fb(bb);
        bp::rmsTick(r, f.getX(), f.getY(), f.getWidth(), f.getHeight(), db, minDb, maxDb, rmsArgb);
    };
    rms(leftBarBounds, processor_.getInputLeftRmsDb());
    rms(rightBarBounds, processor_.getInputRightRmsDb());

    const auto holdArgb = bp::withAlpha(0xFFFFFFu, tok::opacity::METER_TEXT_PRIMARY);
    const auto hold = [&](juce::Rectangle<int> bb, float db) {
        auto f = fb(bb);
        bp::peakHoldLine(r, f.getX(), f.getY(), f.getWidth(), f.getHeight(), db, minDb, maxDb, holdArgb);
    };
    hold(leftBarBounds, peakHoldInputL_.value);
    hold(rightBarBounds, peakHoldInputR_.value);

    // L/R labels (themed font selected on g, drawn through the renderer)
    g.setFont(bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::Annotation, getScaleFactor()));
    r.setColour(tok::text::METER_LABEL);
    r.drawText("L", static_cast<float>(leftBarBounds.getX()), static_cast<float>(bounds.getBottom() + scaled(1)),
               static_cast<float>(leftBarBounds.getWidth()), scaledF(8.0f), bws::ui::rendering::Justification::Centre);
    r.drawText("R", static_cast<float>(rightBarBounds.getX()), static_cast<float>(bounds.getBottom() + scaled(1)),
               static_cast<float>(rightBarBounds.getWidth()), scaledF(8.0f), bws::ui::rendering::Justification::Centre);

    // Solo / swap indicator dots
    const bool soloRActive =
        processor_.getApvts().getRawParameterValue(BarometerProcessor::kSoloRParamId)->load() > 0.5f;
    const float dotSize = scaledF(4.0f);
    const float dotInset = scaledF(2.0f);
    if (soloLActive && soloLClickedOnInput_)
    {
        r.setColour(tok::indicator::SOLO_YELLOW);
        r.fillEllipse(static_cast<float>(leftBarBounds.getCentreX()) - dotInset,
                      static_cast<float>(bounds.getY() + scaled(2)), dotSize, dotSize);
    }
    if (soloRActive && soloRClickedOnInput_)
    {
        r.setColour(tok::indicator::SOLO_YELLOW);
        r.fillEllipse(static_cast<float>(rightBarBounds.getCentreX()) - dotInset,
                      static_cast<float>(bounds.getY() + scaled(2)), dotSize, dotSize);
    }
    if (swapActive)
    {
        const bool anySoloOnInput = (soloLActive && soloLClickedOnInput_) || (soloRActive && soloRClickedOnInput_);
        const float dotY = anySoloOnInput ? static_cast<float>(bounds.getY() + scaled(8))
                                          : static_cast<float>(bounds.getY() + scaled(2));
        r.setColour(tok::accent::BRASS);
        r.fillEllipse(static_cast<float>(bounds.getRight() - scaled(6)), dotY, dotSize, dotSize);
    }

    // dB scale ticks on the left side
    g.setFont(bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::Annotation, getScaleFactor()));
    const std::array<float, 8> tickLevels = {6.0f, 0.0f, -6.0f, -12.0f, -18.0f, -24.0f, -36.0f, -48.0f};
    const std::array<const char*, 8> tickLabels = {"+6", "0", "-6", "-12", "-18", "-24", "-36", "-48"};
    const std::array<bool, 8> showLabel = {true, true, false, true, false, true, false, true};

    for (size_t i = 0; i < tickLevels.size(); ++i)
    {
        const float tickNorm = (tickLevels[i] - minDb) / rangeDb;
        const int tickY =
            innerBounds.getBottom() - static_cast<int>(tickNorm * static_cast<float>(innerBounds.getHeight()));

        r.setColour(tok::text::PHASE_BORDER);
        r.drawLine(static_cast<float>(bounds.getX() - scaled(3)), static_cast<float>(tickY),
                   static_cast<float>(bounds.getX()), static_cast<float>(tickY), 1.0f);

        if (showLabel[i])
        {
            r.setColour(tok::text::TICK_LABEL);
            r.drawText(tickLabels[i], static_cast<float>(bounds.getX() - scaled(24)),
                       static_cast<float>(tickY - scaled(4)), scaledF(20.0f), scaledF(8.0f),
                       bws::ui::rendering::Justification::Right);
        }
    }
}

void BarometerEditor::paintOutputMeter(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    namespace bp = bws::barometer::paint;
    namespace tok = bws::tokens::barometer;
    bws::ui::rendering::JuceRenderer r(g);

    const auto kernelTheme = makeEditorKernelTheme(getTheme());
    const float leftPeakDb = processor_.getLeftPeakDb();
    const float rightPeakDb = processor_.getRightPeakDb();
    const bool soloRActive =
        processor_.getApvts().getRawParameterValue(BarometerProcessor::kSoloRParamId)->load() > 0.5f;
    const bool swapActive =
        processor_.getApvts().getRawParameterValue(BarometerProcessor::kSwapLRParamId)->load() > 0.5f;

    constexpr float minDb = -60.0f;
    constexpr float maxDb = 6.0f;
    constexpr float rangeDb = maxDb - minDb;

    const int meterPadding = scaled(2);
    auto innerBounds = bounds.reduced(meterPadding);

    const auto fb = [](juce::Rectangle<int> b) {
        return b.toFloat();
    };
    const auto toRect = [](bp::BarRect br) {
        return juce::Rectangle<int>(juce::roundToInt(br.x), juce::roundToInt(br.y), juce::roundToInt(br.w),
                                    juce::roundToInt(br.h));
    };
    const auto bf = fb(bounds);
    const auto barRects = bp::meterBarRects(bf.getX(), bf.getY(), bf.getWidth(), bf.getHeight(),
                                            static_cast<float>(meterPadding), kMeterBarWidth, kMeterGap);
    auto leftBarBounds = toRect(barRects.left);
    auto rightBarBounds = toRect(barRects.right);

    r.setColour(tok::bg::BASE);
    r.fillRoundedRect(fb(bounds).getX(), fb(bounds).getY(), fb(bounds).getWidth(), fb(bounds).getHeight(), 3.0f);

    // Per-bar hover glow
    if (outputMeterHover_ == MeterBarHover::Left)
    {
        r.setColour(bp::withAlpha(tok::accent::BRASS, tok::opacity::METER_HOVER_GLOW));
        auto e = fb(leftBarBounds).expanded(1.0f);
        r.fillRoundedRect(e.getX(), e.getY(), e.getWidth(), e.getHeight(), 2.0f);
    }
    if (outputMeterHover_ == MeterBarHover::Right)
    {
        r.setColour(bp::withAlpha(tok::accent::BRASS, tok::opacity::METER_HOVER_GLOW));
        auto e = fb(rightBarBounds).expanded(1.0f);
        r.fillRoundedRect(e.getX(), e.getY(), e.getWidth(), e.getHeight(), 2.0f);
    }

    if (outputMeterHovered_ || soloRActive || swapActive)
    {
        r.setColour(bp::withAlpha(tok::accent::BRASS,
                                  outputMeterHovered_ ? tok::opacity::METER_HOVER : tok::opacity::METER_NORMAL));
        auto b = fb(bounds).reduced(0.5f);
        r.drawRoundedRect(b.getX(), b.getY(), b.getWidth(), b.getHeight(), 3.0f, 1.5f);
    }
    else
    {
        r.setColour(tok::text::OUTLINE_BORDER);
        r.drawRoundedRect(fb(bounds).getX(), fb(bounds).getY(), fb(bounds).getWidth(), fb(bounds).getHeight(), 3.0f,
                          1.0f);
    }

    const bp::MeterColors mc {tok::meter::GREEN, tok::meter::YELLOW, tok::meter::RED, tok::meter::CLIP};
    const auto bar = [&](juce::Rectangle<int> bb, float db) {
        auto f = fb(bb);
        bp::meterBar(r, f.getX(), f.getY(), f.getWidth(), f.getHeight(), db, minDb, maxDb, mc);
    };
    bar(leftBarBounds, leftPeakDb);
    bar(rightBarBounds, rightPeakDb);

    const auto rmsArgb = bp::withAlpha(0xFFFFFFu, tok::opacity::METER_TEXT_SECONDARY);
    const auto rms = [&](juce::Rectangle<int> bb, float db) {
        auto f = fb(bb);
        bp::rmsTick(r, f.getX(), f.getY(), f.getWidth(), f.getHeight(), db, minDb, maxDb, rmsArgb);
    };
    rms(leftBarBounds, processor_.getOutputLeftRmsDb());
    rms(rightBarBounds, processor_.getOutputRightRmsDb());

    const auto holdArgb = bp::withAlpha(0xFFFFFFu, tok::opacity::METER_TEXT_PRIMARY);
    const auto hold = [&](juce::Rectangle<int> bb, float db) {
        auto f = fb(bb);
        bp::peakHoldLine(r, f.getX(), f.getY(), f.getWidth(), f.getHeight(), db, minDb, maxDb, holdArgb);
    };
    hold(leftBarBounds, peakHoldOutputL_.value);
    hold(rightBarBounds, peakHoldOutputR_.value);

    // L/R labels (themed font selected on g, drawn through the renderer)
    g.setFont(bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::Annotation, getScaleFactor()));
    r.setColour(tok::text::METER_LABEL);
    r.drawText("L", static_cast<float>(leftBarBounds.getX()), static_cast<float>(bounds.getBottom() + scaled(1)),
               static_cast<float>(leftBarBounds.getWidth()), scaledF(8.0f), bws::ui::rendering::Justification::Centre);
    r.drawText("R", static_cast<float>(rightBarBounds.getX()), static_cast<float>(bounds.getBottom() + scaled(1)),
               static_cast<float>(rightBarBounds.getWidth()), scaledF(8.0f), bws::ui::rendering::Justification::Centre);

    // Solo / swap indicator dots
    const bool soloLActive =
        processor_.getApvts().getRawParameterValue(BarometerProcessor::kSoloLParamId)->load() > 0.5f;
    const float dotSize = scaledF(4.0f);
    const float dotInset = scaledF(2.0f);
    if (soloLActive && !soloLClickedOnInput_)
    {
        r.setColour(tok::indicator::SOLO_YELLOW);
        r.fillEllipse(static_cast<float>(leftBarBounds.getCentreX()) - dotInset,
                      static_cast<float>(bounds.getY() + scaled(2)), dotSize, dotSize);
    }
    if (soloRActive && !soloRClickedOnInput_)
    {
        r.setColour(tok::indicator::SOLO_YELLOW);
        r.fillEllipse(static_cast<float>(rightBarBounds.getCentreX()) - dotInset,
                      static_cast<float>(bounds.getY() + scaled(2)), dotSize, dotSize);
    }
    if (swapActive)
    {
        const bool anySoloOnOutput = (soloLActive && !soloLClickedOnInput_) || (soloRActive && !soloRClickedOnInput_);
        const float dotY = anySoloOnOutput ? static_cast<float>(bounds.getY() + scaled(8))
                                           : static_cast<float>(bounds.getY() + scaled(2));
        r.setColour(tok::accent::BRASS);
        r.fillEllipse(static_cast<float>(bounds.getX() + scaled(2)), dotY, dotSize, dotSize);
    }

    // dB scale ticks on the right side
    g.setFont(bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::Annotation, getScaleFactor()));
    const std::array<float, 8> tickLevels = {6.0f, 0.0f, -6.0f, -12.0f, -18.0f, -24.0f, -36.0f, -48.0f};
    const std::array<const char*, 8> tickLabels = {"+6", "0", "-6", "-12", "-18", "-24", "-36", "-48"};
    const std::array<bool, 8> showLabel = {true, true, false, true, false, true, false, true};

    for (size_t i = 0; i < tickLevels.size(); ++i)
    {
        const float tickNorm = (tickLevels[i] - minDb) / rangeDb;
        const int tickY =
            innerBounds.getBottom() - static_cast<int>(tickNorm * static_cast<float>(innerBounds.getHeight()));

        r.setColour(tok::text::PHASE_BORDER);
        r.drawLine(static_cast<float>(bounds.getRight()), static_cast<float>(tickY),
                   static_cast<float>(bounds.getRight() + scaled(3)), static_cast<float>(tickY), 1.0f);

        if (showLabel[i])
        {
            r.setColour(tok::text::TICK_LABEL);
            r.drawText(tickLabels[i], static_cast<float>(bounds.getRight() + scaled(4)),
                       static_cast<float>(tickY - scaled(4)), scaledF(22.0f), scaledF(8.0f),
                       bws::ui::rendering::Justification::Left);
        }
    }
}

void BarometerEditor::paintLufsReadout(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    const auto kernelTheme = makeEditorKernelTheme(getTheme());
    // Two-tier layout: Hero row (I + TP) on top, secondary row (M, S, LRA) below

    const float momentary = processor_.getMomentaryLufs();
    const float shortTerm = processor_.getShortTermLufs();
    const float integrated = processor_.getIntegratedLufs();
    const float truePeak = processor_.getHeldTruePeakDb();
    const float lra = processor_.getLoudnessRange();
    // render LRA as "---" until stable (≥60 s + non-empty gated
    //  distribution per EBU Tech 3342 stability convention).
    const bool lraStable = processor_.isLoudnessRangeStable();

    // Format helpers
    auto formatLufs = [](float val) -> juce::String {
        if (!std::isfinite(val) || val <= -100.0F)
        {
            return "---";
        }
        return juce::String(val, 1);
    };
    auto formatTp = [](float val) -> juce::String {
        if (!std::isfinite(val) || val <= -100.0F)
        {
            return "---";
        }
        return juce::String(val, 1);
    };
    auto formatLra = [lraStable](float val) -> juce::String {
        if (!lraStable || !std::isfinite(val) || val < 0.0F)
        {
            return "---";
        }
        return juce::String(val, 1);
    };

    bws::ui::rendering::JuceRenderer r(g);
    namespace bp = bws::barometer::paint;

    // Subtle separator line above the readout
    r.setColour(
        bp::withAlpha(bws::tokens::barometer::text::OUTLINE_BORDER, bws::tokens::barometer::opacity::SEPARATOR_LINE));
    r.drawLine(static_cast<float>(bounds.getX()), static_cast<float>(bounds.getY()),
               static_cast<float>(bounds.getRight()), static_cast<float>(bounds.getY()), 1.0f);

    // Colors
    const auto labelColor = juce::Colour(bws::tokens::barometer::text::TICK_LABEL);
    const auto valueColor =
        juce::Colour(kReadoutTextColor).withAlpha(bws::tokens::barometer::opacity::METER_TEXT_SECONDARY);
    const auto dimColor = juce::Colour(kReadoutTextColor).withAlpha(bws::tokens::barometer::opacity::READOUT_DIM);
    const auto brassColor = juce::Colour(kPhaseActiveColor);
    const auto redColor = juce::Colour(bws::tokens::barometer::indicator::LUFS_CLIP_INDICATOR);

    // =========================================================================
    // HERO ROW - Integrated LUFS + True Peak (uniform spacing)
    // =========================================================================
    const int heroY = bounds.getY() + scaled(3);
    const int heroH = scaled(18);

    // Uniform structure: both I and TP use identical slot widths
    // [label 16] [pad 3] [value 66] [pad 3] [extra 40] ... [section gap 12] ...
    const int slotLabel = scaled(16);  // "I" or "TP" - same width for both
    const int slotPad = scaled(3);     // uniform gap between every element
    const int slotValue = scaled(66);  // value field - fits "-17.2 LUFS" and "-5.0 dBTP"
    const int slotExtra = scaled(40);  // deviation or badge slot
    const int sectionGap = scaled(12); // breathing room between I and TP sections

    // Determine I-section state
    juce::Colour iColor = valueColor;
    bool iOnTarget = false;
    juce::String deviationText;

    if (lufsTarget_ != LufsTarget::Off && std::isfinite(integrated) && integrated > -100.0F)
    {
        const float target = getLufsTargetValue();
        const float diff = integrated - target;

        if (diff >= 0.0F)
        {
            deviationText = "+" + juce::String(diff, 1) + " LU";
        }
        else
        {
            deviationText = juce::String(diff, 1) + " LU";
        }

        if (diff >= -1.0F && diff <= 1.0F)
        {
            iColor = juce::Colour(bws::tokens::barometer::meter::GREEN);
            iOnTarget = true;
        }
        else if (diff > 1.0F && diff <= 3.0F)
        {
            iColor = juce::Colour(bws::tokens::barometer::meter::YELLOW);
        }
        else if (diff > 3.0F)
        {
            iColor = juce::Colour(bws::tokens::barometer::meter::RED);
        }
    }

    // Determine TP state
    juce::Colour tpColor = valueColor;
    juce::String tpStatus;

    if (std::isfinite(truePeak) && truePeak > -100.0F)
    {
        if (truePeak > 0.0F)
        {
            tpColor = redColor;
            tpStatus = "CLIP";
        }
        else if (truePeak > -1.0F)
        {
            tpColor = brassColor;
            tpStatus = "HOT";
        }
    }

    // Each section: label + pad + value + pad + extra = consistent per section
    const bool hasDeviation = deviationText.isNotEmpty();
    const bool hasBadge = tpStatus.isNotEmpty();
    const int oneSectionW = slotLabel + slotPad + slotValue + slotPad + slotExtra;
    const int totalHeroW = oneSectionW + sectionGap + oneSectionW;
    const int heroStartX = bounds.getX() + ((bounds.getWidth() - totalHeroW) / 2);

    // --- Integrated LUFS ---
    {
        const auto heroFont =
            bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::Readout, getScaleFactor());

        int cx = heroStartX;

        // "I" label
        g.setFont(bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::ControlText, getScaleFactor()));
        r.setColour(labelColor.getARGB());
        r.drawText("I", static_cast<float>(cx), static_cast<float>(heroY), static_cast<float>(slotLabel),
                   static_cast<float>(heroH), bws::ui::rendering::Justification::Left);
        cx += slotLabel + slotPad;

        // Value
        auto iValueBounds = juce::Rectangle<int>(cx, heroY, slotValue, heroH);

        if (iOnTarget)
        {
            auto hb = iValueBounds.toFloat().reduced(0.0F, scaledF(2.0F));
            r.setColour(iColor.withAlpha(bws::tokens::barometer::opacity::SLIDER_SHADOW).getARGB());
            r.fillRoundedRect(hb.getX(), hb.getY(), hb.getWidth(), hb.getHeight(), scaledF(3.0F));
        }

        g.setFont(heroFont);
        r.setColour(iColor.getARGB());
        const auto iStr = formatLufs(integrated) + " LUFS";
        r.drawText(iStr.toRawUTF8(), static_cast<float>(iValueBounds.getX()), static_cast<float>(iValueBounds.getY()),
                   static_cast<float>(iValueBounds.getWidth()), static_cast<float>(iValueBounds.getHeight()),
                   bws::ui::rendering::Justification::Left);
        cx += slotValue + slotPad;

        // Deviation
        if (hasDeviation)
        {
            g.setFont(
                bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::Annotation, getScaleFactor()));
            r.drawText(deviationText.toRawUTF8(), static_cast<float>(cx), static_cast<float>(heroY),
                       static_cast<float>(slotExtra), static_cast<float>(heroH),
                       bws::ui::rendering::Justification::Left);
        }
    }

    // --- True Peak ---
    {
        const auto heroFont =
            bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::Readout, getScaleFactor());

        int cx = heroStartX + oneSectionW + sectionGap;

        // "TP" label
        g.setFont(bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::ControlText, getScaleFactor()));
        r.setColour(labelColor.getARGB());
        r.drawText("TP", static_cast<float>(cx), static_cast<float>(heroY), static_cast<float>(slotLabel),
                   static_cast<float>(heroH), bws::ui::rendering::Justification::Left);
        cx += slotLabel + slotPad;

        // Value
        g.setFont(heroFont);
        r.setColour(tpColor.getARGB());
        const auto tpStr = formatTp(truePeak) + " dBTP";
        r.drawText(tpStr.toRawUTF8(), static_cast<float>(cx), static_cast<float>(heroY), static_cast<float>(slotValue),
                   static_cast<float>(heroH), bws::ui::rendering::Justification::Left);
        cx += slotValue + slotPad;

        // Badge
        if (hasBadge)
        {
            auto badgeBounds = juce::Rectangle<int>(cx, heroY + scaled(3), scaled(28), scaled(12)).toFloat();
            r.setColour(tpColor.withAlpha(bws::tokens::barometer::opacity::STATUS_BADGE_FILL).getARGB());
            r.fillRoundedRect(badgeBounds.getX(), badgeBounds.getY(), badgeBounds.getWidth(), badgeBounds.getHeight(),
                              scaledF(2.0F));
            g.setFont(
                bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::Annotation, getScaleFactor()));
            r.setColour(tpColor.getARGB());
            r.drawText(tpStatus.toRawUTF8(), badgeBounds.getX(), badgeBounds.getY(), badgeBounds.getWidth(),
                       badgeBounds.getHeight(), bws::ui::rendering::Justification::Centre);
        }
    }

    // =========================================================================
    // SECONDARY ROW - M, S, LRA (anchored to hero row bounds, dimmed)
    // =========================================================================
    const int secY = heroY + heroH + scaled(2);
    const int secH = scaled(12);
    const auto secFont =
        bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::Annotation, getScaleFactor());
    g.setFont(secFont);
    r.setColour(dimColor.getARGB());

    // Anchor to the same bounding box as the hero row - 3 equal columns within it
    const int secColW = totalHeroW / 3;

    const auto mStr = "M: " + formatLufs(momentary);
    r.drawText(mStr.toRawUTF8(), static_cast<float>(heroStartX), static_cast<float>(secY), static_cast<float>(secColW),
               static_cast<float>(secH), bws::ui::rendering::Justification::Centre);

    const auto sStr = "S: " + formatLufs(shortTerm);
    r.drawText(sStr.toRawUTF8(), static_cast<float>(heroStartX + secColW), static_cast<float>(secY),
               static_cast<float>(secColW), static_cast<float>(secH), bws::ui::rendering::Justification::Centre);

    const auto lraStr = "LRA: " + formatLra(lra) + " LU";
    r.drawText(lraStr.toRawUTF8(), static_cast<float>(heroStartX + (secColW * 2)), static_cast<float>(secY),
               static_cast<float>(secColW), static_cast<float>(secH), bws::ui::rendering::Justification::Centre);

    // Sparkline - visual trend below numeric readouts
    const auto boundsF = bounds.toFloat();
    const float sparklineTop = static_cast<float>(secY + secH) + scaledF(2.0F);
    const float sparklineH = scaledF(16.0F);
    const float sparklineLabelW = scaledF(10.0F);

    // "S" label at left edge - identifies sparkline as short-term LUFS
    g.setFont(bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::Annotation, getScaleFactor()));
    r.setColour(dimColor.getARGB());
    r.drawText("S", boundsF.getX(), sparklineTop, sparklineLabelW, sparklineH, bws::ui::rendering::Justification::Left);

    paintSparkline(g,
                   {boundsF.getX() + sparklineLabelW, sparklineTop, boundsF.getWidth() - sparklineLabelW, sparklineH});
}

void BarometerEditor::paintSparkline(juce::Graphics& g, juce::Rectangle<float> area) const
{
    bws::ui::rendering::JuceRenderer r(g);
    const auto strokeArgb = bws::barometer::paint::withAlpha(bws::tokens::barometer::accent::LINK_BLUE,
                                                             bws::tokens::barometer::opacity::SPARKLINE_STROKE);
    bws::barometer::paint::sparkline(r, sparklineHistory_, sparklineWritePos_, area.getX(), area.getY(),
                                     area.getWidth(), area.getHeight(), scaledF(1.0F), strokeArgb);
}

void BarometerEditor::paintContent(juce::Graphics& g)
{
    namespace bp = bws::barometer::paint;
    namespace tok = bws::tokens::barometer;
    namespace rr = bws::ui::rendering;
    bws::ui::rendering::JuceRenderer r(g);

    const auto kernelTheme = makeEditorKernelTheme(getTheme());
    const auto fb = [](juce::Rectangle<int> b) {
        return b.toFloat();
    };

    // =========================================================================
    // Title: "BAROMETER"
    // =========================================================================
    {
        auto titleArea = fb(juce::Rectangle<int>(0, scaled(kTopMargin), getWidth(), scaled(kTitleHeight)));
        g.setFont(bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::Title, getScaleFactor()));
        r.setColour(juce::Colour(kTitleColor).getARGB());
        r.drawText("BAROMETER", titleArea.getX(), titleArea.getY(), titleArea.getWidth(), titleArea.getHeight(),
                   rr::Justification::Centre);
    }

    // =========================================================================
    // ontrols Label (PHASE label above the 4-button group)
    // =========================================================================
#if !USE_MINIMAL_ICONS
    {
        // Get the center of the phase button group (L, Link, R, M)
        auto phaseLBounds = phaseLButton_->getBounds();
        auto monoBounds = monoButton_->getBounds();
        const int phaseGroupCenterX = (phaseLBounds.getX() + monoBounds.getRight()) / 2;

        auto labelBounds = juce::Rectangle<int>(phaseGroupCenterX - scaled(kPhaseGroupWidth) / 2,
                                                phaseLBounds.getY() - scaled(kLabelHeight) - scaled(kLabelToControlGap),
                                                scaled(kPhaseGroupWidth), scaled(kLabelHeight));
        g.setFont(bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::ControlText, getScaleFactor()));
        r.setColour(juce::Colour(kBrandingColor).getARGB());
        // Use centredBottom for text baseline alignment with Balance/Width tabs
        auto lb = fb(labelBounds);
        r.drawText("PHASE", lb.getX(), lb.getY(), lb.getWidth(), lb.getHeight(), rr::Justification::CentreBottom);
    }
#endif

    // =========================================================================
    // Balance/Width Slider Custom Paint (clean track, value above)
    // =========================================================================
    {
        // Use the active slider bounds (balance or width)
        auto sliderBounds = balanceWidthMode_ == 0 ? balanceSlider_->getBounds() : widthSlider_->getBounds();

        // Track background - inset appearance (recessed into panel)
        const int trackHeight = scaled(8);
        const int trackY = sliderBounds.getCentreY() - trackHeight / 2;
        auto trackBounds = juce::Rectangle<int>(sliderBounds.getX(), trackY, sliderBounds.getWidth(), trackHeight);

        // Dark recessed track
        const float cornerRadius = scaledF(4.0f);
        auto tf = fb(trackBounds);
        r.setColour(juce::Colour(tok::bg::BASE).getARGB());
        r.fillRoundedRect(tf.getX(), tf.getY(), tf.getWidth(), tf.getHeight(), cornerRadius);

        // Inner shadow for depth
        auto tfr = tf.reduced(0.5f);
        r.setColour(juce::Colour(tok::slider::TRACK_SHADOW).getARGB());
        r.drawRoundedRect(tfr.getX(), tfr.getY(), tfr.getWidth(), tfr.getHeight(), cornerRadius, 1.0f);

        // Track border
        r.setColour(juce::Colour(kPhaseBorderColor).getARGB());
        r.drawRoundedRect(tf.getX(), tf.getY(), tf.getWidth(), tf.getHeight(), cornerRadius, 1.0f);

        // Notch position and thumb calculation depends on mode
        float normalizedPos;
        juce::String valueText;
        juce::String leftMarker, rightMarker;

        if (balanceWidthMode_ == 0)
        {
            // Balance mode
            const float balance = static_cast<float>(balanceSlider_->getValue());
            normalizedPos = (balance + 100.0f) / 200.0f;
            valueText = formatBalanceValue(balance);
            leftMarker = "L";
            rightMarker = "R";

            // Center notch at 0 position
            const int centerX = sliderBounds.getCentreX();
            r.setColour(juce::Colour(tok::text::OUTLINE_BORDER).getARGB());
            r.drawLine(static_cast<float>(centerX), static_cast<float>(trackY + scaled(2)), static_cast<float>(centerX),
                       static_cast<float>(trackY + trackHeight - scaled(2)), 1.5f);
        }
        else
        {
            // Width mode
            const float width = static_cast<float>(widthSlider_->getValue());
            normalizedPos = width / 2.0f; // 0.0-2.0 maps to 0.0-1.0
            valueText = juce::String(static_cast<int>(width * 100.0f)) + "%";
            leftMarker = "0";
            rightMarker = "200";

            // Notch at 100% (center) position
            const int centerX = sliderBounds.getCentreX();
            r.setColour(juce::Colour(tok::text::OUTLINE_BORDER).getARGB());
            r.drawLine(static_cast<float>(centerX), static_cast<float>(trackY + scaled(2)), static_cast<float>(centerX),
                       static_cast<float>(trackY + trackHeight - scaled(2)), 1.5f);
        }

        // Thumb position - slightly larger for premium feel (20px vs 16px)
        const int thumbSize = scaled(20);
        const int thumbX = sliderBounds.getX() +
                           static_cast<int>(normalizedPos * static_cast<float>(sliderBounds.getWidth() - thumbSize));
        const int thumbY = sliderBounds.getCentreY() - thumbSize / 2;

        // Premium thumb with radial gradient (high-end hardware look)
        auto thumbBounds = juce::Rectangle<int>(thumbX, thumbY, thumbSize, thumbSize).toFloat();
        const float thumbCx = thumbBounds.getCentreX();
        const float thumbCy = thumbBounds.getCentreY();
        const float thumbRadius = thumbBounds.getWidth() / 2.0f;

        // 1. Base radial gradient (creates depth/dimensionality)
        {
            auto tg = r.createRadialGradient(thumbCx - thumbRadius * 0.3f, thumbCy - thumbRadius * 0.3f, thumbCx,
                                             thumbCy + thumbRadius);
            r.addGradientStop(tg, 0.0f, juce::Colour(tok::slider::THUMB_BRIGHT).getARGB());
            r.addGradientStop(tg, 1.0f, juce::Colour(tok::slider::THUMB_DARK).getARGB());
            r.applyGradient(tg);
            r.destroyGradient(tg);
            r.fillEllipse(thumbBounds.getX(), thumbBounds.getY(), thumbBounds.getWidth(), thumbBounds.getHeight());
        }

        // The thumb circle used to clip the highlight/shadow fills.
        const auto clipToThumb = [&]() {
            r.beginPath();
            r.arcTo(thumbBounds.getCentreX(), thumbBounds.getCentreY(), thumbRadius, 0.0f,
                    juce::MathConstants<float>::twoPi);
            r.closePath();
            r.clipPath();
        };

        // 2. Top highlight (simulates light source from above)
        {
            auto hb = thumbBounds.reduced(thumbRadius * 0.15f);
            hb = hb.withHeight(hb.getHeight() * 0.5f);
            r.saveState();
            clipToThumb();
            auto hg = r.createLinearGradient(hb.getCentreX(), hb.getY(), hb.getCentreX(), hb.getBottom());
            r.addGradientStop(hg, 0.0f, juce::Colours::white.withAlpha(tok::opacity::SLIDER_HIGHLIGHT).getARGB());
            r.addGradientStop(hg, 1.0f, juce::Colours::transparentWhite.getARGB());
            r.applyGradient(hg);
            r.destroyGradient(hg);
            r.fillRect(hb.getX(), hb.getY(), hb.getWidth(), hb.getHeight());
            r.restoreState();
        }

        // 3. Bottom shadow (subtle, for 3D effect)
        {
            auto sb = thumbBounds.reduced(thumbRadius * 0.15f);
            sb = sb.withTop(sb.getCentreY());
            r.saveState();
            clipToThumb();
            auto sg = r.createLinearGradient(sb.getCentreX(), sb.getY(), sb.getCentreX(), sb.getBottom());
            r.addGradientStop(sg, 0.0f, juce::Colours::transparentBlack.getARGB());
            r.addGradientStop(sg, 1.0f, juce::Colours::black.withAlpha(tok::opacity::SLIDER_SHADOW).getARGB());
            r.applyGradient(sg);
            r.destroyGradient(sg);
            r.fillRect(sb.getX(), sb.getY(), sb.getWidth(), sb.getHeight());
            r.restoreState();
        }

        // 4. Outer ring (precision machined look)
        auto ring1 = thumbBounds.reduced(0.5f);
        r.setColour(juce::Colour(tok::slider::THUMB_RING).getARGB());
        r.drawEllipse(ring1.getX(), ring1.getY(), ring1.getWidth(), ring1.getHeight(), 1.0f);

        // 5. Inner bevel highlight (adds edge definition)
        auto ring2 = thumbBounds.reduced(scaledF(1.5f));
        r.setColour(juce::Colour(tok::accent::BRASS).withAlpha(tok::opacity::SLIDER_RING).getARGB());
        r.drawEllipse(ring2.getX(), ring2.getY(), ring2.getWidth(), ring2.getHeight(), 0.5f);

        // Markers at ends - positioned BELOW the track for better visibility
        // Use precise centering relative to thumb positions at extremes
        const int markerY = trackY + trackHeight + scaled(3);
        const int markerH = scaled(12);
        g.setFont(bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::Annotation, getScaleFactor()));
        r.setColour(juce::Colour(tok::text::TICK_LABEL).getARGB());

        // Reuse thumbSize from above for centering (thumb center at extremes)
        const int thumbCenterOffset = thumbSize / 2;

        // Left marker - center under where thumb would be at left extreme
        const int leftMarkerX = sliderBounds.getX() + thumbCenterOffset;
        const int markerTextW = scaled(24); // Wide enough for "200"
        r.drawText(leftMarker.toRawUTF8(), static_cast<float>(leftMarkerX - markerTextW / 2),
                   static_cast<float>(markerY), static_cast<float>(markerTextW), static_cast<float>(markerH),
                   rr::Justification::Centre);

        // Right marker - center under where thumb would be at right extreme
        const int rightMarkerX = sliderBounds.getRight() - thumbCenterOffset;
        r.drawText(rightMarker.toRawUTF8(), static_cast<float>(rightMarkerX - markerTextW / 2),
                   static_cast<float>(markerY), static_cast<float>(markerTextW), static_cast<float>(markerH),
                   rr::Justification::Centre);

        // Value readout ABOVE the tab buttons, aligned with PHASE label baseline
        // Tab buttons are at: controlsY - tabH - 4px gap
        // Value readout should be above tabs: tabY - valueHeight - gap
        const int tabH = scaled(kTabButtonHeight);
        const int tabY = sliderBounds.getY() - tabH - scaled(4); // Match resized() calculation
        const int valueY = tabY - scaled(kValueLabelHeight) - scaled(kValueToSliderGap);
        auto valueArea =
            juce::Rectangle<int>(sliderBounds.getX(), valueY, sliderBounds.getWidth(), scaled(kValueLabelHeight));
        g.setFont(bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::Readout, getScaleFactor()));
        r.setColour(juce::Colour(kReadoutTextColor).withAlpha(tok::opacity::SLIDER_READOUT).getARGB());
        // Use centredBottom for text baseline alignment with PHASE label
        auto va = fb(valueArea);
        r.drawText(valueText.toRawUTF8(), va.getX(), va.getY(), va.getWidth(), va.getHeight(),
                   rr::Justification::CentreBottom);
    }

    // =========================================================================
    // Input Meter (left side of dial) - shows levels BEFORE processing
    // =========================================================================
    if (!inputMeterBounds_.isEmpty())
    {
        paintInputMeter(g, inputMeterBounds_);

        // "IN" label below input meter (improved readability)
        g.setFont(bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::Annotation, getScaleFactor()));
        r.setColour(juce::Colour(tok::text::IN_OUT_LABEL).getARGB());
        r.drawText("IN", static_cast<float>(inputMeterBounds_.getX()),
                   static_cast<float>(inputMeterBounds_.getBottom() + scaled(10)),
                   static_cast<float>(inputMeterBounds_.getWidth()), static_cast<float>(scaled(10)),
                   rr::Justification::Centre);
    }

    // =========================================================================
    // Output Meter (right side of dial) - shows levels AFTER processing
    // =========================================================================
    if (!meterBounds_.isEmpty())
    {
        paintOutputMeter(g, meterBounds_);

        // "OUT" label below output meter (improved readability)
        g.setFont(bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::Annotation, getScaleFactor()));
        r.setColour(juce::Colour(tok::text::IN_OUT_LABEL).getARGB());
        r.drawText("OUT", static_cast<float>(meterBounds_.getX() - 2),
                   static_cast<float>(meterBounds_.getBottom() + scaled(10)),
                   static_cast<float>(meterBounds_.getWidth() + 4), static_cast<float>(scaled(10)),
                   rr::Justification::Centre);
    }

    // =========================================================================
    // LUFS / True Peak Readout Strip
    // =========================================================================
    if (!lufsReadoutBounds_.isEmpty())
    {
        paintLufsReadout(g, lufsReadoutBounds_);
    }

    // =========================================================================
    // Branding: "BELLWEATHER STUDIOS"
    // =========================================================================
    {
        // Position brand name with proper gap below controls
        const int brandY = getHeight() - scaled(kBottomMargin) - scaled(kBrandHeight);
        auto brandingArea = fb(juce::Rectangle<int>(0, brandY, getWidth(), scaled(kBrandHeight)));
        g.setFont(bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::ControlText, getScaleFactor()));
        r.setColour(juce::Colour(kBrandingColor).getARGB());
        r.drawText("BELLWEATHER STUDIOS", brandingArea.getX(), brandingArea.getY(), brandingArea.getWidth(),
                   brandingArea.getHeight(), rr::Justification::Centre);
    }
}

} // namespace bws::barometer

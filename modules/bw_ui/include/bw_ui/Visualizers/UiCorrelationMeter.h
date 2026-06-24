// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * @file UiCorrelationMeter.h
 * @brief Pro-level stereo phase correlation meter for audio plugin UIs
 *
 * DESIGN GOALS:
 * - Professional visualization inspired by iZotope Insight, Nugen VisLM, Waves PAZ
 * - Shows stereo phase correlation from -1 (out of phase) to +1 (mono/in phase)
 * - Multiple display modes: Bar, Needle, Histogram
 * - Thread-safe audio→UI communication
 *
 * CORRELATION VALUES:
 * - +1: Perfectly correlated (mono compatible)
 * -  0: Uncorrelated (wide stereo)
 * - -1: Perfectly out of phase (phase cancellation risk)
 *
 * USAGE:
 *   // In your editor:
 *   correlationMeter.setMode(UiCorrelationMeter::Mode::Bar);
 *   correlationMeter.setShowNumericReadout(true);
 *   correlationMeter.setAveragingTime(50.0f);
 *
 *   // In audio callback:
 *   correlationMeter.pushSamples(leftChannel, rightChannel, numSamples);
 *   // OR if you calculate correlation yourself:
 *   correlationMeter.pushCorrelation(correlationValue);
 */

#include "bw_ui/foundation/UiTheme.h"
#include <bw_ui/rendering/JuceRenderer.h>
#include <bw_ui/Visualizers/CorrelationMeterModel.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>
#include <vector>
#include <cmath>

namespace bws::ui
{

/**
 * @class UiCorrelationMeter
 * @brief Professional stereo phase correlation meter with multiple display modes
 *
 * FEATURES:
 * - Bar mode: Horizontal/vertical bar moving from center
 * - Needle mode: Classic analog-style needle meter
 * - Histogram mode: Shows correlation distribution over time
 * - Color coding: Red (< -0.5) → Yellow (-0.5 to 0) → Green (0 to +1)
 * - Peak hold indicator (tracks minimum/worst case)
 * - Numeric readout
 * - "MONO SAFE" indicator when consistently > 0.5
 * - Thread-safe sample input
 */
class UiCorrelationMeter
    : public juce::Component
    , private juce::Timer
{
public:
    //==========================================================================
    // ENUMS
    //==========================================================================

    /**
     * Display mode for the correlation meter
     */
    enum class Mode
    {
        Bar,      ///< Horizontal bar moving left/right from center
        Needle,   ///< Classic analog-style needle meter
        Histogram ///< Shows distribution of correlation over time
    };

    //==========================================================================
    // CONSTRUCTION
    //==========================================================================

    /**
     * @brief Construct correlation meter with theme
     * @param theme Resolved UI theme for styling
     */
    explicit UiCorrelationMeter(const UiThemeResolved& theme)
        : themeRef(theme)
    {
        setOpaque(false);
        histogramBins.resize(histogramBinCount, 0.0f);
        model_.reset(histogramBinCount);
        startTimerHz(60);
    }

    ~UiCorrelationMeter() override { stopTimer(); }

    //==========================================================================
    // CONFIGURATION
    //==========================================================================

    /**
     * @brief Set display mode
     * @param m Bar, Needle, or Histogram mode
     */
    void setMode(Mode m)
    {
        if (mode != m)
        {
            mode = m;
            if (m == Mode::Histogram)
                clearHistogram();
            repaint();
        }
    }

    Mode getMode() const { return mode; }

    /**
     * @brief Set meter orientation
     * @param horizontal True for horizontal (default), false for vertical
     */
    void setOrientation(bool horizontal)
    {
        isHorizontal = horizontal;
        repaint();
    }

    /**
     * @brief Show/hide numeric readout
     * @param show True to display current correlation value
     */
    void setShowNumericReadout(bool show)
    {
        showNumericReadout = show;
        repaint();
    }

    /**
     * @brief Show/hide peak hold indicator
     * @param show True to display peak (minimum) correlation
     */
    void setShowPeakHold(bool show)
    {
        showPeakHold = show;
        repaint();
    }

    /**
     * @brief Show/hide mono safe indicator
     * @param show True to display "MONO SAFE" when correlation > 0.5
     */
    void setShowMonoSafeIndicator(bool show)
    {
        showMonoSafe = show;
        repaint();
    }

    /**
     * @brief Set correlation averaging time
     * @param ms Averaging window in milliseconds (default: 50ms)
     */
    void setAveragingTime(float ms) { averagingTimeMs = juce::jlimit(10.0f, 500.0f, ms); }

    /**
     * @brief Set peak hold time before decay
     * @param seconds Hold time in seconds (default: 2.0s)
     */
    void setPeakHoldTime(float seconds) { peakHoldSeconds = juce::jlimit(0.5f, 10.0f, seconds); }

    /**
     * @brief Set display smoothing factor
     * @param factor 0.0 = no smoothing, 1.0 = maximum smoothing (default: 0.85)
     */
    void setSmoothingFactor(float factor) { smoothingFactor = juce::jlimit(0.0f, 0.99f, factor); }

    /**
     * @brief Set UI scale factor
     * @param s Scale multiplier
     */
    void setScale(float s)
    {
        scale = s;
        repaint();
    }

    /**
     * @brief Set custom colors
     */
    void setColors(juce::Colour inPhase, juce::Colour neutral, juce::Colour outOfPhase)
    {
        colorInPhase = inPhase;
        colorNeutral = neutral;
        colorOutOfPhase = outOfPhase;
        useCustomColors = true;
        repaint();
    }

    /**
     * @brief Reset to theme colors
     */
    void useThemeColors()
    {
        useCustomColors = false;
        repaint();
    }

    //==========================================================================
    // RUNTIME INPUT (Thread-Safe)
    //==========================================================================

    /**
     * @brief Push raw stereo samples for correlation calculation
     * @param left Left channel samples
     * @param right Right channel samples
     * @param numSamples Number of samples
     *
     * Call this from your audio processBlock(). Thread-safe.
     * Correlation is calculated as: sum(L*R) / sqrt(sum(L²) * sum(R²))
     */
    void pushSamples(const float* left, const float* right, int numSamples)
    {
        if (numSamples <= 0 || left == nullptr || right == nullptr)
            return;

        // Calculate correlation for this block
        double sumLR = 0.0;
        double sumL2 = 0.0;
        double sumR2 = 0.0;

        for (int i = 0; i < numSamples; ++i)
        {
            const double l = static_cast<double>(left[i]);
            const double r = static_cast<double>(right[i]);
            sumLR += l * r;
            sumL2 += l * l;
            sumR2 += r * r;
        }

        // Avoid division by zero
        const double denominator = std::sqrt(sumL2 * sumR2);
        float correlation = 1.0f;

        if (denominator > 1e-10)
            correlation = static_cast<float>(sumLR / denominator);

        // Clamp to valid range
        correlation = juce::jlimit(-1.0f, 1.0f, correlation);

        // Store atomically
        currentCorrelation.store(correlation, std::memory_order_release);
        hasNewData.store(true, std::memory_order_release);
    }

    /**
     * @brief Push pre-calculated correlation value
     * @param correlation Correlation value from -1 to +1
     *
     * Use this if you calculate correlation yourself. Thread-safe.
     */
    void pushCorrelation(float correlation)
    {
        correlation = juce::jlimit(-1.0f, 1.0f, correlation);
        currentCorrelation.store(correlation, std::memory_order_release);
        hasNewData.store(true, std::memory_order_release);
    }

    /**
     * @brief Reset the meter state
     */
    void reset()
    {
        currentCorrelation.store(1.0f, std::memory_order_relaxed);
        model_.reset(histogramBinCount);
        smoothedCorrelation = 1.0f;
        peakCorrelation = 1.0f;
        displayMonoSafe_ = false;
        clearHistogram();
        repaint();
    }

    void setExternallyDriven(bool external)
    {
        externallyDriven_ = external;
        if (external)
        {
            stopTimer();
        }
        else
        {
            startTimerHz(60);
        }
    }

    [[nodiscard]] bool isExternallyDriven() const { return externallyDriven_; }

    [[nodiscard]] bool isSelfTimerRunning() const { return isTimerRunning(); }

    void advanceExternal(double dtSeconds) { tick(dtSeconds); }

    [[nodiscard]] float getSmoothedCorrelation() const { return smoothedCorrelation; }
    [[nodiscard]] float getPeakCorrelation() const { return peakCorrelation; }

    //==========================================================================
    // COMPONENT OVERRIDES
    //==========================================================================

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        if (bounds.isEmpty())
            return;

        rendering::JuceRenderer renderer(g);

        // Clear background
        renderer.setColour(themeRef.colors.bg0.getARGB());
        renderer.fillRect(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight());

        // Draw based on mode
        switch (mode)
        {
        case Mode::Bar:
            drawBarMode(renderer, bounds);
            break;
        case Mode::Needle:
            drawNeedleMode(renderer, bounds);
            break;
        case Mode::Histogram:
            drawHistogramMode(renderer, bounds);
            break;
        }
    }

    void resized() override
    {
        // Layout calculated in paint()
    }

private:
    //==========================================================================
    // TIMER CALLBACK
    //==========================================================================

    void timerCallback() override { tick(1.0 / 60.0); }

    void tick(double dtSeconds)
    {
        const auto cfg = makeConfig();
        const bool newData = hasNewData.exchange(false, std::memory_order_acquire);
        const float raw = currentCorrelation.load(std::memory_order_relaxed);
        const float prevPeak = model_.peak;

        model_.advance(raw, newData, static_cast<float>(dtSeconds), cfg);
        syncDisplayFromModel(cfg);

        if (newData || std::abs(model_.peak - prevPeak) > 1.0e-6f)
        {
            repaint();
        }
    }

    CorrelationMeterModel::Config makeConfig() const
    {
        CorrelationMeterModel::Config cfg;
        cfg.smoothingTauMs = (smoothingFactor > 0.0f) ? -1000.0f * (1.0f / 30.0f) / std::log(smoothingFactor) : 0.0f;
        cfg.peakReleaseTauMs = -1000.0f * (1.0f / 60.0f) / std::log(1.0f - 0.05f);
        cfg.peakHoldSeconds = peakHoldSeconds;
        cfg.monoSafeSeconds = 1.0f;
        cfg.binCount = histogramBinCount;
        cfg.histogramEnabled = (mode == Mode::Histogram);
        return cfg;
    }

    void syncDisplayFromModel(const CorrelationMeterModel::Config& cfg)
    {
        smoothedCorrelation = model_.smoothed;
        peakCorrelation = model_.peak;
        displayMonoSafe_ = model_.monoSafe(cfg);
        if (mode == Mode::Histogram)
        {
            histogramBins = model_.histogram;
        }
    }

    //==========================================================================
    // DRAWING - BAR MODE
    //==========================================================================

    void drawBarMode(rendering::IRenderer& r, juce::Rectangle<float> bounds)
    {
        const float padding = 4.0f * scale;
        const float cornerRadius = 4.0f * scale;

        // Reserve space for readout if needed
        auto readoutBounds = juce::Rectangle<float>();
        if (showNumericReadout)
        {
            if (isHorizontal)
            {
                const float readoutHeight = 18.0f * scale;
                readoutBounds = bounds.removeFromBottom(readoutHeight);
                bounds.removeFromBottom(padding);
            }
            else
            {
                const float readoutWidth = 45.0f * scale;
                readoutBounds = bounds.removeFromRight(readoutWidth);
                bounds.removeFromRight(padding);
            }
        }

        // Background
        r.setColour(themeRef.colors.bg1.getARGB());
        r.fillRoundedRect(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), cornerRadius);

        // Border
        r.setColour(themeRef.colors.outline0.getARGB());
        r.drawRoundedRect(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), cornerRadius,
                          themeRef.surfaces.strokeThin);

        // Inner track
        auto trackBounds = bounds.reduced(padding);

        // Draw scale markings
        drawScaleMarkings(r, trackBounds);

        // Calculate bar position (0 = center, -1 = left, +1 = right)
        const float normalized = (smoothedCorrelation + 1.0f) * 0.5f;

        // Draw bar from center
        auto barBounds = trackBounds.reduced(bws::tokens::shared::spacing::XXS * scale);
        const float centerX = barBounds.getCentreX();
        const float centerY = barBounds.getCentreY();

        juce::Rectangle<float> bar;
        if (isHorizontal)
        {
            const float barWidth = (normalized - 0.5f) * barBounds.getWidth();
            if (barWidth >= 0)
                bar = juce::Rectangle<float>(centerX, barBounds.getY(), barWidth, barBounds.getHeight());
            else
                bar = juce::Rectangle<float>(centerX + barWidth, barBounds.getY(), -barWidth, barBounds.getHeight());
        }
        else
        {
            const float barHeight = (normalized - 0.5f) * barBounds.getHeight();
            if (barHeight >= 0)
                bar = juce::Rectangle<float>(barBounds.getX(), centerY - barHeight, barBounds.getWidth(), barHeight);
            else
                bar = juce::Rectangle<float>(barBounds.getX(), centerY, barBounds.getWidth(), -barHeight);
        }

        // Bar color based on correlation
        r.setColour(getCorrelationColor(smoothedCorrelation).getARGB());
        r.fillRoundedRect(bar.getX(), bar.getY(), bar.getWidth(), bar.getHeight(), 2.0f * scale);

        // Center line
        r.setColour(
            themeRef.colors.text1.withAlpha(bws::tokens::shared::opacity::correlation_meter::CENTER_LINE).getARGB());
        if (isHorizontal)
            r.drawLine(centerX, trackBounds.getY(), centerX, trackBounds.getBottom(), 1.0f);
        else
            r.drawLine(trackBounds.getX(), centerY, trackBounds.getRight(), centerY, 1.0f);

        // Peak hold indicator
        if (showPeakHold)
        {
            const float peakNorm = (peakCorrelation + 1.0f) * 0.5f;
            r.setColour(getCorrelationColor(peakCorrelation)
                            .withAlpha(bws::tokens::shared::opacity::correlation_meter::PEAK_HOLD)
                            .getARGB());

            if (isHorizontal)
            {
                float peakX = barBounds.getX() + peakNorm * barBounds.getWidth();
                r.fillRect(peakX - 1.5f * scale, barBounds.getY(), 3.0f * scale, barBounds.getHeight());
            }
            else
            {
                float peakY = barBounds.getBottom() - peakNorm * barBounds.getHeight();
                r.fillRect(barBounds.getX(), peakY - 1.5f * scale, barBounds.getWidth(), 3.0f * scale);
            }
        }

        // Numeric readout
        if (showNumericReadout)
            drawNumericReadout(r, readoutBounds);

        // Mono safe indicator
        if (showMonoSafe)
            drawMonoSafeIndicator(r, bounds);
    }

    //==========================================================================
    // DRAWING - NEEDLE MODE
    //==========================================================================

    void drawNeedleMode(rendering::IRenderer& r, juce::Rectangle<float> bounds)
    {
        const float padding = 8.0f * scale;

        // Reserve space for readout
        auto readoutBounds = juce::Rectangle<float>();
        if (showNumericReadout)
        {
            const float readoutHeight = 20.0f * scale;
            readoutBounds = bounds.removeFromBottom(readoutHeight);
            bounds.removeFromBottom(padding / 2);
        }

        // Background arc area
        r.setColour(themeRef.colors.bg1.getARGB());
        r.fillRoundedRect(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(),
                          themeRef.surfaces.radiusMd);

        r.setColour(themeRef.colors.outline0.getARGB());
        r.drawRoundedRect(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(),
                          themeRef.surfaces.radiusMd, themeRef.surfaces.strokeThin);

        // Calculate needle geometry
        auto meterBounds = bounds.reduced(padding);
        const float centerX = meterBounds.getCentreX();
        const float centerY = meterBounds.getBottom() - padding;
        const float radius = juce::jmin(meterBounds.getWidth() * 0.45f, meterBounds.getHeight() * 0.8f);

        // Draw arc background with color gradient
        const float arcStartAngle = juce::MathConstants<float>::pi * 0.8f;
        const float arcEndAngle = juce::MathConstants<float>::pi * 0.2f;
        const float arcThickness = 8.0f * scale;

        drawColoredArc(r, centerX, centerY, radius, arcStartAngle, arcEndAngle, arcThickness);

        // Draw scale ticks and labels
        drawNeedleScale(r, centerX, centerY, radius, arcStartAngle, arcEndAngle);

        // Draw needle
        const float needleAngle = juce::jmap(smoothedCorrelation, -1.0f, 1.0f, arcStartAngle, arcEndAngle);
        const float needleLength = radius - 10.0f * scale;

        // Needle shadow
        const float shadowOffset = 2.0f * scale;
        r.setColour(
            juce::Colours::black.withAlpha(bws::tokens::shared::opacity::correlation_meter::NEEDLE_SHADOW).getARGB());
        r.drawLine(centerX + shadowOffset, centerY + shadowOffset,
                   centerX + shadowOffset + std::cos(needleAngle) * needleLength,
                   centerY + shadowOffset - std::sin(needleAngle) * needleLength, 2.5f * scale);

        // Needle
        r.setColour(getCorrelationColor(smoothedCorrelation).getARGB());
        r.drawLine(centerX, centerY, centerX + std::cos(needleAngle) * needleLength,
                   centerY - std::sin(needleAngle) * needleLength, 2.0f * scale);

        // Needle pivot
        r.setColour(themeRef.colors.text0.getARGB());
        r.fillEllipse(centerX - 4.0f * scale, centerY - 4.0f * scale, 8.0f * scale, 8.0f * scale);

        // Peak hold needle (thinner, semi-transparent)
        if (showPeakHold)
        {
            const float peakAngle = juce::jmap(peakCorrelation, -1.0f, 1.0f, arcStartAngle, arcEndAngle);
            r.setColour(getCorrelationColor(peakCorrelation)
                            .withAlpha(bws::tokens::shared::opacity::correlation_meter::PEAK_NEEDLE)
                            .getARGB());
            r.drawLine(centerX, centerY, centerX + std::cos(peakAngle) * needleLength * 0.8f,
                       centerY - std::sin(peakAngle) * needleLength * 0.8f, 1.5f * scale);
        }

        // Numeric readout
        if (showNumericReadout)
            drawNumericReadout(r, readoutBounds);

        // Mono safe indicator
        if (showMonoSafe)
            drawMonoSafeIndicator(r, bounds);
    }

    //==========================================================================
    // DRAWING - HISTOGRAM MODE
    //==========================================================================

    void drawHistogramMode(rendering::IRenderer& r, juce::Rectangle<float> bounds)
    {
        const float padding = 4.0f * scale;
        const float cornerRadius = 4.0f * scale;

        // Reserve space for readout
        auto readoutBounds = juce::Rectangle<float>();
        if (showNumericReadout)
        {
            const float readoutHeight = 18.0f * scale;
            readoutBounds = bounds.removeFromBottom(readoutHeight);
            bounds.removeFromBottom(padding);
        }

        // Background
        r.setColour(themeRef.colors.bg1.getARGB());
        r.fillRoundedRect(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), cornerRadius);

        r.setColour(themeRef.colors.outline0.getARGB());
        r.drawRoundedRect(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), cornerRadius,
                          themeRef.surfaces.strokeThin);

        // Histogram area
        auto histBounds = bounds.reduced(padding);

        // Draw scale markings
        drawScaleMarkings(r, histBounds);

        // Draw histogram bars
        const float barWidth = histBounds.getWidth() / static_cast<float>(histogramBinCount);
        const float maxHeight = histBounds.getHeight() - 4.0f * scale;

        for (int i = 0; i < histogramBinCount; ++i)
        {
            float binValue = histogramBins[static_cast<size_t>(i)];
            if (binValue < 0.01f)
                continue;

            float correlation = binToCorrelation(i);
            float barHeight = binValue * maxHeight;
            float x = histBounds.getX() + static_cast<float>(i) * barWidth;

            r.setColour(getCorrelationColor(correlation)
                            .withAlpha(bws::tokens::shared::opacity::correlation_meter::HIST_BAR)
                            .getARGB());
            r.fillRect(x, histBounds.getBottom() - barHeight, barWidth - 1.0f * scale, barHeight);
        }

        // Current position indicator
        int currentBin = correlationToBin(smoothedCorrelation);
        float indicatorX = histBounds.getX() + (static_cast<float>(currentBin) + 0.5f) * barWidth;
        r.setColour(themeRef.colors.text0.getARGB());
        r.drawLine(indicatorX, histBounds.getY(), indicatorX, histBounds.getBottom(), 2.0f * scale);

        // Center line
        r.setColour(themeRef.colors.text1.withAlpha(bws::tokens::shared::opacity::correlation_meter::HIST_CENTER_LINE)
                        .getARGB());
        r.drawLine(histBounds.getCentreX(), histBounds.getY(), histBounds.getCentreX(), histBounds.getBottom(), 1.0f);

        // Numeric readout
        if (showNumericReadout)
            drawNumericReadout(r, readoutBounds);

        // Mono safe indicator
        if (showMonoSafe)
            drawMonoSafeIndicator(r, bounds);
    }

    //==========================================================================
    // DRAWING HELPERS
    //==========================================================================

    void drawScaleMarkings(rendering::IRenderer& r, juce::Rectangle<float> bounds)
    {
        r.setColour(
            themeRef.colors.text1.withAlpha(bws::tokens::shared::opacity::correlation_meter::SCALE_LABEL).getARGB());
        r.setFont(9.0f * scale, rendering::FontWeight::Regular);

        if (isHorizontal)
        {
            // Labels at -1, 0, +1
            r.drawText("-1", bounds.getX(), bounds.getY() - 12.0f * scale, 20.0f * scale, 12.0f * scale,
                       rendering::Justification::Left);
            r.drawText("0", bounds.getCentreX() - 10.0f * scale, bounds.getY() - 12.0f * scale, 20.0f * scale,
                       12.0f * scale, rendering::Justification::Centre);
            r.drawText("+1", bounds.getRight() - 20.0f * scale, bounds.getY() - 12.0f * scale, 20.0f * scale,
                       12.0f * scale, rendering::Justification::Right);

            // L/R labels
            r.setColour(themeRef.colors.text1.withAlpha(bws::tokens::shared::opacity::correlation_meter::AXIS_LABEL_DIM)
                            .getARGB());
            r.drawText("L", bounds.getX(), bounds.getBottom() + 2.0f * scale, 20.0f * scale, 12.0f * scale,
                       rendering::Justification::Left);
            r.drawText("R", bounds.getRight() - 20.0f * scale, bounds.getBottom() + 2.0f * scale, 20.0f * scale,
                       12.0f * scale, rendering::Justification::Right);
        }
    }

    void drawNeedleScale(rendering::IRenderer& r, float cx, float cy, float radius, float startAngle, float endAngle)
    {
        r.setColour(themeRef.colors.text1.getARGB());
        r.setFont(10.0f * scale, rendering::FontWeight::Regular);

        // Draw tick marks and labels at -1, -0.5, 0, +0.5, +1
        const float tickLength = 8.0f * scale;
        const float labelRadius = radius + 12.0f * scale;

        float values[] = {-1.0f, -0.5f, 0.0f, 0.5f, 1.0f};
        const char* labels[] = {"-1", "-.5", "0", "+.5", "+1"};

        for (int i = 0; i < 5; ++i)
        {
            float angle = juce::jmap(values[i], -1.0f, 1.0f, startAngle, endAngle);

            // Tick
            float x1 = cx + std::cos(angle) * (radius - tickLength);
            float y1 = cy - std::sin(angle) * (radius - tickLength);
            float x2 = cx + std::cos(angle) * radius;
            float y2 = cy - std::sin(angle) * radius;

            r.setColour(
                themeRef.colors.text1.withAlpha(bws::tokens::shared::opacity::correlation_meter::SCALE_TICK).getARGB());
            r.drawLine(x1, y1, x2, y2, 1.5f * scale);

            // Label
            float lx = cx + std::cos(angle) * labelRadius;
            float ly = cy - std::sin(angle) * labelRadius;

            r.setColour(themeRef.colors.text1.getARGB());
            r.drawText(labels[i], lx - 15.0f * scale, ly - 6.0f * scale, 30.0f * scale, 12.0f * scale,
                       rendering::Justification::Centre);
        }
    }

    void drawColoredArc(rendering::IRenderer& r, float cx, float cy, float radius, float startAngle, float endAngle,
                        float thickness)
    {
        const int segments = 50;
        const float segmentAngle = (endAngle - startAngle) / segments;

        for (int i = 0; i < segments; ++i)
        {
            float angle1 = startAngle + static_cast<float>(i) * segmentAngle;
            float angle2 = angle1 + segmentAngle;
            float midCorrelation = juce::jmap(static_cast<float>(i) / segments, 0.0f, 1.0f, -1.0f, 1.0f);

            r.setColour(getCorrelationColor(midCorrelation)
                            .withAlpha(bws::tokens::shared::opacity::correlation_meter::ARC_SEGMENT)
                            .getARGB());
            r.beginPath();
            r.arcTo(cx, cy, radius, -angle1, -angle2);
            r.strokePath(thickness);
        }
    }

    void drawNumericReadout(rendering::IRenderer& r, juce::Rectangle<float> bounds)
    {
        // Background
        r.setColour(themeRef.colors.bg2.getARGB());
        r.fillRoundedRect(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), 3.0f * scale);

        // Value text
        juce::String valueText;
        if (smoothedCorrelation >= 0.0f)
            valueText = "+" + juce::String(smoothedCorrelation, 2);
        else
            valueText = juce::String(smoothedCorrelation, 2);

        r.setColour(getCorrelationColor(smoothedCorrelation).getARGB());
        r.setFont(12.0f * scale, rendering::FontWeight::Bold);
        r.drawText(valueText.toRawUTF8(), bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(),
                   rendering::Justification::Centre);
    }

    void drawMonoSafeIndicator(rendering::IRenderer& r, juce::Rectangle<float> bounds)
    {
        // Show "MONO SAFE" when correlation has held above 0.5 long enough
        const bool isMonoSafe = displayMonoSafe_;

        const float indicatorWidth = 70.0f * scale;
        const float indicatorHeight = 14.0f * scale;
        auto indicatorBounds = juce::Rectangle<float>(bounds.getRight() - indicatorWidth - 4.0f * scale,
                                                      bounds.getY() + 4.0f * scale, indicatorWidth, indicatorHeight);

        juce::Colour bgColor =
            isMonoSafe ? themeRef.colors.ok.withAlpha(bws::tokens::shared::opacity::correlation_meter::BADGE_BG_ACTIVE)
                       : themeRef.colors.bg2.withAlpha(bws::tokens::shared::opacity::correlation_meter::BADGE_BG_IDLE);
        juce::Colour textColor =
            isMonoSafe
                ? themeRef.colors.ok
                : themeRef.colors.text1.withAlpha(bws::tokens::shared::opacity::correlation_meter::BADGE_TEXT_IDLE);

        r.setColour(bgColor.getARGB());
        r.fillRoundedRect(indicatorBounds.getX(), indicatorBounds.getY(), indicatorBounds.getWidth(),
                          indicatorBounds.getHeight(), 2.0f * scale);

        r.setColour(textColor.getARGB());
        r.setFont(8.0f * scale, rendering::FontWeight::Bold);
        r.drawText("MONO SAFE", indicatorBounds.getX(), indicatorBounds.getY(), indicatorBounds.getWidth(),
                   indicatorBounds.getHeight(), rendering::Justification::Centre);
    }

    //==========================================================================
    // COLOR HELPERS
    //==========================================================================

    juce::Colour getCorrelationColor(float correlation) const
    {
        juce::Colour red = useCustomColors ? colorOutOfPhase : themeRef.colors.danger;
        juce::Colour yellow = useCustomColors ? colorNeutral : themeRef.colors.warn;
        juce::Colour green = useCustomColors ? colorInPhase : themeRef.colors.ok;

        if (correlation < -0.5f)
        {
            // Red zone: -1 to -0.5
            return red;
        }
        else if (correlation < 0.0f)
        {
            // Yellow zone: -0.5 to 0, interpolate red→yellow
            float t = (correlation + 0.5f) / 0.5f;
            return red.interpolatedWith(yellow, t);
        }
        else if (correlation < 0.5f)
        {
            // Transition zone: 0 to 0.5, interpolate yellow→green
            float t = correlation / 0.5f;
            return yellow.interpolatedWith(green, t);
        }
        else
        {
            // Green zone: 0.5 to 1
            return green;
        }
    }

    //==========================================================================
    // HISTOGRAM HELPERS
    //==========================================================================

    int correlationToBin(float correlation) const
    {
        float normalized = (correlation + 1.0f) * 0.5f; // 0 to 1
        int bin = static_cast<int>(normalized * (histogramBinCount - 1));
        return juce::jlimit(0, histogramBinCount - 1, bin);
    }

    float binToCorrelation(int bin) const
    {
        float normalized = static_cast<float>(bin) / (histogramBinCount - 1);
        return normalized * 2.0f - 1.0f; // -1 to +1
    }

    void clearHistogram() { std::fill(histogramBins.begin(), histogramBins.end(), 0.0f); }

    //==========================================================================
    // MEMBER VARIABLES
    //==========================================================================

    // Theme reference
    const UiThemeResolved& themeRef;

    // Display configuration
    Mode mode {Mode::Bar};
    bool isHorizontal {true};
    bool showNumericReadout {true};
    bool showPeakHold {true};
    bool showMonoSafe {true};
    float scale {1.0f};

    // Timing configuration
    float averagingTimeMs {50.0f};
    float peakHoldSeconds {2.0f};
    float smoothingFactor {0.85f};

    // Custom colors
    bool useCustomColors {false};
    juce::Colour colorInPhase {juce::Colours::green};
    juce::Colour colorNeutral {juce::Colours::yellow};
    juce::Colour colorOutOfPhase {juce::Colours::red};

    // Runtime state (thread-safe input)
    std::atomic<float> currentCorrelation {1.0f};
    std::atomic<bool> hasNewData {false};

    // Ballistics
    CorrelationMeterModel model_;
    bool externallyDriven_ {false};

    // Display state (synced from model_ each tick; read by paint)
    float smoothedCorrelation {1.0f};
    float peakCorrelation {1.0f};
    bool displayMonoSafe_ {false};

    // Histogram data
    static constexpr int histogramBinCount = 41; // -1 to +1 in 0.05 steps
    std::vector<float> histogramBins;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UiCorrelationMeter)
};

} // namespace bws::ui

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * =============================================================================
 * BarometerEditor.h - Barometer Utility Plugin Editor
 * =============================================================================
 *
 * Bellweather Studios - gain and metering utility plugin
 *
 * Layout:
 *   - Base size: 408 × 489 px (token-driven, fixed aspect ratio)
 *   - Hero Gain Dial: 200px
 *   - Balance Slider: 120px wide × 24px tall (continuous control)
 * - uttons: four 24px controls (L, link, R, mono)
 *   - A/B Toggle: above output meter for state comparison
 *   - Meters: 19px wide stereo input/output strips flanking the dial
 *   - Preset Dropdown: Below title, centered
 *
 * Dimensions resolve through Barometer design tokens and ResizableEditor scaling.
 *
 * =============================================================================
 */

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <bw_ui/foundation/GoldenRatioConstants.h>
#include "bw_ui/Components/UiArcKnob.h"
#include "bw_ui/Components/UiResizeCorner.h"
#include "bw_ui/foundation/UiTheme.h"
#include <bw_ui/adapters/UiThemeKernelAdapter.h> // bws::ui::kernel::ThemeSnapshot for makeEditorKernelTheme
#include "bw_ui/weather/controls/WeatherComponents.h"
#include "bw_ui/weather/layout/WeatherLayout.h"
#include "bw_ui/weather/controls/WeatherTabGroup.h"

// Weather Instrument Enhancements
#include "bw_ui/weather/containers/ResizableEditor.h"
#include "bw_ui/weather/controls/WeatherKnob.h"
#include "bw_ui/weather/displays/InteractiveDisplay.h"

#include "BarometerProcessor.h"
#include <bw_ui/generated/BwTokens.h>
#include <bw_ui/Visualizers/UiCorrelationMeter.h>
#include <bw_ui/adapters/JuceTimerClock.h>
#include <bw_dsp_metering/MeterBallistics.h>
#include "FadeModel.h"
#include <bw_ui/weather/displays/AboutOverlay.h>
#include <bw_ui/preset_system/WeatherPresetManager.h>
#include <array>
#include <cmath>

namespace bws::barometer::toy
{
class BarometerToyOverlay;
}

namespace bws::barometer
{

// =============================================================================
// Type Aliases for Weather Components
// =============================================================================
// Shared weather components keep Barometer's UI aligned with the library theme.
// across the Weather Instrument suite (BwsBarometer,, etc.)
// =============================================================================

using ABToggleButton = bws::weather::ABToggle;
using ScrollablePresetComboBox = bws::weather::ScrollablePresetBox;
using MouseWheelSlider = bws::weather::MouseWheelSlider;
using MouseWheelToggleButton = bws::weather::MouseWheelToggle;
using ValueDisplayPopup = bws::weather::ValuePopup;

// Weather Instrument Enhancements
using WeatherKnob = bws::weather::WeatherKnob;
using ResizableEditorBase = bws::weather::ResizableEditor;
using SizePreset = bws::weather::SizePreset;

// =============================================================================
// Dial label configuration
// =============================================================================
// A_VisualRange:  -24 to +24 on dial, full range in readout
// B_HonestRange:  -96 to +24 on dial (asymmetric)
// C_MinimalIcons: -∞, 0, +24 only
// =============================================================================
enum class DialLabelConfig
{
    A_VisualRange,
    B_HonestRange,
    C_MinimalIcons
};

// Active dial-label configuration:
static constexpr DialLabelConfig kDialLabelConfig = DialLabelConfig::A_VisualRange;

// =============================================================================
// Spacing variants - top-section compression that redistributes space toward
// the bottom section. Define exactly one:
// =============================================================================
#define SPACING_CURRENT // Baseline - current spacing
// #define SPACING_VARIANT_A  // Mild compression (-20px from top, +9px to bottom)
// #define SPACING_VARIANT_B  // Medium compression (-30px from top, +15px to bottom)
// #define SPACING_VARIANT_C  // Aggressive compression (-40px from top, +21px to bottom)
// #define SPACING_VARIANT_D  // Maximum compression (-50px from top, +27px to bottom)
// =============================================================================

// =============================================================================
// LAYOUT MODE: Toggle between slider labels and icon-only minimal mode
// =============================================================================
// Set to 1 for icon-only mode (no text labels, tooltips only)
// Set to 0 for standard mode (text labels above controls)
// =============================================================================
#define USE_MINIMAL_ICONS 0

// =============================================================================
// FadingTextButton - alpha-fade on hover for non-intrusive UI elements
// =============================================================================
class FadingTextButton : public juce::TextButton
{
public:
    FadingTextButton(const juce::String& buttonText = {})
        : juce::TextButton(buttonText)
    {
        setAlpha(0.0f); // Start invisible
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void mouseEnter(const juce::MouseEvent& e) override
    {
        juce::TextButton::mouseEnter(e);
        fade_.fadingIn = true;
    }

    void mouseExit(const juce::MouseEvent& e) override
    {
        juce::TextButton::mouseExit(e);
        fade_.fadingIn = false;
    }

    void setFadeInDurationMs(int durationMs) { fadeInDurationMs_ = durationMs; }
    void setFadeOutDurationMs(int durationMs) { fadeOutDurationMs_ = durationMs; }

    void advanceFade(double dtSeconds)
    {
        const FadeModel::Config cfg {
            .fadeInDurationMs = static_cast<float>(fadeInDurationMs_),
            .fadeOutDurationMs = static_cast<float>(fadeOutDurationMs_),
            .fadeInMultiplier = bws::tokens::barometer::animation::FADE_IN_MULTIPLIER,
            .fadeOutMultiplier = bws::tokens::barometer::animation::FADE_OUT_MULTIPLIER,
        };
        const float before = fade_.alpha;
        const bool active = fade_.advance(static_cast<float>(dtSeconds), cfg);
        if (active || std::abs(fade_.alpha - before) > 1.0e-6f)
        {
            setAlpha(fade_.alpha);
            repaint();
        }
    }

private:
    FadeModel fade_;
    int fadeInDurationMs_ = bws::tokens::barometer::animation::FADE_IN_DURATION_MS;
    int fadeOutDurationMs_ = bws::tokens::barometer::animation::FADE_OUT_DURATION_MS;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FadingTextButton)
};

class BarometerEditor
    : public ::bws::weather::ResizableEditor
    , public juce::SettableTooltipClient
    , public ::bws::weather::WeatherPresetManager::Listener
{
public:
    explicit BarometerEditor(BarometerProcessor& processor);
    ~BarometerEditor() override;


    // WeatherPresetManager::Listener - shared dialog + any manager-side
    // mutation will notify us; we refresh our custom combo in response.
    void weatherPresetChanged() override;
    void weatherPresetListChanged() override;

protected:
    juce::Rectangle<int> getBasePluginSize() const override { return {0, 0, kPluginWidth, kPluginBaseHeight}; }
    juce::String getPluginName() const override;
    void layoutComponents() override;
    void paintContent(juce::Graphics& g) override;
    void onThemeChanged() override;

private:
    // Per-tick meter updates, driven by uiClock_.
    void onUiTick(double dtSeconds);

    // Builds the kernel theme snapshot consumed by the paint methods and ctor.
    static bws::ui::kernel::ThemeSnapshot makeEditorKernelTheme(const bws::ui::UiThemeResolved& theme);

    // Construction phases, called in order from the constructor body.
    void setupControls();
    void setupMetersAndState();
    void setupPreferencesAndOverlay();

    BarometerProcessor& processor_;

    // ==========================================================================
    // PREFERENCE PERSISTENCE (non-scale preferences)
    // ==========================================================================
    juce::PropertiesFile* getPropertiesFile();

    // ==========================================================================
    // Controls
    // ==========================================================================

    // Hero Gain Dial (260px) - Arc-style knob with centered digital readout
    std::unique_ptr<bws::ui::UiArcKnob> gainKnob_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment_;

    // Balance Slider (120px × 24px) - Horizontal slider for L/R balance
    std::unique_ptr<MouseWheelSlider> balanceSlider_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> balanceAttachment_;

    // Per-Channel ontrols (4-button group: L, Link, R, M)
    std::unique_ptr<MouseWheelToggleButton> phaseLButton_;
    std::unique_ptr<MouseWheelToggleButton> phaseLinkButton_;
    std::unique_ptr<MouseWheelToggleButton> phaseRButton_;
    std::unique_ptr<MouseWheelToggleButton> monoButton_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> phaseLAttachment_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> phaseRAttachment_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> phaseLinkAttachment_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> monoAttachment_;

    // A/B Compare Toggle Button (shows both states: A | B)
    std::unique_ptr<ABToggleButton> abToggle_;

    // DC Filter Toggle Button (5Hz high-pass to remove DC offset)
    std::unique_ptr<MouseWheelToggleButton> dcFilterButton_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> dcFilterAttachment_;

    // Preset Dropdown
    std::unique_ptr<ScrollablePresetComboBox> presetCombo_;

    // Balance/Width Tab Group (switches slider behavior)
    std::unique_ptr<bws::weather::WeatherTabGroup> balanceWidthTabs_;
    int balanceWidthMode_ = 0; // 0 = Balance, 1 = Width

    // Width Slider (shares same visual space as balance slider)
    std::unique_ptr<MouseWheelSlider> widthSlider_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> widthAttachment_;

    // Settings Gear Button (top-right corner)
    std::unique_ptr<juce::TextButton> settingsButton_;

    // Tooltip window (required for tooltips to display)
    std::unique_ptr<juce::TooltipWindow> tooltipWindow_;
    bool tooltipsEnabled_ = true;

    // Value display popup (hover value display)
    std::unique_ptr<ValueDisplayPopup> valueDisplayPopup_;

    // Tooltip toggle helpers
    void setTooltipsEnabled(bool enabled);
    void saveTooltipPreference();


    // About overlay (shared bw_ui component)
    std::unique_ptr<bws::weather::AboutOverlay> aboutOverlay_;
    std::unique_ptr<bws::barometer::toy::BarometerToyOverlay> toyOverlay_;
    void loadTooltipPreference();

    // ==========================================================================
    // GOLDEN RATIO LAYOUT CONSTANTS
    // ==========================================================================
    // Using core golden ratio from bws::design namespace
    // See: modules/bw_ui/include/bw_ui/foundation/GoldenRatioConstants.h
    // ==========================================================================
    static constexpr float PHI = bws::design::kPhi;

    // Hero dial size
    static constexpr int kHeroDialSize = bws::tokens::barometer::layout::HERO_DIAL_SIZE;

    // Control sizes (fixed across all variants)
    static constexpr int kPhaseButtonSize = bws::tokens::barometer::layout::PHASE_BUTTON_SIZE;
    static constexpr int kPhaseButtonGap = bws::tokens::barometer::spacing::PHASE_BUTTON_GAP;
    static constexpr int kPhaseGroupWidth = kPhaseButtonSize * 4 + kPhaseButtonGap * 3; // 120px total (L, Link, R, M)

    // Balance slider dimensions (continuous control needs precision)
    static constexpr int kBalanceSliderWidth = bws::tokens::barometer::layout::BALANCE_SLIDER_WIDTH;
    static constexpr int kBalanceSliderHeight = bws::tokens::barometer::layout::BALANCE_SLIDER_HEIGHT;

    // ==========================================================================
    // SPACING VARIANT DEFINITIONS
    // ==========================================================================
    // Each variant compresses the top section to give breathing room to bottom.
    // Variant B is recommended starting point - balanced compression.
    // ==========================================================================

#if defined(SPACING_CURRENT)
    // -------------------------------------------------------------------------
    // BASELINE: Golden ratio spacing with preset-to-dial gap
    // -------------------------------------------------------------------------
    // Golden ratio base unit
    static constexpr int kSpacingUnit = bws::tokens::barometer::spacing::UNIT;
    static constexpr int kSpacingSmall = static_cast<int>(kSpacingUnit / PHI); // ~15px (PHI-derived)

    static constexpr int kTopMargin = bws::tokens::barometer::spacing::TOP_MARGIN;
    static constexpr int kTitleHeight = bws::tokens::barometer::spacing::TITLE_HEIGHT;
    static constexpr int kTitleToPresetGap = bws::tokens::barometer::spacing::TITLE_TO_PRESET_GAP;
    static constexpr int kPresetToDialGap = bws::tokens::barometer::spacing::PRESET_TO_DIAL_GAP;
    // Legacy readout constants (kept for height calculation compatibility)
    static constexpr int kTitleToReadoutGap = kTitleToPresetGap;                      // Maps to title→preset
    static constexpr int kReadoutHeight = 22;                                         // Preset dropdown height
    static constexpr int kReadoutToDialGap = kPresetToDialGap;                        // Maps to preset→dial
    static constexpr int kTitleToDialGap = kTitleToPresetGap + 22 + kPresetToDialGap; // 15 + 22 + 24 = 61px total
    static constexpr int kDialToControlsGap = bws::tokens::barometer::spacing::DIAL_TO_CONTROLS_GAP;
    static constexpr int kControlsToBrandGap = bws::tokens::barometer::spacing::CONTROLS_TO_BRAND_GAP;
    static constexpr int kBottomMargin = bws::tokens::barometer::spacing::BOTTOM_MARGIN;

#elif defined(SPACING_VARIANT_A)
    // -------------------------------------------------------------------------
    // VARIANT A: Mild Compression (-20px from top, +9px to bottom)
    // -------------------------------------------------------------------------
    static constexpr int kTopMargin = 28;
    static constexpr int kTitleHeight = 20;
    static constexpr int kTitleToReadoutGap = 18;
    static constexpr int kReadoutHeight = 28;
    static constexpr int kReadoutToDialGap = 11;
    static constexpr int kDialToControlsGap = 24;
    static constexpr int kControlsToBrandGap = 20;
    static constexpr int kBottomMargin = 16;

#elif defined(SPACING_VARIANT_B)
    // -------------------------------------------------------------------------
    // VARIANT B: Medium Compression (-30px from top, +15px to bottom)
    // -------------------------------------------------------------------------
    static constexpr int kTopMargin = 24;
    static constexpr int kTitleHeight = 20;
    static constexpr int kTitleToReadoutGap = 16;
    static constexpr int kReadoutHeight = 25;
    static constexpr int kReadoutToDialGap = 10;
    static constexpr int kDialToControlsGap = 30;
    static constexpr int kControlsToBrandGap = 20;
    static constexpr int kBottomMargin = 16;

#elif defined(SPACING_VARIANT_C)
    // -------------------------------------------------------------------------
    // VARIANT C: Aggressive Compression (-40px from top, +21px to bottom)
    // -------------------------------------------------------------------------
    static constexpr int kTopMargin = 20;
    static constexpr int kTitleHeight = 20;
    static constexpr int kTitleToReadoutGap = 12;
    static constexpr int kReadoutHeight = 24;
    static constexpr int kReadoutToDialGap = 8;
    static constexpr int kDialToControlsGap = 36;
    static constexpr int kControlsToBrandGap = 20;
    static constexpr int kBottomMargin = 16;

#elif defined(SPACING_VARIANT_D)
    // -------------------------------------------------------------------------
    // VARIANT D: Maximum Compression (-50px from top, +27px to bottom)
    // -------------------------------------------------------------------------
    static constexpr int kTopMargin = 18;
    static constexpr int kTitleHeight = 20;
    static constexpr int kTitleToReadoutGap = 10;
    static constexpr int kReadoutHeight = 22;
    static constexpr int kReadoutToDialGap = 6;
    static constexpr int kDialToControlsGap = 42;
    static constexpr int kControlsToBrandGap = 20;
    static constexpr int kBottomMargin = 16;

#else
    // -------------------------------------------------------------------------
    // FALLBACK: Same as SPACING_CURRENT
    // -------------------------------------------------------------------------
    static constexpr int kTopMargin = 38;
    static constexpr int kTitleHeight = 20;
    static constexpr int kTitleToReadoutGap = 24;
    static constexpr int kReadoutHeight = 28;
    static constexpr int kReadoutToDialGap = 15;
    static constexpr int kDialToControlsGap = 15;
    static constexpr int kControlsToBrandGap = 20;
    static constexpr int kBottomMargin = 16;
#endif

    // ==========================================================================
    // EQUAL-WIDTH CONTAINER LAYOUT
    // Both Phase and Balance sit in 120px containers for visual balance
    // ==========================================================================
    static constexpr int kControlContainerWidth = bws::tokens::barometer::layout::CONTROL_CONTAINER_WIDTH;
    static constexpr int kControlsGap = bws::tokens::barometer::spacing::CONTROLS_GAP;

    // Label positioning (labels ABOVE controls)
    static constexpr int kLabelHeight = bws::tokens::barometer::layout::LABEL_HEIGHT;
    static constexpr int kLabelToControlGap = bws::tokens::barometer::spacing::LABEL_TO_CONTROL_GAP;
    static constexpr int kValueLabelHeight = bws::tokens::barometer::layout::VALUE_LABEL_HEIGHT;
    static constexpr int kValueToSliderGap = bws::tokens::barometer::layout::VALUE_TO_SLIDER_GAP;
    static constexpr int kSliderLabelSpace = bws::tokens::barometer::layout::SLIDER_LABEL_SPACE;
    static constexpr int kLufsReadoutHeight = bws::tokens::barometer::layout::LUFS_READOUT_HEIGHT;
    static constexpr int kCorrelationMeterHeight = bws::tokens::barometer::layout::CORRELATION_METER_HEIGHT;

    // Controls row height (slider + value label above)
    static constexpr int kControlsRowHeight = kBalanceSliderHeight; // 24px (just the slider)

    // Controls group width: Container (120) + Gap (24) + Container (120) = 264px
    static constexpr int kControlsGroupWidth = (kControlContainerWidth * 2) + kControlsGap;

    // Brand name height
    static constexpr int kBrandHeight = bws::tokens::barometer::layout::BRAND_HEIGHT;

    // A/B Button (centered above output meter)
    static constexpr int kABButtonWidth = bws::tokens::barometer::layout::AB_BUTTON_WIDTH;
    static constexpr int kABButtonHeight = bws::tokens::barometer::layout::AB_BUTTON_HEIGHT;
    static constexpr int kABButtonAboveMeterGap = bws::tokens::barometer::layout::AB_BUTTON_ABOVE_METER_GAP;

    // DC Filter Button (centered above input meter)
    static constexpr int kDcFilterButtonWidth = bws::tokens::barometer::layout::DC_FILTER_BUTTON_WIDTH;
    static constexpr int kDcFilterButtonHeight = bws::tokens::barometer::layout::DC_FILTER_BUTTON_HEIGHT;

    // Settings Gear Button (top-right corner)
    static constexpr int kSettingsButtonSize = bws::tokens::barometer::layout::SETTINGS_BUTTON_SIZE;
    static constexpr int kSettingsButtonRightMargin = bws::tokens::barometer::layout::SETTINGS_BUTTON_RIGHT_MARGIN;
    static constexpr int kSettingsButtonTopMargin = bws::tokens::barometer::layout::SETTINGS_BUTTON_TOP_MARGIN;

    // Balance/Width Tab Buttons (above slider)
    static constexpr int kTabButtonWidth = bws::tokens::barometer::layout::TAB_BUTTON_WIDTH;
    static constexpr int kTabButtonHeight = bws::tokens::barometer::layout::TAB_BUTTON_HEIGHT;
    static constexpr int kTabButtonGap = bws::tokens::barometer::layout::TAB_BUTTON_GAP;

    // Preset Dropdown
    static constexpr int kPresetDropdownWidth = bws::tokens::barometer::layout::PRESET_DROPDOWN_WIDTH;
    static constexpr int kPresetDropdownHeight = bws::tokens::barometer::layout::PRESET_DROPDOWN_HEIGHT;
    static constexpr int kPresetDropdownTopGap = bws::tokens::barometer::layout::PRESET_DROPDOWN_TOP_GAP;

    // Output Meter (right side of dial) - Stereo L/R
    static constexpr int kMeterBarWidth = bws::tokens::barometer::layout::METER_BAR_WIDTH;
    static constexpr int kMeterGap = bws::tokens::barometer::layout::METER_GAP;
    static constexpr int kMeterWidth = kMeterBarWidth * 2 + kMeterGap; // Total meter width (19px)
    static constexpr int kMeterHeight = bws::tokens::barometer::layout::METER_HEIGHT;
    static constexpr int kMeterDialGap = bws::tokens::barometer::spacing::METER_DIAL_GAP;

    // Plugin dimensions (computed from variant spacing)
    // Layout: TopMargin + Title + Gap + Preset + Gap + Dial + Gap + Controls + SliderLabels + Gap + Brand +
    // BottomMargin
    static constexpr int kPluginWidth = bws::tokens::barometer::layout::PLUGIN_WIDTH;
    static constexpr int kPluginBaseHeight = kTopMargin + kTitleHeight + kTitleToReadoutGap + kReadoutHeight +
                                             kReadoutToDialGap + kHeroDialSize + kDialToControlsGap +
                                             kControlsRowHeight + kSliderLabelSpace + kLufsReadoutHeight +
                                             kCorrelationMeterHeight + 2 // +2px gap
                                             + kControlsToBrandGap + kBrandHeight + kBottomMargin;
    static constexpr int kPluginHeight = kPluginBaseHeight; // Alias for backward compat


    // Colors (referencing design tokens - see bw_ui/generated/BwTokens.h)
    static constexpr juce::uint32 kBackgroundColor = bws::tokens::barometer::bg::BASE;       // Dark charcoal
    static constexpr juce::uint32 kTitleColor = bws::tokens::barometer::text::TITLE;         // Warm ivory
    static constexpr juce::uint32 kBrandingColor = bws::tokens::barometer::text::BRANDING;   // Subtle gray
    static constexpr juce::uint32 kReadoutBgColor = bws::tokens::barometer::bg::READOUT;     // Slightly lighter
    static constexpr juce::uint32 kReadoutTextColor = bws::tokens::barometer::text::READOUT; // Warm ivory
    static constexpr juce::uint32 kPhaseActiveColor = bws::tokens::barometer::indicator::PHASE_ACTIVE; // Brass gold
    static constexpr juce::uint32 kPhaseInactiveColor = bws::tokens::barometer::bg::PHASE_INACTIVE;    // Dark gray
    static constexpr juce::uint32 kPhaseBorderColor = bws::tokens::barometer::text::PHASE_BORDER;      // Border

    // Balance slider colors
    static constexpr juce::uint32 kSliderTrackColor = bws::tokens::barometer::bg::SLIDER_TRACK; // Dark track
    static constexpr juce::uint32 kSliderThumbColor =
        bws::tokens::barometer::accent::BRASS_ACTIVE; // Brass gold (matches phase)

    // Helper to format gain display
    juce::String formatGainValue(float gainDb) const;

    // Helper to format balance display
    juce::String formatBalanceValue(float balance) const;

    // Helper to paint output meter
    void paintOutputMeter(juce::Graphics& g, juce::Rectangle<int> bounds);

    // Helper to paint input meter
    void paintInputMeter(juce::Graphics& g, juce::Rectangle<int> bounds);

    // Helper to paint LUFS/TP readout strip
    void paintLufsReadout(juce::Graphics& g, juce::Rectangle<int> bounds);

    // Helper to update A/B button appearance
    void updateABButtonText();

    // Helper to update phase button appearance
    void updatePhaseButtonAppearance();

    // Phase button click handlers (for link logic)
    void onPhaseLClicked();
    void onPhaseRClicked();
    void onPhaseLinkClicked();

    // Balance/Width tab helpers
    void setBalanceWidthMode(int mode);

    // Settings menu helpers
    void showSettingsMenu();
    void resetToDefaults();

public:
    /// Launch the hidden Signal Lock overlay.
    void openSignalLockOverlay();

    /// Test-only: true while the easter-egg overlay is alive.
    [[nodiscard]] bool isToyOverlayActiveForTest() const noexcept { return toyOverlay_ != nullptr; }

    /// Test-only: drives the overlay's close path (equivalent to Escape).
    void triggerToyOverlayCloseForTest();

private:
    // User preset helpers
    void rebuildPresetCombo();
    void syncPresetComboSelection();
    void showSavePresetDialog();
    void showManagePresetsDialog();
    void showPresetLocation();
    void showChangePresetLocationDialog();

    // Keyboard input handler
    using ResizableEditor::keyPressed;
    bool keyPressed(const juce::KeyPress& key) override;

    // Preset ComboBox ID constants
    static constexpr int kUserPresetIdOffset = 100; // User preset IDs start at 100
    static constexpr int kSavePresetId = 9001;
    static constexpr int kManagePresetsId = 9002;
    static constexpr int kShowPresetLocationId = 9003;
    static constexpr int kChangePresetLocationId = 9004;

    // Stereo Correlation Meter
    std::unique_ptr<bws::ui::UiCorrelationMeter> correlationMeter_;
    juce::Rectangle<int> correlationMeterBounds_;

    // Sparkline history - short-term LUFS trend (5 seconds at 30Hz)
    static constexpr size_t kSparklineHistorySize = 150;
    std::array<float, kSparklineHistorySize> sparklineHistory_ {};
    size_t sparklineWritePos_ = 0;
    void paintSparkline(juce::Graphics& g, juce::Rectangle<float> bounds) const;
    void writeSparklineSample();

    bws::ui::adapters::JuceTimerClock uiClock_;
    struct UiTicker : bws::ui::kernel::ITicker
    {
        BarometerEditor* owner_ = nullptr;
        void tick(double dtSeconds) override { owner_->onUiTick(dtSeconds); }
    };
    UiTicker uiTicker_;
    struct FadeTicker : bws::ui::kernel::ITicker
    {
        BarometerEditor* owner_ = nullptr;
        void tick(double dtSeconds) override { (void)dtSeconds; }
    };
    FadeTicker fadeTicker_;

    // LUFS Target Coloring
    enum class LufsTarget
    {
        Off,
        Streaming_14,
        AppleMusic_16,
        Broadcast_23,
        Broadcast_24
    };
    LufsTarget lufsTarget_ = LufsTarget::Streaming_14;
    float getLufsTargetValue() const;
    void saveLufsTargetPreference();
    void loadLufsTargetPreference();

    // Peak hold state (tracked in UI at timer rate, ~30Hz)
    static constexpr int kPeakHoldTicks = bws::tokens::barometer::animation::PEAK_HOLD_TICKS;
    static constexpr float kPeakFloorDb = -100.0f;
    static constexpr float kPeakRateHz = 30.0f;
    static constexpr float kPeakHoldSeconds = static_cast<float>(kPeakHoldTicks) / kPeakRateHz;
    static constexpr float kPeakFallDbPerSec = bws::tokens::barometer::animation::METER_FALL_RATE_DB * kPeakRateHz;
    bws::audio::PeakHold peakHoldInputL_ {.value = kPeakFloorDb};
    bws::audio::PeakHold peakHoldInputR_ {.value = kPeakFloorDb};
    bws::audio::PeakHold peakHoldOutputL_ {.value = kPeakFloorDb};
    bws::audio::PeakHold peakHoldOutputR_ {.value = kPeakFloorDb};

    // Meter bounds (cached for repaint region)
    juce::Rectangle<int> meterBounds_;       // Output meter (right of dial)
    juce::Rectangle<int> inputMeterBounds_;  // Input meter (left of dial)
    juce::Rectangle<int> lufsReadoutBounds_; // LUFS/TP readout strip below controls
    int lufsHoveredColumn_ = -1;             // -1 = none, 0-4 = M/S/I/TP/LRA

    // Meter interaction state (click L/R bar to Solo L/R, Cmd+click to Swap)
    // Track which specific bar (L or R) is hovered for visual feedback
    enum class MeterBarHover
    {
        None,
        Left,
        Right
    };
    MeterBarHover inputMeterHover_ = MeterBarHover::None;
    MeterBarHover outputMeterHover_ = MeterBarHover::None;

    // Track which meter was clicked when Solo was activated (for indicator placement)
    // true = clicked on Input meter, false = clicked on Output meter
    bool soloLClickedOnInput_ = true;
    bool soloRClickedOnInput_ = true;

    // Legacy compatibility (used by some existing code)
    bool inputMeterHovered_ = false;
    bool outputMeterHovered_ = false;

    // Meter interaction handlers
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    // Helper to toggle a bool parameter
    void toggleBoolParameter(const juce::String& paramId);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BarometerEditor)
};

} // namespace bws::barometer

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

// BarometerEditorSetup.cpp - constructor, layout, and theme-snapshot setup for BarometerEditor

#include "BarometerEditor.h"
#include "ui/BarometerToyOverlay.h"
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

bws::ui::kernel::ThemeSnapshot BarometerEditor::makeEditorKernelTheme(const bws::ui::UiThemeResolved& theme)
{
    return bws::ui::adapters::makeKernelThemeSnapshot(theme);
}

BarometerEditor::BarometerEditor(BarometerProcessor& p)
    : ResizableEditorBase(p, {0, 0, kPluginWidth, kPluginBaseHeight})
    , processor_(p)
{
    // Register as preset-manager listener as the first ctor-body action, so
    // any notification fired during subsequent construction (incl. the
    // startup rebuildPresetCombo below) reaches us. removeListener in dtor.
    processor_.getPresetManager().addListener(this);

    setupControls();
    setupMetersAndState();
    setupPreferencesAndOverlay();
}

void BarometerEditor::setupControls()
{

    // =========================================================================
    // Hero Gain Dial (260px) - UiArcKnob
    // =========================================================================
    gainKnob_ = std::make_unique<bws::ui::UiArcKnob>();

    // Configure gain range: -96 to +24 dB
    gainKnob_->setRange(BarometerProcessor::kGainMinDb, BarometerProcessor::kGainMaxDb, 0.1);
    gainKnob_->setSkewFactorFromMidPoint(0.0); // Center at 0 dB
    gainKnob_->setDoubleClickReturnValue(true, 0.0);
    gainKnob_->setDoubleClickAction(bws::ui::UiArcKnob::DoubleClickAction::Reset);

    // Set brass gold color to match theme
    gainKnob_->setArcColour(juce::Colour(kPhaseActiveColor)); // Brass gold

    // Show tick mark at 0 dB
    gainKnob_->setShowTickMark(true);
    gainKnob_->setTickMarkValue(0.0);

    // Custom value formatter for dB display
    gainKnob_->setValueDisplayFormat([](double value) -> juce::String {
        if (value <= BarometerProcessor::kGainMinDb)
            return juce::String::fromUTF8("-\u221E dB"); // -∞ dB
        if (value >= 0.0)
            return "+" + juce::String(value, 1) + " dB";
        return juce::String(value, 1) + " dB";
    });

    gainKnob_->setTooltip("Gain: -96 to +24 dB\nShift+Drag: Output Trim\nCmd+Drag: Fine adjustment\nDouble-click: "
                          "Enter gain value\nShift+Double-click: Enter trim value\nCmd+Double-click: Reset to "
                          "default\nRight-click: Context menu\nScroll: Adjust value");
    addAndMakeVisible(gainKnob_.get());

    // Direct APVTS attachment (UiArcKnob is a Slider)
    gainAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor_.getApvts(), BarometerProcessor::kGainParamId, *gainKnob_);

    // =========================================================================
    // Balance Slider (120px × 24px) - Horizontal slider for L/R balance
    // Replaces the dial to avoid "baby barometer" visual competition
    // =========================================================================
    balanceSlider_ = std::make_unique<MouseWheelSlider>();
    balanceSlider_->setSliderStyle(juce::Slider::LinearHorizontal);
    balanceSlider_->setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    balanceSlider_->setRange(-100.0, 100.0, 1.0);
    balanceSlider_->setValue(0.0);                        // Center default
    balanceSlider_->setDoubleClickReturnValue(true, 0.0); // Double-click returns to center
    balanceSlider_->setTooltip("Balance: L 100 to R 100\nDouble-click to center");
    // Make slider completely transparent - we paint custom thumb in paint()
    balanceSlider_->setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    balanceSlider_->setColour(juce::Slider::trackColourId, juce::Colours::transparentBlack);
    balanceSlider_->setColour(juce::Slider::thumbColourId, juce::Colours::transparentBlack);

    const float initialBalance =
        processor_.getApvts().getRawParameterValue(BarometerProcessor::kBalanceParamId)->load();
    balanceSlider_->setValue(static_cast<double>(initialBalance), juce::dontSendNotification);

    // Repaint on value change to update custom rendering
    balanceSlider_->onValueChange = [this]() {
        repaint();
    };

    addAndMakeVisible(balanceSlider_.get());

    balanceAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor_.getApvts(), BarometerProcessor::kBalanceParamId, *balanceSlider_);

    // =========================================================================
    // Per-Channel ontrols (3-button group: L, Link, R)
    // =========================================================================

    // Left utton
    phaseLButton_ = std::make_unique<MouseWheelToggleButton>("L");
    phaseLButton_->setClickingTogglesState(true);
    phaseLButton_->setTooltip("Invert Left Channel Phase\nWhen linked, also toggles Right");
    phaseLButton_->setColour(juce::TextButton::buttonColourId, juce::Colour(kPhaseInactiveColor));
    phaseLButton_->setColour(juce::TextButton::buttonOnColourId, juce::Colour(kPhaseActiveColor));
    phaseLButton_->setColour(juce::TextButton::textColourOffId,
                             juce::Colour(bws::tokens::barometer::indicator::PHASE_TEXT));
    phaseLButton_->setColour(juce::TextButton::textColourOnId, juce::Colour(bws::tokens::barometer::bg::BASE));
    phaseLButton_->setMouseCursor(juce::MouseCursor::PointingHandCursor);
    addAndMakeVisible(phaseLButton_.get());

    // Link Button (chain symbol when linked)
    phaseLinkButton_ = std::make_unique<MouseWheelToggleButton>();
    phaseLinkButton_->setButtonText(juce::String::charToString(0x26AF)); // ⚯ chain symbol
    phaseLinkButton_->setClickingTogglesState(true);
    phaseLinkButton_->setTooltip("Link L/R Phase Controls");
    phaseLinkButton_->setColour(juce::TextButton::buttonColourId, juce::Colour(kPhaseInactiveColor));
    phaseLinkButton_->setColour(juce::TextButton::buttonOnColourId,
                                juce::Colour(bws::tokens::barometer::accent::LINK_BLUE)); // Blue for link
    phaseLinkButton_->setColour(juce::TextButton::textColourOffId,
                                juce::Colour(bws::tokens::barometer::text::PHASE_BORDER));
    phaseLinkButton_->setColour(juce::TextButton::textColourOnId, juce::Colour(0xFFFFFFFF));
    phaseLinkButton_->setMouseCursor(juce::MouseCursor::PointingHandCursor);
    addAndMakeVisible(phaseLinkButton_.get());

    // Right utton
    phaseRButton_ = std::make_unique<MouseWheelToggleButton>("R");
    phaseRButton_->setClickingTogglesState(true);
    phaseRButton_->setTooltip("Invert Right Channel Phase\nWhen linked, also toggles Left");
    phaseRButton_->setColour(juce::TextButton::buttonColourId, juce::Colour(kPhaseInactiveColor));
    phaseRButton_->setColour(juce::TextButton::buttonOnColourId, juce::Colour(kPhaseActiveColor));
    phaseRButton_->setColour(juce::TextButton::textColourOffId,
                             juce::Colour(bws::tokens::barometer::indicator::PHASE_TEXT));
    phaseRButton_->setColour(juce::TextButton::textColourOnId, juce::Colour(bws::tokens::barometer::bg::BASE));
    phaseRButton_->setMouseCursor(juce::MouseCursor::PointingHandCursor);
    addAndMakeVisible(phaseRButton_.get());

    // Mono Button
    monoButton_ = std::make_unique<MouseWheelToggleButton>("M");
    monoButton_->setClickingTogglesState(true);
    monoButton_->setTooltip("Mono: Sum to mono for mix checking\nKeyboard: Cmd+M");
    monoButton_->setColour(juce::TextButton::buttonColourId, juce::Colour(kPhaseInactiveColor));
    monoButton_->setColour(juce::TextButton::buttonOnColourId, juce::Colour(kPhaseActiveColor));
    monoButton_->setColour(juce::TextButton::textColourOffId,
                           juce::Colour(bws::tokens::barometer::indicator::PHASE_TEXT));
    monoButton_->setColour(juce::TextButton::textColourOnId, juce::Colour(bws::tokens::barometer::bg::BASE));
    monoButton_->setMouseCursor(juce::MouseCursor::PointingHandCursor);
    addAndMakeVisible(monoButton_.get());

    // Get initial states from processor
    const bool initialPhaseL =
        processor_.getApvts().getRawParameterValue(BarometerProcessor::kPhaseInvertLParamId)->load() > 0.5f;
    const bool initialPhaseR =
        processor_.getApvts().getRawParameterValue(BarometerProcessor::kPhaseInvertRParamId)->load() > 0.5f;
    const bool initialLink =
        processor_.getApvts().getRawParameterValue(BarometerProcessor::kPhaseLinkParamId)->load() > 0.5f;
    const bool initialMono =
        processor_.getApvts().getRawParameterValue(BarometerProcessor::kMonoParamId)->load() > 0.5f;

    phaseLButton_->setToggleState(initialPhaseL, juce::dontSendNotification);
    phaseRButton_->setToggleState(initialPhaseR, juce::dontSendNotification);
    phaseLinkButton_->setToggleState(initialLink, juce::dontSendNotification);
    monoButton_->setToggleState(initialMono, juce::dontSendNotification);

    // APVTS Attachments
    phaseLAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor_.getApvts(), BarometerProcessor::kPhaseInvertLParamId, *phaseLButton_);
    phaseRAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor_.getApvts(), BarometerProcessor::kPhaseInvertRParamId, *phaseRButton_);
    phaseLinkAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor_.getApvts(), BarometerProcessor::kPhaseLinkParamId, *phaseLinkButton_);
    monoAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor_.getApvts(), BarometerProcessor::kMonoParamId, *monoButton_);

    // Click handlers for link behavior
    phaseLButton_->onClick = [this]() {
        onPhaseLClicked();
    };
    phaseRButton_->onClick = [this]() {
        onPhaseRClicked();
    };
    phaseLinkButton_->onClick = [this]() {
        onPhaseLinkClicked();
    };

    updatePhaseButtonAppearance();

    // =========================================================================
    // A/B Compare Toggle (shows both A and B, highlights active state)
    // =========================================================================
    abToggle_ = std::make_unique<ABToggleButton>();
    abToggle_->setTooltip(
        "A/B Compare\nClick: Toggle A/B\nAlt+Click: Copy A to B\nShift+Click: Copy B to A\nSpace Bar: Toggle");

    abToggle_->onClick = [this]() {
        processor_.toggleAB();
        updateABButtonText();
        repaint();
    };

    abToggle_->onCopyAToB = [this]() {
        processor_.copyAToB();
        // Brief visual feedback could be added here
        repaint();
    };

    abToggle_->onCopyBToA = [this]() {
        processor_.copyBToA();
        // Brief visual feedback could be added here
        repaint();
    };

    updateABButtonText();
    addAndMakeVisible(abToggle_.get());

    // =========================================================================
    // DC Filter Toggle Button (above input meter)
    // =========================================================================
    dcFilterButton_ = std::make_unique<MouseWheelToggleButton>("DC");
    dcFilterButton_->setClickingTogglesState(true);
    dcFilterButton_->setTooltip(
        "DC Filter: Removes DC offset with 5Hz high-pass filter\nUseful for vinyl transfers and some preamps");
    dcFilterButton_->setColour(juce::TextButton::buttonColourId, juce::Colour(kPhaseInactiveColor));
    dcFilterButton_->setColour(juce::TextButton::buttonOnColourId,
                               juce::Colour(bws::tokens::barometer::indicator::DC_FILTER_ACTIVE)); // Blue when active
    dcFilterButton_->setColour(juce::TextButton::textColourOffId,
                               juce::Colour(bws::tokens::barometer::indicator::PHASE_TEXT));
    dcFilterButton_->setColour(juce::TextButton::textColourOnId, juce::Colour(0xFFFFFFFF));
    dcFilterButton_->setMouseCursor(juce::MouseCursor::PointingHandCursor);
    addAndMakeVisible(dcFilterButton_.get());

    // Get initial state
    const bool initialDcFilter =
        processor_.getApvts().getRawParameterValue(BarometerProcessor::kDcFilterParamId)->load() > 0.5f;
    dcFilterButton_->setToggleState(initialDcFilter, juce::dontSendNotification);

    // APVTS Attachment
    dcFilterAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor_.getApvts(), BarometerProcessor::kDcFilterParamId, *dcFilterButton_);

    // =========================================================================
    // Hero Knob - Shift+Drag Trim Mode (multi-parameter trim)
    // =========================================================================
    // Set up callback for Shift+drag to control Output Trim parameter
    gainKnob_->onTrimValueChange = [this](float normalizedValue) {
        auto* param = processor_.getApvts().getParameter(BarometerProcessor::kOutputTrimParamId);
        if (param)
        {
            param->setValueNotifyingHost(normalizedValue);
        }
    };

    // Callbacks for DAW automation gesture tracking
    gainKnob_->onTrimDragStart = [this]() {
        auto* param = processor_.getApvts().getParameter(BarometerProcessor::kOutputTrimParamId);
        if (param)
            param->beginChangeGesture();
    };

    gainKnob_->onTrimDragEnd = [this]() {
        auto* param = processor_.getApvts().getParameter(BarometerProcessor::kOutputTrimParamId);
        if (param)
            param->endChangeGesture();
    };

    // Callback to get current trim value for display
    gainKnob_->getTrimValue = [this]() -> float {
        return processor_.getApvts().getRawParameterValue(BarometerProcessor::kOutputTrimParamId)->load();
    };

    // Format trim value display: "TRIM: +3.0 dB"
    gainKnob_->setTrimDisplayFormat([](double value) -> juce::String {
        if (value >= 0.0)
            return "TRIM: +" + juce::String(value, 1) + " dB";
        else
            return "TRIM: " + juce::String(value, 1) + " dB";
    });

    // =========================================================================
    // Preset Dropdown (below title) - with scroll wheel navigation
    // =========================================================================
    presetCombo_ = std::make_unique<ScrollablePresetComboBox>("Presets");
    presetCombo_->setTooltip("Presets\nClick: Open menu\nScroll: Next/Previous preset\nShift+Scroll: Skip 5 "
                             "presets\nCmd+S: Save | Cmd+R: Reset");

    // Style the combo box
    presetCombo_->setColour(juce::ComboBox::backgroundColourId, juce::Colour(bws::tokens::barometer::bg::READOUT));
    presetCombo_->setColour(juce::ComboBox::textColourId, juce::Colour(kTitleColor));
    presetCombo_->setColour(juce::ComboBox::outlineColourId,
                            juce::Colour(bws::tokens::barometer::text::OUTLINE_BORDER));
    presetCombo_->setColour(juce::ComboBox::arrowColourId, juce::Colour(kPhaseActiveColor));

    // Populate preset combo (factory + user presets + actions)
    rebuildPresetCombo();
    syncPresetComboSelection();

    presetCombo_->onChange = [this]() {
        const int selectedId = presetCombo_->getSelectedId();

        if (selectedId >= 1 && selectedId <= processor_.getFactoryPresetCount())
        {
            // Factory preset (IDs 1-10)
            processor_.loadPreset(selectedId - 1);
        }
        else if (selectedId >= kUserPresetIdOffset && selectedId < kSavePresetId)
        {
            // User preset (IDs 100+)
            processor_.loadUserPreset(selectedId - kUserPresetIdOffset);
        }
        else if (selectedId == kSavePresetId)
        {
            // Show save dialog
            showSavePresetDialog();
        }
        else if (selectedId == kManagePresetsId)
        {
            // Show manage dialog
            showManagePresetsDialog();
        }
        else if (selectedId == kShowPresetLocationId)
        {
            // Show preset location in Finder/Explorer
            showPresetLocation();
        }
        else if (selectedId == kChangePresetLocationId)
        {
            // Change preset library location
            showChangePresetLocationDialog();
        }
    };

    // Scroll wheel navigation for presets
    presetCombo_->onNavigate = [this](int offset) {
        const int currentId = presetCombo_->getSelectedId();

        // Calculate the total number of navigable presets (factory + user)
        const int numFactory = processor_.getFactoryPresetCount();
        const int numUser = static_cast<int>(processor_.getNumUserPresets());
        const int totalPresets = numFactory + numUser;

        if (totalPresets == 0)
            return;

        // Determine current index in the combined list
        int currentIndex = 0;
        if (currentId >= 1 && currentId <= numFactory)
        {
            currentIndex = currentId - 1; // Factory preset
        }
        else if (currentId >= kUserPresetIdOffset && currentId < kUserPresetIdOffset + numUser)
        {
            currentIndex = numFactory + (currentId - kUserPresetIdOffset); // User preset
        }

        // Calculate new index with wrapping
        int newIndex = currentIndex + offset;

        // Clamp to valid range (no wrapping)
        newIndex = juce::jlimit(0, totalPresets - 1, newIndex);

        // Convert back to combo box ID
        int newId;
        if (newIndex < numFactory)
        {
            newId = newIndex + 1; // Factory preset ID
        }
        else
        {
            newId = kUserPresetIdOffset + (newIndex - numFactory); // User preset ID
        }

        if (newId != currentId)
        {
            presetCombo_->setSelectedId(newId, juce::sendNotification);
        }
    };

    addAndMakeVisible(presetCombo_.get());

    // =========================================================================
    // Width Slider (shares visual space with balance slider)
    // =========================================================================
    widthSlider_ = std::make_unique<MouseWheelSlider>();
    widthSlider_->setSliderStyle(juce::Slider::LinearHorizontal);
    widthSlider_->setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    widthSlider_->setRange(0.0, 2.0, 0.01);
    widthSlider_->setValue(1.0);                        // 100% default
    widthSlider_->setDoubleClickReturnValue(true, 1.0); // Double-click returns to 100%
    widthSlider_->setTooltip("Stereo Width: 0% (mono) to 200% (wide)\nDouble-click for 100%");
    widthSlider_->setVisible(false); // Hidden by default (Balance mode)
    // Make slider completely transparent - we paint custom thumb in paint()
    widthSlider_->setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    widthSlider_->setColour(juce::Slider::trackColourId, juce::Colours::transparentBlack);
    widthSlider_->setColour(juce::Slider::thumbColourId, juce::Colours::transparentBlack);

    const float initialWidth = processor_.getApvts().getRawParameterValue(BarometerProcessor::kWidthParamId)->load();
    widthSlider_->setValue(static_cast<double>(initialWidth), juce::dontSendNotification);

    widthSlider_->onValueChange = [this]() {
        repaint();
    };

    addAndMakeVisible(widthSlider_.get());

    widthAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor_.getApvts(), BarometerProcessor::kWidthParamId, *widthSlider_);

    // =========================================================================
    // Balance/Width Tab Group
    // =========================================================================
    balanceWidthTabs_ = std::make_unique<bws::weather::WeatherTabGroup>();
    balanceWidthTabs_->setTheme(getTheme());
    balanceWidthTabs_->addTab("Balance");
    balanceWidthTabs_->addTab("Width");
    balanceWidthTabs_->onTabChanged = [this](int index) {
        setBalanceWidthMode(index);
    };
    balanceWidthTabs_->setSelectedTab(0, false); // Start with Balance selected
    addAndMakeVisible(balanceWidthTabs_.get());

    // =========================================================================
    // Settings Gear Button (top-right corner)
    // =========================================================================
    settingsButton_ = std::make_unique<juce::TextButton>();
    settingsButton_->setButtonText(juce::String::charToString(0x2699)); // ⚙ gear symbol
    settingsButton_->setTooltip("Settings");
    settingsButton_->setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    settingsButton_->setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    settingsButton_->setColour(juce::TextButton::textColourOffId, juce::Colour(bws::tokens::barometer::text::BRANDING));
    settingsButton_->setColour(juce::TextButton::textColourOnId, juce::Colour(kPhaseActiveColor));
    settingsButton_->setMouseCursor(juce::MouseCursor::PointingHandCursor);
    settingsButton_->onClick = [this]() {
        showSettingsMenu();
    };
    addAndMakeVisible(settingsButton_.get());


    // =========================================================================
    // Background: Procedural solid color (asset-free)
    // =========================================================================
    // The background is rendered procedurally in paint().
}

void BarometerEditor::setupMetersAndState()
{
    // =========================================================================
    // Stereo Correlation Meter
    // =========================================================================
    correlationMeter_ = std::make_unique<bws::ui::UiCorrelationMeter>(getTheme());
    correlationMeter_->setMode(bws::ui::UiCorrelationMeter::Mode::Bar);
    correlationMeter_->setOrientation(true); // Horizontal
    correlationMeter_->setShowNumericReadout(true);
    correlationMeter_->setShowPeakHold(true);
    correlationMeter_->setShowMonoSafeIndicator(false); // Too tall for 20px strip
    addAndMakeVisible(correlationMeter_.get());

    // Load LUFS target preference
    loadLufsTargetPreference();

    // Initialize sparkline history to silence
    sparklineHistory_.fill(-70.0F);
}

void BarometerEditor::setupPreferencesAndOverlay()
{
    // =========================================================================
    // Preferences & Post-Init
    // =========================================================================

    // Load tooltip preference and create tooltip window if enabled
    loadTooltipPreference();

    // Create value display popup (hover value display)
    valueDisplayPopup_ = std::make_unique<ValueDisplayPopup>();
    addChildComponent(valueDisplayPopup_.get()); // Initially hidden

    // Barometer uses the settings gear menu and corner drag for resize, not a
    // dedicated fullscreen button.
    setFullScreenButtonVisible(false);

    // Initialize slider visibility explicitly (balanceWidthMode_ defaults to 0)
    // This ensures the balance slider is interactive from the start
    balanceSlider_->setVisible(true);
    balanceSlider_->setInterceptsMouseClicks(true, true);
    widthSlider_->setVisible(false);

    // All meter animation runs on one clock at 30 Hz.
    if (correlationMeter_)
    {
        correlationMeter_->setExternallyDriven(true);
    }
    uiTicker_.owner_ = this;
    uiClock_.subscribe(&uiTicker_, 30.0);
    fadeTicker_.owner_ = this;
    uiClock_.subscribe(&fadeTicker_, 60.0);

    // Propagate the default theme to children
    onThemeChanged();

    // Force initial layout - the base class constructor's setSize() triggered
    // resized() → layoutComponents() before our components existed (the guard
    // returned early). Now that everything is constructed, run layout once.
    resized();
}

void BarometerEditor::layoutComponents()
{
    // Guard: base class constructor triggers layoutComponents() before our
    // components are created in the derived constructor body.
    if (!gainKnob_)
        return;

    // Update scale factor for components that need it
    if (abToggle_)
    {
        abToggle_->setScaleFactor(getScaleFactor());
        abToggle_->setTheme(getTheme());
    }

    const int width = getWidth();

    // =========================================================================
    // Settings Gear: Top-right corner
    // =========================================================================
    const int settingsSize = scaled(kSettingsButtonSize);
    const int settingsX = width - scaled(kSettingsButtonRightMargin) - settingsSize;
    const int settingsY = scaled(kSettingsButtonTopMargin);
    settingsButton_->setBounds(settingsX, settingsY, settingsSize, settingsSize);


    if (aboutOverlay_)
        aboutOverlay_->setBounds(getLocalBounds());

    if (toyOverlay_)
        toyOverlay_->setBounds(getLocalBounds());

    // =========================================================================
    // Preset Dropdown: Below title, centered
    // =========================================================================
    const int presetY = scaled(kTopMargin + kTitleHeight + kTitleToPresetGap);
    const int presetW = scaled(kPresetDropdownWidth);
    const int presetH = scaled(kPresetDropdownHeight);
    const int presetX = (width - presetW) / 2;
    presetCombo_->setBounds(presetX, presetY, presetW, presetH);

    // =========================================================================
    // Hero Arc Knob: Centered between input and output meters
    // =========================================================================
    const int dialSize = scaled(kHeroDialSize);
    const int dialY = presetY + presetH + scaled(kPresetToDialGap);
    const int dialX = (width - dialSize) / 2;
    gainKnob_->setBounds(dialX, dialY, dialSize, dialSize);

    // =========================================================================
    // Input Meter: Left side of dial (BEFORE processing)
    // =========================================================================
    const int meterW = scaled(kMeterWidth);
    const int meterH = scaled(kMeterHeight);
    const int meterGap = scaled(kMeterDialGap);
    const int meterY = dialY + (dialSize - meterH) / 2; // Vertically centered with dial
    const int inputMeterX = dialX - meterGap - meterW;
    inputMeterBounds_ = juce::Rectangle<int>(inputMeterX, meterY, meterW, meterH);

    // =========================================================================
    // Output Meter: Right side of dial (AFTER processing)
    // =========================================================================
    const int outputMeterX = dialX + dialSize + meterGap;
    meterBounds_ = juce::Rectangle<int>(outputMeterX, meterY, meterW, meterH);

    // =========================================================================
    // A/B Button: Centered above output meter
    // =========================================================================
    const int abW = scaled(kABButtonWidth);
    const int abH = scaled(kABButtonHeight);
    const int abButtonX = outputMeterX + (meterW - abW) / 2;
    const int abButtonY = dialY - abH - scaled(kABButtonAboveMeterGap);
    abToggle_->setBounds(abButtonX, abButtonY, abW, abH);

    // =========================================================================
    // DC Filter Button: Centered above input meter
    // =========================================================================
    const int dcW = scaled(kDcFilterButtonWidth);
    const int dcH = scaled(kDcFilterButtonHeight);
    const int dcButtonX = inputMeterX + (meterW - dcW) / 2;
    const int dcButtonY = dialY - dcH - scaled(kABButtonAboveMeterGap); // Same gap as A/B
    dcFilterButton_->setBounds(dcButtonX, dcButtonY, dcW, dcH);

    // =========================================================================
    // Controls row: two equal-width containers
    // =========================================================================
    const int controlsY = dialY + dialSize + scaled(kDialToControlsGap);
    const int controlsGroupW = scaled(kControlsGroupWidth);
    const int controlsGroupX = (width - controlsGroupW) / 2;

    // Phase container (120px): 4-button group (114px) centered within
    const int phaseContainerX = controlsGroupX;
    const int containerW = scaled(kControlContainerWidth);
    const int phaseGroupW = scaled(kPhaseGroupWidth);
    const int phaseGroupOffset = (containerW - phaseGroupW) / 2; // Center 114px in 120px
    const int btnSize = scaled(kPhaseButtonSize);
    const int btnGap = scaled(kPhaseButtonGap);
    const int phaseLX = phaseContainerX + phaseGroupOffset;
    const int phaseLinkX = phaseLX + btnSize + btnGap;
    const int phaseRX = phaseLinkX + btnSize + btnGap;
    const int monoX = phaseRX + btnSize + btnGap;

    phaseLButton_->setBounds(phaseLX, controlsY, btnSize, btnSize);
    phaseLinkButton_->setBounds(phaseLinkX, controlsY, btnSize, btnSize);
    phaseRButton_->setBounds(phaseRX, controlsY, btnSize, btnSize);
    monoButton_->setBounds(monoX, controlsY, btnSize, btnSize);

    // Balance/Width container (120px): Tab buttons above, slider below
    const int balanceContainerX = phaseContainerX + containerW + scaled(kControlsGap);

    // ==========================================================================
    // BALANCE/WIDTH LAYOUT
    // Layout from top to bottom:
    //   1. Value readout (text baseline aligned with PHASE label)
    //   2. Tab buttons (Balance/Width) - positioned just above slider
    //   3. Slider track
    // ==========================================================================
    const int tabW = scaled(kTabButtonWidth);
    const int tabH = scaled(kTabButtonHeight);
    const int tabGap = scaled(kTabButtonGap);
    const int tabTotalW = tabW * 2 + tabGap;
    const int tabX = balanceContainerX + (scaled(kBalanceSliderWidth) - tabTotalW) / 2;

    // Position tabs just above the slider with minimal gap
    const int tabY = controlsY - tabH - scaled(4); // 4px gap above slider

    // Configure tab group sizing and position
    if (balanceWidthTabs_)
    {
        balanceWidthTabs_->setTabSize(tabW, tabH);
        balanceWidthTabs_->setTabGap(tabGap);
        balanceWidthTabs_->setBounds(tabX, tabY, tabTotalW, tabH);
    }

    // Both sliders share the same bounds (only one visible at a time)
    balanceSlider_->setBounds(balanceContainerX, controlsY, scaled(kBalanceSliderWidth), scaled(kBalanceSliderHeight));
    widthSlider_->setBounds(balanceContainerX, controlsY, scaled(kBalanceSliderWidth), scaled(kBalanceSliderHeight));

    // =========================================================================
    // LUFS Readout Strip: Below slider labels, above correlation meter
    // =========================================================================
    const int lufsReadoutY = controlsY + scaled(kControlsRowHeight) + scaled(kSliderLabelSpace);
    const int lufsReadoutH = scaled(kLufsReadoutHeight);
    const int lufsReadoutInset = scaled(24); // Horizontal inset from edges
    lufsReadoutBounds_ =
        juce::Rectangle<int>(lufsReadoutInset, lufsReadoutY, getWidth() - (lufsReadoutInset * 2), lufsReadoutH);

    // =========================================================================
    // Stereo Correlation Meter: Below LUFS readout, above branding
    // =========================================================================
    const int corrMeterY = lufsReadoutY + lufsReadoutH + scaled(2);
    const int corrMeterH = scaled(kCorrelationMeterHeight);
    correlationMeterBounds_ =
        juce::Rectangle<int>(lufsReadoutInset, corrMeterY, getWidth() - (lufsReadoutInset * 2), corrMeterH);
    if (correlationMeter_)
    {
        correlationMeter_->setBounds(correlationMeterBounds_);
        correlationMeter_->setScale(getScaleFactor());
    }
}

} // namespace bws::barometer

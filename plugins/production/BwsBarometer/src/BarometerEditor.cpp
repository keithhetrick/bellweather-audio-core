// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

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


BarometerEditor::~BarometerEditor()
{
    // Unregister before destroying the rest of the editor so no late
    // listener callback can touch destroyed members.
    processor_.getPresetManager().removeListener(this);

    valueDisplayPopup_.reset();
    tooltipWindow_.reset();
    gainAttachment_.reset();
    balanceAttachment_.reset();
    widthAttachment_.reset();
    phaseLAttachment_.reset();
    phaseRAttachment_.reset();
    phaseLinkAttachment_.reset();
    monoAttachment_.reset();
    dcFilterAttachment_.reset();
    gainKnob_.reset();
    balanceSlider_.reset();
    widthSlider_.reset();
    phaseLButton_.reset();
    phaseLinkButton_.reset();
    phaseRButton_.reset();
    monoButton_.reset();
    abToggle_.reset();
    dcFilterButton_.reset();
    presetCombo_.reset();
    balanceWidthTabs_.reset();
    settingsButton_.reset();
    correlationMeter_.reset();
}

void BarometerEditor::onThemeChanged()
{
    const auto& t = getTheme();
    if (balanceWidthTabs_)
        balanceWidthTabs_->setTheme(t);
    if (aboutOverlay_)
        aboutOverlay_->setTheme(t);
    repaint();
}

void BarometerEditor::onUiTick(double dtSeconds)
{
    // Output meter (right of dial)
    if (!meterBounds_.isEmpty())
    {
        auto expandedBounds = meterBounds_.expanded(2, 2);
        expandedBounds.setWidth(meterBounds_.getWidth() + 22);   // Include dB labels
        expandedBounds.setHeight(meterBounds_.getHeight() + 20); // Include L/R labels + IN/OUT label
        repaint(expandedBounds);
    }

    // Input meter (left of dial)
    if (!inputMeterBounds_.isEmpty())
    {
        auto expandedBounds = inputMeterBounds_.expanded(2, 2);
        expandedBounds.setWidth(inputMeterBounds_.getWidth() + 22);   // Include dB labels
        expandedBounds.setHeight(inputMeterBounds_.getHeight() + 20); // Include L/R labels + IN/OUT label
        repaint(expandedBounds);
    }

    const auto dt = static_cast<float>(dtSeconds);
    peakHoldInputL_.updateLinear(processor_.getInputLeftPeakDb(), kPeakHoldSeconds, kPeakFallDbPerSec, dt);
    peakHoldInputR_.updateLinear(processor_.getInputRightPeakDb(), kPeakHoldSeconds, kPeakFallDbPerSec, dt);
    peakHoldOutputL_.updateLinear(processor_.getLeftPeakDb(), kPeakHoldSeconds, kPeakFallDbPerSec, dt);
    peakHoldOutputR_.updateLinear(processor_.getRightPeakDb(), kPeakHoldSeconds, kPeakFallDbPerSec, dt);

    if (correlationMeter_)
    {
        correlationMeter_->pushCorrelation(processor_.getStereoCorrelation());
        correlationMeter_->advanceExternal(dtSeconds);
    }

    writeSparklineSample();

    if (!lufsReadoutBounds_.isEmpty())
    {
        repaint(lufsReadoutBounds_);
    }
}

void BarometerEditor::writeSparklineSample()
{
    sparklineHistory_[sparklineWritePos_] = processor_.getShortTermLufs();
    sparklineWritePos_ = (sparklineWritePos_ + 1) % kSparklineHistorySize;
}

void BarometerEditor::updateABButtonText()
{
    const bool isA = processor_.isStateA();
    abToggle_->setActiveState(isA);
}

void BarometerEditor::updatePhaseButtonAppearance()
{
    // Update button text to show phase state
    // Show "Ø" when inverted, otherwise show L/R
    phaseLButton_->setButtonText(phaseLButton_->getToggleState() ? juce::String::charToString(0x00D8) // Ø
                                                                 : "L");
    phaseRButton_->setButtonText(phaseRButton_->getToggleState() ? juce::String::charToString(0x00D8) // Ø
                                                                 : "R");

    // Link button: chain when linked, empty when unlinked
    phaseLinkButton_->setButtonText(phaseLinkButton_->getToggleState()
                                        ? juce::String::charToString(0x26AF) // ⚯ chain symbol
                                        : " ");

    repaint();
}

void BarometerEditor::onPhaseLClicked()
{
    const bool isLinked = phaseLinkButton_->getToggleState();
    if (isLinked)
    {
        // When linked, sync R to match L
        phaseRButton_->setToggleState(phaseLButton_->getToggleState(), juce::sendNotification);
    }
    updatePhaseButtonAppearance();
}

void BarometerEditor::onPhaseRClicked()
{
    const bool isLinked = phaseLinkButton_->getToggleState();
    if (isLinked)
    {
        // When linked, sync L to match R
        phaseLButton_->setToggleState(phaseRButton_->getToggleState(), juce::sendNotification);
    }
    updatePhaseButtonAppearance();
}

void BarometerEditor::onPhaseLinkClicked()
{
    const bool nowLinked = phaseLinkButton_->getToggleState();
    if (nowLinked)
    {
        // When enabling link, sync R to match L
        phaseRButton_->setToggleState(phaseLButton_->getToggleState(), juce::sendNotification);
    }
    updatePhaseButtonAppearance();
}


// =============================================================================
// Scale Preference Persistence
// =============================================================================

juce::PropertiesFile* BarometerEditor::getPropertiesFile()
{
    static juce::PropertiesFile::Options options;
    options.applicationName = "BwsBarometer";
    options.filenameSuffix = ".settings";
    options.folderName = "BellweatherStudios";
    options.osxLibrarySubFolder = "Application Support";

    static std::unique_ptr<juce::PropertiesFile> props;
    if (!props)
        props = std::make_unique<juce::PropertiesFile>(options);

    return props.get();
}

// =============================================================================
// ResizableEditor Overrides
// =============================================================================

juce::String BarometerEditor::getPluginName() const
{
    return "BwsBarometer";
}

// =============================================================================
// Tooltip Preference Persistence
// =============================================================================


// =============================================================================
// LUFS Target Preference Persistence
// =============================================================================


// =============================================================================
// Meter Interaction Handlers (Solo/Swap via click)
// =============================================================================

void BarometerEditor::toggleBoolParameter(const juce::String& paramId)
{
    auto* param = processor_.getApvts().getParameter(paramId);
    if (param)
    {
        const bool current = param->getValue() > 0.5f;
        param->setValueNotifyingHost(current ? 0.0f : 1.0f);
    }
}


} // namespace bws::barometer

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

// BarometerEditorInteraction.cpp - mouse, keyboard, and overlay interaction methods for BarometerEditor

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

bool BarometerEditor::keyPressed(const juce::KeyPress& key)
{
    // Space bar: Toggle A/B
    if (key.isKeyCode(juce::KeyPress::spaceKey))
    {
        processor_.toggleAB();
        updateABButtonText();
        repaint();
        return true;
    }

    // Up arrow: Navigate to previous preset
    if (key.isKeyCode(juce::KeyPress::upKey))
    {
        const int currentId = presetCombo_->getSelectedId();
        if (currentId > 1)
        {
            presetCombo_->setSelectedId(currentId - 1);
        }
        return true;
    }

    // Down arrow: Navigate to next preset
    if (key.isKeyCode(juce::KeyPress::downKey))
    {
        const int currentId = presetCombo_->getSelectedId();
        const int maxFactoryId = 10;
        const int maxUserId = kUserPresetIdOffset + processor_.getNumUserPresets() - 1;

        // Determine next valid ID (skip section headers and action items)
        int nextId = currentId + 1;
        if (currentId < maxFactoryId)
        {
            // Within factory presets
            if (nextId <= maxFactoryId)
            {
                presetCombo_->setSelectedId(nextId);
            }
            else if (processor_.getNumUserPresets() > 0)
            {
                // Jump to first user preset
                presetCombo_->setSelectedId(kUserPresetIdOffset);
            }
        }
        else if (currentId >= kUserPresetIdOffset && currentId < maxUserId)
        {
            // Within user presets
            presetCombo_->setSelectedId(nextId);
        }
        return true;
    }

    // Cmd/Ctrl+R: Reset to Unity preset
    if (key.getModifiers().isCommandDown() && key.getKeyCode() == 'R')
    {
        processor_.loadPreset(0);                                   // Unity preset (index 0)
        presetCombo_->setSelectedId(1, juce::dontSendNotification); // ID 1 = Unity
        return true;
    }

    // Cmd/Ctrl+S: Save preset
    if (key.getModifiers().isCommandDown() && key.getKeyCode() == 'S')
    {
        showSavePresetDialog();
        return true;
    }

    // Cmd/Ctrl+T: Toggle tooltips
    if (key.getModifiers().isCommandDown() && key.getKeyCode() == 'T')
    {
        setTooltipsEnabled(!tooltipsEnabled_);
        return true;
    }

    // Cmd/Ctrl+M: Toggle mono
    if (key.getModifiers().isCommandDown() && key.getKeyCode() == 'M')
    {
        monoButton_->setToggleState(!monoButton_->getToggleState(), juce::sendNotification);
        return true;
    }

    // Cmd/Ctrl+Z: Undo
    if (key.getModifiers().isCommandDown() && !key.getModifiers().isShiftDown() && key.getKeyCode() == 'Z')
    {
        processor_.getUndoManager().undo();
        return true;
    }

    // Cmd/Ctrl+Shift+Z: Redo
    if (key.getModifiers().isCommandDown() && key.getModifiers().isShiftDown() && key.getKeyCode() == 'Z')
    {
        processor_.getUndoManager().redo();
        return true;
    }

    return false;
}

void BarometerEditor::setBalanceWidthMode(int mode)
{
    if (balanceWidthMode_ == mode)
        return;

    balanceWidthMode_ = mode;

    // Toggle visibility of sliders
    balanceSlider_->setVisible(mode == 0);
    widthSlider_->setVisible(mode == 1);

    // Update tab selection (don't notify - avoid recursion)
    if (balanceWidthTabs_)
    {
        balanceWidthTabs_->setSelectedTab(mode, false);
    }

    repaint();
}

void BarometerEditor::triggerToyOverlayCloseForTest()
{
    if (toyOverlay_)
    {
        toyOverlay_->keyPressed(juce::KeyPress(juce::KeyPress::escapeKey));
    }
}

void BarometerEditor::openSignalLockOverlay()
{
    if (toyOverlay_ != nullptr)
        return;

    // SafePointer: editor may be torn down between Escape and the queued reset.
    juce::Component::SafePointer<BarometerEditor> safeThis(this);
    toyOverlay_ = std::make_unique<bws::barometer::toy::BarometerToyOverlay>(
        [safeThis]() {
            juce::MessageManager::callAsync([safeThis]() {
                if (auto* self = safeThis.getComponent())
                {
                    self->toyOverlay_.reset();
                }
            });
        },
        getScaleFactor());
    toyOverlay_->setBounds(getLocalBounds());
    addAndMakeVisible(*toyOverlay_);
    toyOverlay_->grabKeyboardFocus();
}

void BarometerEditor::mouseDown(const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();

    // Helper lambda to handle solo toggle based on which bar (L or R) was clicked
    // Click on left bar of meter = Solo L, click on right bar = Solo R
    // isInputMeter: true if click is on input meter, false if on output meter
    auto handleMeterClick = [this, &e](const juce::Rectangle<int>& meterBounds, int clickX, bool isInputMeter) {
        if (e.mods.isCommandDown())
        {
            // Cmd+Click: Toggle Swap L/R
            toggleBoolParameter(BarometerProcessor::kSwapLRParamId);
        }
        else
        {
            // Determine which bar was clicked (L is left half, R is right half)
            const int meterCenterX = meterBounds.getCentreX();
            const bool clickedLeftBar = clickX < meterCenterX;

            auto* soloLParam = processor_.getApvts().getParameter(BarometerProcessor::kSoloLParamId);
            auto* soloRParam = processor_.getApvts().getParameter(BarometerProcessor::kSoloRParamId);

            if (clickedLeftBar)
            {
                // Click on L bar: Toggle Solo Left
                if (soloLParam)
                {
                    const bool currentSoloL = soloLParam->getValue() > 0.5f;

                    // Begin gesture for automation recording
                    soloLParam->beginChangeGesture();

                    if (!currentSoloL && soloRParam)
                    {
                        // Turning Solo L on, turn Solo R off (mutual exclusion)
                        soloRParam->beginChangeGesture();
                        soloRParam->setValueNotifyingHost(0.0f);
                        soloRParam->endChangeGesture();
                    }
                    soloLParam->setValueNotifyingHost(currentSoloL ? 0.0f : 1.0f);
                    soloLParam->endChangeGesture();

                    // Track which meter was clicked (only when turning on)
                    if (!currentSoloL)
                    {
                        soloLClickedOnInput_ = isInputMeter;
                    }
                }
            }
            else
            {
                // Click on R bar: Toggle Solo Right
                if (soloRParam)
                {
                    const bool currentSoloR = soloRParam->getValue() > 0.5f;

                    // Begin gesture for automation recording
                    soloRParam->beginChangeGesture();

                    if (!currentSoloR && soloLParam)
                    {
                        // Turning Solo R on, turn Solo L off (mutual exclusion)
                        soloLParam->beginChangeGesture();
                        soloLParam->setValueNotifyingHost(0.0f);
                        soloLParam->endChangeGesture();
                    }
                    soloRParam->setValueNotifyingHost(currentSoloR ? 0.0f : 1.0f);
                    soloRParam->endChangeGesture();

                    // Track which meter was clicked (only when turning on)
                    if (!currentSoloR)
                    {
                        soloRClickedOnInput_ = isInputMeter;
                    }
                }
            }
        }
    };

    // Check if click is on input meter (left of dial)
    if (inputMeterBounds_.contains(pos))
    {
        handleMeterClick(inputMeterBounds_, pos.x, true); // true = input meter
        repaint();
        return;
    }

    // Check if click is on output meter (right of dial)
    if (meterBounds_.contains(pos))
    {
        handleMeterClick(meterBounds_, pos.x, false); // false = output meter
        repaint();
        return;
    }

    // Double-click on LUFS readout strip: reset integrated LUFS + true-peak hold
    if (lufsReadoutBounds_.contains(pos) && e.getNumberOfClicks() >= 2)
    {
        processor_.resetIntegratedLufs();
        processor_.resetTruePeakHold();
        repaint(lufsReadoutBounds_);
        return;
    }

    // Pass to base class for other interactions
    juce::AudioProcessorEditor::mouseDown(e);
}

void BarometerEditor::mouseMove(const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();
    bool needsRepaint = false;

    // Helper to determine which bar (L or R) the mouse is over
    auto getBarHover = [](const juce::Rectangle<int>& bounds, int x) -> MeterBarHover {
        if (!bounds.contains(x, bounds.getCentreY()))
            return MeterBarHover::None;
        return x < bounds.getCentreX() ? MeterBarHover::Left : MeterBarHover::Right;
    };

    // Check input meter hover (per-bar)
    const auto newInputHover =
        inputMeterBounds_.contains(pos) ? getBarHover(inputMeterBounds_, pos.x) : MeterBarHover::None;
    if (newInputHover != inputMeterHover_)
    {
        inputMeterHover_ = newInputHover;
        inputMeterHovered_ = (newInputHover != MeterBarHover::None);
        needsRepaint = true;
    }

    // Check output meter hover (per-bar)
    const auto newOutputHover = meterBounds_.contains(pos) ? getBarHover(meterBounds_, pos.x) : MeterBarHover::None;
    if (newOutputHover != outputMeterHover_)
    {
        outputMeterHover_ = newOutputHover;
        outputMeterHovered_ = (newOutputHover != MeterBarHover::None);
        needsRepaint = true;
    }

    // LUFS readout tooltip hit-testing (per-column)
    if (lufsReadoutBounds_.contains(pos))
    {
        const int colW = lufsReadoutBounds_.getWidth() / 5;
        const int col = juce::jlimit(0, 4, (pos.x - lufsReadoutBounds_.getX()) / std::max(1, colW));
        if (col != lufsHoveredColumn_)
        {
            lufsHoveredColumn_ = col;

            juce::String tooltip;
            switch (col)
            {
            case 0:
                tooltip = "Momentary (M): 400ms sliding window loudness\nShows instantaneous perceived "
                          "loudness\nUseful for finding the loudest moments";
                break;
            case 1:
                tooltip = "Short-Term (S): 3-second sliding window\nSmooths fast changes for section "
                          "comparison\nBetter for relative loudness of song parts";
                break;
            case 2:
            {
                // Dynamic tooltip - includes active target and color legend
                tooltip = "Integrated (I): Gated program loudness\nITU-R BS.1770-5 gated mean over full measurement";
                if (lufsTarget_ != LufsTarget::Off)
                {
                    const float target = getLufsTargetValue();
                    juce::String targetName;
                    switch (lufsTarget_)
                    {
                    case LufsTarget::Streaming_14:
                        targetName = "Streaming";
                        break;
                    case LufsTarget::AppleMusic_16:
                        targetName = "Apple Music";
                        break;
                    case LufsTarget::Broadcast_23:
                        targetName = "EBU R128";
                        break;
                    case LufsTarget::Broadcast_24:
                        targetName = "ATSC A/85";
                        break;
                    case LufsTarget::Off:
                        break;
                    }
                    tooltip += "\n\nTarget: " + juce::String(static_cast<int>(target)) + " LUFS (" + targetName + ")";
                    tooltip += "\n  Green pill = On target (within +/-1 LU)";
                    tooltip += "\n  Bold amber = Slightly hot (1-3 LU above)";
                    tooltip += "\n  Bold red + underline = Too loud (>3 LU above)";
                }
                else
                {
                    tooltip += "\nThe number streaming platforms use for normalization";
                    tooltip += "\nSet a target in Settings > LUFS Target for color guidance";
                }
                tooltip += "\nDouble-click to reset";
                break;
            }
            case 3:
                tooltip = "True Peak (TP): 4x oversampled inter-sample peak\nStreaming platforms require < -1 "
                          "dBTP\nBrass = warning, Red = clipping\nDouble-click to reset hold";
                break;
            case 4:
                tooltip = "Loudness Range (LRA): Dynamic range in LU\n3-5 = heavily compressed\n7-10 = typical "
                          "pop/rock\n10+ = wide dynamics (classical, film)";
                break;
            default:
                break;
            }
            setTooltip(tooltip);
        }
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
        if (needsRepaint)
            repaint();
        return;
    }
    else if (lufsHoveredColumn_ >= 0)
    {
        lufsHoveredColumn_ = -1;
        setTooltip({});
    }

    // Update cursor based on hover state
    if (inputMeterHovered_ || outputMeterHovered_)
    {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }
    else
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    if (needsRepaint)
        repaint();
}

void BarometerEditor::mouseExit(const juce::MouseEvent&)
{
    if (inputMeterHovered_ || outputMeterHovered_)
    {
        inputMeterHover_ = MeterBarHover::None;
        outputMeterHover_ = MeterBarHover::None;
        inputMeterHovered_ = false;
        outputMeterHovered_ = false;
        setMouseCursor(juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void BarometerEditor::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    juce::AudioProcessorEditor::mouseWheelMove(e, wheel);
}

void BarometerEditor::mouseDrag(const juce::MouseEvent& e)
{
    juce::AudioProcessorEditor::mouseDrag(e);
}

void BarometerEditor::mouseUp(const juce::MouseEvent& e)
{
    juce::AudioProcessorEditor::mouseUp(e);
}

} // namespace bws::barometer

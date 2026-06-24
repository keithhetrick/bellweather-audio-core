// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "BarometerToyOverlay.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>

namespace bws::barometer::toy
{

namespace
{
constexpr double kErrorSampleInterval = 1.0 / 30.0; ///< ~30Hz sample rate

const std::array<int, 10>& konamiSequence()
{
    static const std::array<int, 10> seq = {
        juce::KeyPress::upKey,
        juce::KeyPress::upKey,
        juce::KeyPress::downKey,
        juce::KeyPress::downKey,
        juce::KeyPress::leftKey,
        juce::KeyPress::rightKey,
        juce::KeyPress::leftKey,
        juce::KeyPress::rightKey,
        'B',
        'A',
    };
    return seq;
}

juce::StringArray buildHintLines()
{
    juce::StringArray lines {"WASD OR ARROWS FOR TRIM", "ALIGN ACTIVE WAVE WITH REFERENCE", "SHIFT FOR FINE ADJUST"};
    if (juce::PluginHostType().isProTools())
    {
        lines.add("PRO TOOLS: TURN OFF KEYBOARD FOCUS (A..Z) TO PLAY");
    }
    return lines;
}
} // namespace

BarometerToyOverlay::BarometerToyOverlay(std::function<void()> onClose, float scale)
    : state_(createInitialSignalLockState())
    , timer_([this](double dt) { tick(dt); })
    , hint_(buildHintLines(), 6, scale)
    , onClose_(std::move(onClose))
    , scale_(juce::jmax(0.25f, scale))
{
    view_.setSignalLockState(&state_);
    view_.setScale(scale_);
    addAndMakeVisible(view_);
    addAndMakeVisible(hint_);
    setOpaque(true);
    setAlwaysOnTop(true);
    setWantsKeyboardFocus(true);
    timer_.start();
}

BarometerToyOverlay::~BarometerToyOverlay()
{
    timer_.stop();
}

void BarometerToyOverlay::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a)); // Barometer bg.base
}

void BarometerToyOverlay::resized()
{
    view_.setBounds(getLocalBounds());

    const int hintW = static_cast<int>(360.0f * scale_);
    const int hintH = static_cast<int>(130.0f * scale_);
    const auto bounds = getLocalBounds();
    hint_.setBounds(bounds.getCentreX() - hintW / 2, bounds.getCentreY() - hintH / 2, hintW, hintH);
}

bool BarometerToyOverlay::feedKonami(int keyCode) noexcept
{
    const auto& seq = konamiSequence();
    for (int retry = 0; retry < 2; ++retry)
    {
        if (keyCode == seq[konamiMatched_])
        {
            ++konamiMatched_;
            if (konamiMatched_ == seq.size())
            {
                konamiMatched_ = 0;
                return true;
            }
            return false;
        }
        if (konamiMatched_ == 0)
        {
            return false;
        }
        konamiMatched_ = 0;
    }
    return false;
}

bool BarometerToyOverlay::keyPressed(const juce::KeyPress& key)
{
    if (feedKonami(key.getKeyCode()))
    {
        solveSignalLock(state_);
    }
    if (key == juce::KeyPress::escapeKey)
    {
        if (onClose_)
        {
            onClose_();
        }
        return true;
    }
    if (key == juce::KeyPress('A') || key == juce::KeyPress(juce::KeyPress::leftKey))
    {
        held_.left_ = true;
        return true;
    }
    if (key == juce::KeyPress('D') || key == juce::KeyPress(juce::KeyPress::rightKey))
    {
        held_.right_ = true;
        return true;
    }
    if (key == juce::KeyPress('W') || key == juce::KeyPress(juce::KeyPress::upKey))
    {
        held_.up_ = true;
        return true;
    }
    if (key == juce::KeyPress('S') || key == juce::KeyPress(juce::KeyPress::downKey))
    {
        held_.down_ = true;
        return true;
    }
    return false;
}

bool BarometerToyOverlay::keyStateChanged(bool isKeyDown)
{
    if (isKeyDown)
    {
        return false;
    }
    refreshHeldKeysFromOs();
    return false;
}

void BarometerToyOverlay::focusLost(FocusChangeType /*cause*/)
{
    held_ = {};
}

void BarometerToyOverlay::visibilityChanged()
{
    if (!isVisible())
    {
        held_ = {};
    }
}

void BarometerToyOverlay::parentHierarchyChanged()
{
    if (isShowing() && !hasKeyboardFocus(true))
    {
        juce::Component::SafePointer<BarometerToyOverlay> self(this);
        juce::MessageManager::callAsync([self] {
            if (self)
            {
                self->grabKeyboardFocus();
            }
        });
    }
}

void BarometerToyOverlay::refreshHeldKeysFromOs() noexcept
{
    held_.left_ =
        juce::KeyPress::isKeyCurrentlyDown('A') || juce::KeyPress::isKeyCurrentlyDown(juce::KeyPress::leftKey);
    held_.right_ =
        juce::KeyPress::isKeyCurrentlyDown('D') || juce::KeyPress::isKeyCurrentlyDown(juce::KeyPress::rightKey);
    held_.up_ = juce::KeyPress::isKeyCurrentlyDown('W') || juce::KeyPress::isKeyCurrentlyDown(juce::KeyPress::upKey);
    held_.down_ =
        juce::KeyPress::isKeyCurrentlyDown('S') || juce::KeyPress::isKeyCurrentlyDown(juce::KeyPress::downKey);
}

SignalLockInput BarometerToyOverlay::sampleInput() const noexcept
{
    SignalLockInput in {};
    int phase = 0;
    int freq = 0;
    if (held_.left_)
    {
        phase += 1;
    }
    if (held_.right_)
    {
        phase -= 1;
    }
    if (held_.up_)
    {
        freq += 1;
    }
    if (held_.down_)
    {
        freq -= 1;
    }
    in.phaseDirection = phase;
    in.frequencyDirection = freq;
    in.fineMode = juce::ModifierKeys::getCurrentModifiers().isShiftDown();
    return in;
}

void BarometerToyOverlay::tick(double dt)
{
    updateSignalLock(state_, dt, sampleInput());

    errorSampleAccum_ += dt;
    if (errorSampleAccum_ >= kErrorSampleInterval)
    {
        errorSampleAccum_ -= kErrorSampleInterval;
        view_.pushErrorSample(state_.error);
    }

    repaint();
}

} // namespace bws::barometer::toy

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_juce_adapters/JucePresetStateBridge.h"

#include <unordered_set>

namespace bws::adapters
{

JucePresetStateBridge::JucePresetStateBridge(juce::AudioProcessor& processor, juce::AudioProcessorValueTreeState& apvts)
    : processor_(processor)
    , apvts_(apvts)
    , callbackState_(std::make_shared<CallbackState>())
    , dirtyTracker_(std::make_shared<bws::preset::AtomicPresetDirtyTracker>())
{
    std::unordered_set<std::string> registeredIds;
    for (auto* parameter : processor_.getParameters())
    {
        auto* hosted = dynamic_cast<juce::HostedAudioProcessorParameter*>(parameter);
        if (hosted == nullptr)
            continue;

        const auto parameterId = hosted->getParameterID();
        if (parameterId.isEmpty())
        {
            jassertfalse;
            continue;
        }

        if (!registeredIds.insert(parameterId.toStdString()).second)
        {
            jassertfalse;
            continue;
        }

        if (apvts_.getParameter(parameterId) == nullptr)
        {
            jassertfalse;
            continue;
        }

        apvts_.addParameterListener(parameterId, this);
        parameterIds_.add(parameterId);
    }

    startTimerHz(30);
}

JucePresetStateBridge::~JucePresetStateBridge()
{
    stopTimer();
    detachParameterListeners();
    if (callbackState_ != nullptr)
    {
        const std::scoped_lock lock(callbackState_->mutex);
        callbackState_->callback = {};
    }
}

bws::preset::PresetStateBytes JucePresetStateBridge::captureState() const
{
    juce::MemoryBlock stateData;
    processor_.getStateInformation(stateData);
    return bws::preset::PresetStateBytes(stateData.getData(), stateData.getSize());
}

bool JucePresetStateBridge::applyState(bws::domain::BwStateBlob state)
{
    auto suppression = suppressDirtyNotifications();
    processor_.setStateInformation(state.data.data(), static_cast<int>(state.data.size()));
    return true;
}

bws::preset::PresetStateSubscription JucePresetStateBridge::subscribeToDirtyChanges(std::function<void()> callback)
{
    if (callbackState_ == nullptr)
        return {};

    {
        const std::scoped_lock lock(callbackState_->mutex);
        callbackState_->callback = std::move(callback);
    }

    std::weak_ptr<CallbackState> weakState(callbackState_);
    return bws::preset::PresetStateSubscription([weakState] {
        if (auto state = weakState.lock())
        {
            const std::scoped_lock lock(state->mutex);
            state->callback = {};
        }
    });
}

bws::preset::ScopedPresetStateSuppression JucePresetStateBridge::suppressDirtyNotifications()
{
    dirtyTracker_->beginSuppression();

    std::weak_ptr<bws::preset::AtomicPresetDirtyTracker> weakTracker(dirtyTracker_);
    return bws::preset::ScopedPresetStateSuppression([weakTracker] {
        if (auto tracker = weakTracker.lock())
            tracker->endSuppression();
    });
}

void JucePresetStateBridge::resetDirtyTracking(bool eligible)
{
    dirtyTracker_->reset(eligible);
}

bool JucePresetStateBridge::hasPendingDirtyChange() const
{
    return dirtyTracker_->hasPendingDirtyChange();
}

void JucePresetStateBridge::flushPendingDirtyChangeForTesting()
{
    flushPendingDirtyChange();
}

void JucePresetStateBridge::detachParameterListeners()
{
    for (const auto& parameterId : parameterIds_)
        apvts_.removeParameterListener(parameterId, this);
    parameterIds_.clear();
}

void JucePresetStateBridge::parameterChanged(const juce::String&, float)
{
    dirtyTracker_->markDirtyFromRealtime();
}

void JucePresetStateBridge::timerCallback()
{
    flushPendingDirtyChange();
}

void JucePresetStateBridge::flushPendingDirtyChange()
{
    if (!dirtyTracker_->consumePendingDirtyChange())
        return;

    std::function<void()> callback;
    if (callbackState_ != nullptr)
    {
        const std::scoped_lock lock(callbackState_->mutex);
        callback = callbackState_->callback;
    }

    if (callback)
        callback();
}

} // namespace bws::adapters

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_juce_adapters/JucePresetStateBridge.h"

#include "bw_juce_adapters/BwsAudioProcessor.h"

#include <bw_rt/AudioThreadScope.h>

#include <unordered_set>
#include <optional>

namespace bws::adapters
{

JucePresetStateBridge::JucePresetStateBridge(juce::AudioProcessor& processor, juce::AudioProcessorValueTreeState& apvts,
                                             bws::preset::IColdStateTransaction* transaction)
    : processor_(processor)
    , apvts_(apvts)
    , transaction_(transaction)
    , callbackState_(std::make_shared<CallbackState>())
    , dirtyTracker_(std::make_shared<bws::preset::AtomicPresetDirtyTracker>())
{
    std::unordered_set<std::string> registeredIds;
    for (auto* parameter : processor_.getParameters())
    {
        auto* hosted = dynamic_cast<juce::HostedAudioProcessorParameter*>(parameter);
        if (hosted == nullptr)
        {
            mappingValid_ = false;
            continue;
        }

        const auto parameterId = hosted->getParameterID();
        if (parameterId.isEmpty())
        {
            jassertfalse;
            mappingValid_ = false;
            continue;
        }

        if (!registeredIds.insert(parameterId.toStdString()).second)
        {
            jassertfalse;
            mappingValid_ = false;
            continue;
        }

        if (apvts_.getParameter(parameterId) == nullptr)
        {
            jassertfalse;
            mappingValid_ = false;
            continue;
        }

        parameterIds_.add(parameterId);
    }
}

JucePresetStateBridge::~JucePresetStateBridge()
{
    deactivate();
    if (callbackState_ != nullptr)
    {
        const std::scoped_lock lock(callbackState_->mutex);
        callbackState_->callback = {};
    }
}

bool JucePresetStateBridge::activate()
{
    if (active_.load(std::memory_order_acquire))
        return true;
    if (!mappingValid_ || parameterIds_.isEmpty())
        return false;

    dirtyTracker_->reset(false);
    for (const auto& parameterId : parameterIds_)
        apvts_.addParameterListener(parameterId, this);
    active_.store(true, std::memory_order_release);
    startTimerHz(30);
    return true;
}

void JucePresetStateBridge::deactivate() noexcept
{
    if (!active_.exchange(false, std::memory_order_acq_rel))
        return;
    stopTimer();
    detachParameterListeners();
    dirtyTracker_->reset(false);
}

bws::preset::PresetStateBytes JucePresetStateBridge::captureState() const
{
    auto result = captureStateResult();
    return result.verified() || result.status == bws::preset::StateCaptureStatus::capturedUnverified
               ? std::move(result.bytes)
               : bws::preset::PresetStateBytes {};
}

bws::preset::StateCaptureResult JucePresetStateBridge::captureStateResult() const
{
    if (bws::rt::AudioThreadScope::isAudioThread())
        return {bws::preset::StateCaptureStatus::wrongThread, {}};
    if (transaction_ != nullptr)
        return transaction_->captureColdState();
    juce::MemoryBlock stateData;
    processor_.getStateInformation(stateData);
    if (stateData.isEmpty())
        return {bws::preset::StateCaptureStatus::failed, {}};
    return {bws::preset::StateCaptureStatus::capturedUnverified,
            bws::preset::PresetStateBytes(stateData.getData(), stateData.getSize())};
}

bws::preset::StateCaptureResult JucePresetStateBridge::embedPresetMetadataResult(
    bws::domain::BwStateBlob state, bws::preset::PresetMetadataContext metadata) const
{
    if (bws::rt::AudioThreadScope::isAudioThread())
        return {bws::preset::StateCaptureStatus::wrongThread, {}};
    if (transaction_ == nullptr)
        return {bws::preset::StateCaptureStatus::unsupported, {}};
    return transaction_->captureColdStateWithPresetMetadata(state, metadata);
}

bool JucePresetStateBridge::applyState(bws::domain::BwStateBlob state)
{
    if (bws::rt::AudioThreadScope::isAudioThread())
        return false;
    return applyStateWithIntent(state, bws::preset::ColdStateIntent::presetFile);
}

bool JucePresetStateBridge::applyStateWithIntent(bws::domain::BwStateBlob state, bws::preset::ColdStateIntent intent)
{
    if (bws::rt::AudioThreadScope::isAudioThread())
        return false;
    return applyStateWithContext(state, intent, {});
}

bool JucePresetStateBridge::applyStateWithContext(bws::domain::BwStateBlob state, bws::preset::ColdStateIntent intent,
                                                  bws::preset::ColdStateIdentityContext context)
{
    const auto result = applyStateResultWithContext(state, intent, context);
    return result.verifiedCommit() || result.status == bws::preset::StateApplyStatus::appliedUnverified;
}

bws::preset::StateApplyResult JucePresetStateBridge::applyStateResultWithContext(
    bws::domain::BwStateBlob state, bws::preset::ColdStateIntent intent, bws::preset::ColdStateIdentityContext context)
{
    if (bws::rt::AudioThreadScope::isAudioThread())
        return {bws::preset::StateApplyStatus::wrongThread};
    auto suppression = suppressDirtyNotifications();
    if (transaction_ != nullptr)
        return transaction_->applyColdStateWithContext(state, intent, context);
    // In-session intents (preset load/preview) are exempt from the base class's
    // trial-mode restore reset; only hostSession restores carry that policy.
    std::optional<bws::BwsAudioProcessor::ScopedPresetLoad> presetLoadGuard;
    if (intent != bws::preset::ColdStateIntent::hostSession)
    {
        if (auto* bwsProcessor = dynamic_cast<bws::BwsAudioProcessor*>(&processor_))
            presetLoadGuard.emplace(*bwsProcessor);
    }
    processor_.setStateInformation(state.data.data(), static_cast<int>(state.data.size()));
    return {bws::preset::StateApplyStatus::appliedUnverified};
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
}

void JucePresetStateBridge::parameterChanged(const juce::String&, float)
{
    if (!active_.load(std::memory_order_acquire))
        return;
    dirtyTracker_->markDirtyFromRealtime();
}

void JucePresetStateBridge::timerCallback()
{
    if (!active_.load(std::memory_order_acquire))
        return;
    flushPendingDirtyChange();
}

void JucePresetStateBridge::flushPendingDirtyChange()
{
    if (!active_.load(std::memory_order_acquire))
        return;
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

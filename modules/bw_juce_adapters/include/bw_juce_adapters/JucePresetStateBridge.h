// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_preset_core/IPresetStateBridge.h>
#include <bw_preset_core/PresetDirtyTracking.h>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_events/juce_events.h>

#include <functional>
#include <memory>
#include <mutex>

namespace bws::adapters
{

class JucePresetStateBridge final
    : public bws::preset::IPresetStateBridge
    , private juce::AudioProcessorValueTreeState::Listener
    , private juce::Timer
{
public:
    JucePresetStateBridge(juce::AudioProcessor& processor, juce::AudioProcessorValueTreeState& apvts);
    ~JucePresetStateBridge() override;

    [[nodiscard]] bws::preset::PresetStateBytes captureState() const override;
    bool applyState(bws::domain::BwStateBlob state) override;

    [[nodiscard]] bws::preset::PresetStateSubscription subscribeToDirtyChanges(std::function<void()> callback) override;

    [[nodiscard]] bws::preset::ScopedPresetStateSuppression suppressDirtyNotifications() override;

    void resetDirtyTracking(bool eligible) override;
    [[nodiscard]] bool hasPendingDirtyChange() const override;
    void flushPendingDirtyChangeForTesting() override;

private:
    struct CallbackState
    {
        std::mutex mutex;
        std::function<void()> callback;
    };

    void detachParameterListeners();
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void timerCallback() override;
    void flushPendingDirtyChange();

    juce::AudioProcessor& processor_;
    juce::AudioProcessorValueTreeState& apvts_;
    juce::StringArray parameterIds_;
    std::shared_ptr<CallbackState> callbackState_;
    std::shared_ptr<bws::preset::AtomicPresetDirtyTracker> dirtyTracker_;
};

} // namespace bws::adapters

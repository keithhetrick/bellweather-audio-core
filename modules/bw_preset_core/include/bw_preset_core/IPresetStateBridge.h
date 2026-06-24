// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_state_types/BwStateBlob.h>
#include <bw_preset_core/PresetState.h>

#include <functional>
#include <memory>
#include <utility>

namespace bws::preset
{

class PresetStateSubscription
{
public:
    PresetStateSubscription() = default;
    explicit PresetStateSubscription(std::function<void()> unsubscribe)
        : unsubscribe_(std::move(unsubscribe))
    {}

    PresetStateSubscription(PresetStateSubscription&& other) noexcept
        : unsubscribe_(std::move(other.unsubscribe_))
    {
        other.unsubscribe_ = {};
    }

    PresetStateSubscription& operator=(PresetStateSubscription&& other) noexcept
    {
        if (this != &other)
        {
            reset();
            unsubscribe_ = std::move(other.unsubscribe_);
            other.unsubscribe_ = {};
        }
        return *this;
    }

    PresetStateSubscription(const PresetStateSubscription&) = delete;
    PresetStateSubscription& operator=(const PresetStateSubscription&) = delete;

    ~PresetStateSubscription() { reset(); }

    void reset()
    {
        if (unsubscribe_)
        {
            auto unsubscribe = std::move(unsubscribe_);
            unsubscribe_ = {};
            unsubscribe();
        }
    }

private:
    std::function<void()> unsubscribe_;
};

class ScopedPresetStateSuppression
{
public:
    ScopedPresetStateSuppression() = default;
    explicit ScopedPresetStateSuppression(std::function<void()> onExit)
        : onExit_(std::move(onExit))
    {}

    ScopedPresetStateSuppression(ScopedPresetStateSuppression&& other) noexcept
        : onExit_(std::move(other.onExit_))
    {
        other.onExit_ = {};
    }

    ScopedPresetStateSuppression& operator=(ScopedPresetStateSuppression&& other) noexcept
    {
        if (this != &other)
        {
            reset();
            onExit_ = std::move(other.onExit_);
            other.onExit_ = {};
        }
        return *this;
    }

    ScopedPresetStateSuppression(const ScopedPresetStateSuppression&) = delete;
    ScopedPresetStateSuppression& operator=(const ScopedPresetStateSuppression&) = delete;

    ~ScopedPresetStateSuppression() { reset(); }

    void reset()
    {
        if (onExit_)
        {
            auto onExit = std::move(onExit_);
            onExit_ = {};
            onExit();
        }
    }

private:
    std::function<void()> onExit_;
};

class IPresetStateBridge
{
public:
    virtual ~IPresetStateBridge() = default;

    [[nodiscard]] virtual PresetStateBytes captureState() const = 0;
    virtual bool applyState(bws::domain::BwStateBlob state) = 0;

    [[nodiscard]] virtual PresetStateSubscription subscribeToDirtyChanges(std::function<void()> callback) = 0;

    [[nodiscard]] virtual ScopedPresetStateSuppression suppressDirtyNotifications() = 0;

    virtual void resetDirtyTracking(bool eligible) = 0;
    [[nodiscard]] virtual bool hasPendingDirtyChange() const = 0;
    virtual void flushPendingDirtyChangeForTesting() = 0;
};

} // namespace bws::preset

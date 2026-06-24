// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string_view>

namespace bws::ui
{

// Pluggable logging interface. Production uses the default JuceLogger impl
// wired via currentLog()'s Meyer's singleton. Tests intercept via ScopedLogSwap.
class ILog
{
public:
    virtual ~ILog() = default;
    virtual void writeLine(std::string_view message) = 0;
};

// Non-null. Default impl is a Meyer's singleton constructed on first call.
// Message-thread-only mutation contract for setCurrentLog.
ILog& currentLog() noexcept;

// nullptr restores the default impl. Message-thread-only.
void setCurrentLog(ILog* impl) noexcept;

// std::string_view parameters are valid only during the call - must not be
// stored. Async/buffered logging would require the impl to copy internally.
void logAutoDegrade(std::string_view widget, std::string_view widgetName, std::string_view action,
                    std::string_view reason);

// RAII helper for tests: swap an ILog impl for the scope's duration, restore
// the default on destruction. Use to assert captured-log output without
// leaking the test impl across cells.
class ScopedLogSwap
{
public:
    explicit ScopedLogSwap(ILog* test_impl) noexcept { setCurrentLog(test_impl); }
    ~ScopedLogSwap() noexcept { setCurrentLog(nullptr); }

    ScopedLogSwap(const ScopedLogSwap&) = delete;
    ScopedLogSwap& operator=(const ScopedLogSwap&) = delete;
};

} // namespace bws::ui

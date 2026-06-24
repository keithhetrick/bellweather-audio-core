// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/interaction/InteractionLogging.h"

#include <juce_core/juce_core.h>

#include <atomic>
#include <string>

namespace bws::ui
{

namespace
{

// Default implementation. Calls juce::Logger statically without retaining
// framework objects.
class JuceLogger : public ILog
{
public:
    void writeLine(std::string_view message) override
    {
#if JUCE_DEBUG
        juce::Logger::writeToLog(juce::String::fromUTF8(message.data(), static_cast<int>(message.size())));
#else
        juce::ignoreUnused(message);
#endif
    }
};

ILog& defaultLog() noexcept
{
    static JuceLogger instance;
    return instance;
}

std::atomic<ILog*> g_current {nullptr};

} // namespace

ILog& currentLog() noexcept
{
    if (auto* p = g_current.load(std::memory_order_acquire))
        return *p;
    return defaultLog();
}

void setCurrentLog(ILog* impl) noexcept
{
    g_current.store(impl, std::memory_order_release);
}

// Build the formatted message, then writeLine, then jassertfalse.
// Ordering is load-bearing: captured-log tests must intercept BEFORE the
// assert halts the test runner.
void logAutoDegrade(std::string_view widget, std::string_view widgetName, std::string_view action,
                    std::string_view reason)
{
    std::string msg;
    msg.reserve(widget.size() + widgetName.size() + action.size() + reason.size() + 96);
    msg.append(widget.data(), widget.size());
    msg.append(" '");
    msg.append(widgetName.data(), widgetName.size());
    msg.append("': DoubleClickAction::");
    msg.append(action.data(), action.size());
    msg.append(" bound but ");
    msg.append(reason.data(), reason.size());
    msg.append(". Auto-degrading to NoOp. Declare setDoubleClickAction(Reset|NoOp) to silence.");

    currentLog().writeLine(msg);

#if JUCE_DEBUG
    jassertfalse;
#endif
}

} // namespace bws::ui

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <vector>

namespace bws::ui::kernel
{

struct ITicker
{
    virtual ~ITicker() = default;
    // dtSeconds is the elapsed time since this ticker's previous tick.
    virtual void tick(double dtSeconds) = 0;
};

// Rate-aware tick source: subscribers register a rate (Hz); one owner drives
// advance(dt). A subscriber accumulates the master dt and fires once it reaches
// its period, receiving the accumulated time - so the dt values it sees sum to
// real elapsed time. Not re-entrant: do not subscribe/unsubscribe inside tick().
class Clock
{
public:
    void subscribe(ITicker* ticker, double hz)
    {
        if (ticker == nullptr || hz <= 0.0)
            return;
        for (auto& s : subs_)
        {
            if (s.ticker == ticker)
            {
                s.periodSeconds = 1.0 / hz;
                s.accum = 0.0;
                return;
            }
        }
        subs_.push_back({ticker, 1.0 / hz, 0.0});
    }

    void unsubscribe(ITicker* ticker)
    {
        for (std::size_t i = 0; i < subs_.size(); ++i)
        {
            if (subs_[i].ticker == ticker)
            {
                subs_.erase(subs_.begin() + static_cast<std::ptrdiff_t>(i));
                return;
            }
        }
    }

    void advance(double dtSeconds)
    {
        if (dtSeconds <= 0.0)
            return;
        for (auto& s : subs_)
        {
            s.accum += dtSeconds;
            if (s.accum >= s.periodSeconds)
            {
                const double elapsed = s.accum;
                s.accum = 0.0;
                s.ticker->tick(elapsed);
            }
        }
    }

    [[nodiscard]] std::size_t subscriberCount() const { return subs_.size(); }

private:
    struct Sub
    {
        ITicker* ticker;
        double periodSeconds;
        double accum;
    };
    std::vector<Sub> subs_;
};

} // namespace bws::ui::kernel

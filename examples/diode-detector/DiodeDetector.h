// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
// Experimental, fixed-circuit model. Contract: diode-detector-modeling.md.
#pragma once
#include <algorithm>
#include <cmath>

namespace bws::experiment
{
class DiodeDetector
{
public:
    struct Sample
    {
        double voltage;
        int iterations;
        bool accepted;
    };

    bool prepare(double rate, double capacitance = 1e-6) noexcept
    {
        if (!std::isfinite(rate) || rate < 8000.0 || rate > 768000.0 || !std::isfinite(capacitance) ||
            capacitance < 1e-9 || capacitance > 1e-3)
            return false;
        charge_ = capacitance * rate;
        admittance_ = charge_ + 1.0 / loadResistance;
        resistance_ = sourceResistance + 1.0 / admittance_;
        reset();
        return true;
    }

    void reset() noexcept { voltage_ = 0.0; }

    Sample process(double input) noexcept
    {
        if (!std::isfinite(input) || std::abs(input) > 4.0)
            return {voltage_, 0, false};

        // Eliminate capacitor voltage/current from backward Euler KCL.
        // Solve d + R_eff * Is * expm1(d/Vt) = offset for diode voltage d.
        const double history = charge_ * voltage_;
        const double offset = input - history / admittance_;
        double low = std::min(0.0, offset);
        double high = std::max(0.0, offset);
        int iterations = 0;
        for (; iterations < 48 && high - low > 1e-12; ++iterations)
        {
            const double mid = low + 0.5 * (high - low);
            const double residual = mid + resistance_ * saturationCurrent * std::expm1(mid / thermalVoltage) - offset;
            if (residual > 0.0)
                high = mid;
            else
                low = mid;
        }
        if (high - low > 1e-12)
            return {voltage_, iterations, false};
        const double diodeVoltage = low + 0.5 * (high - low);
        const double current = (offset - diodeVoltage) / resistance_;
        const double next = (current + history) / admittance_;
        if (!std::isfinite(next))
            return {voltage_, iterations, false};
        voltage_ = next;
        return {voltage_, iterations, true};
    }

private:
    static constexpr double sourceResistance = 1000.0;
    static constexpr double loadResistance = 100000.0;
    static constexpr double saturationCurrent = 1e-12;
    static constexpr double thermalVoltage = 8.617333262145e-5 * 300.0;
    double charge_ = 1e-6 * 48000.0;
    double admittance_ = charge_ + 1.0 / loadResistance;
    double resistance_ = sourceResistance + 1.0 / admittance_;
    double voltage_ = 0.0;
};
} // namespace bws::experiment

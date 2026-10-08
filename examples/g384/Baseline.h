// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <cmath>
#include <algorithm>

// G384-1: CONTRACT.md defines behavioral equations, not an SSL circuit model.
class Baseline
{
public:
    Baseline(double rate, double threshold, double ratio, double attack, double release, double makeup)
        : threshold_(threshold)
        , slope_(1.0 - 1.0 / ratio)
        , makeup_(makeup)
        , attack_(std::exp(-1.0 / (rate * attack)))
        , release_(std::exp(-1.0 / (rate * release)))
    {}
    double process(double left, double right) noexcept
    {
        const double peak = std::fmax(std::abs(left), std::abs(right));
        const double level = 20.0 * std::log10(std::fmax(peak, 1e-12));
        const double desired = slope_ * std::fmax(level - threshold_, 0.0);
        const double coefficient = desired > attenuation_ ? attack_ : release_;
        attenuation_ = coefficient * attenuation_ + (1.0 - coefficient) * desired;
        return makeup_ - attenuation_;
    }

private:
    double threshold_, slope_, makeup_, attack_, release_;
    double attenuation_ = 0.0;
};

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace bws::barometer
{

// Hover alpha fade. advance() moves alpha toward the hover target and returns
// true while still animating; the settle checks bound alpha to [0, 1].
struct FadeModel
{
    struct Config
    {
        float fadeInDurationMs;
        float fadeOutDurationMs;
        float fadeInMultiplier;
        float fadeOutMultiplier;
    };

    float alpha {0.0f};
    bool fadingIn {false};

    bool advance(float dtSeconds, const Config& cfg)
    {
        const float dtMs = dtSeconds * 1000.0f;
        if (fadingIn)
        {
            const float c = (dtMs / cfg.fadeInDurationMs) * cfg.fadeInMultiplier;
            alpha += (1.0f - alpha) * c;
            if (alpha >= 0.99f)
            {
                alpha = 1.0f;
                return false;
            }
        }
        else
        {
            const float c = (dtMs / cfg.fadeOutDurationMs) * cfg.fadeOutMultiplier;
            alpha *= (1.0f - c);
            if (alpha <= 0.01f)
            {
                alpha = 0.0f;
                return false;
            }
        }
        return true;
    }
};

} // namespace bws::barometer

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "bw_ui/foundation/UiTheme.h"
#include "bw_ui/kernel/Theme.h"

#include <numbers>

namespace bws::ui::kernel
{

inline constexpr float kPi = std::numbers::pi_v<float>;

enum class KnobReadoutOwnership
{
    Internal,
    ExternalCompanion,
    SpecialtyException
};

enum class KnobCompositionPattern
{
    HeroStack,
    StandardControlStack,
    CompactControlStack
};

struct KnobInteractionContract
{
    bool verticalDrag {true};
    bool fineAdjustWithShift {true};
    bool resetToDefault {true};
    bool textEntry {true};
    bool mouseWheel {true};
    bool focusable {true};
    bool arrowKeyStep {true};
    bool shiftArrowFineStep {true};
};

struct KnobCompanionPolicy
{
    bool valueReadout {true};
    bool label {true};
    bool unitsWithReadout {true};
    bool caption {false};
};

struct ArcGeometryModel
{
    float startAngle {kPi * 1.2f};
    float endAngle {kPi * 2.8f};
    float familyTrackRadiusRatio {0.82f};
    float trimTrackInsetRatio {0.10f};
    bool guideArcUsesFullSweep {true};
};

struct ResolvedKnobModel
{
    UiKnobRole publicRole {UiKnobRole::Primary};
    Density density {Density::Standard};
    KnobReadoutOwnership readoutOwnership {KnobReadoutOwnership::ExternalCompanion};
    KnobCompositionPattern compositionPattern {KnobCompositionPattern::StandardControlStack};
    UiReadoutRole companionReadoutRole {UiReadoutRole::Standard};
    KnobInteractionContract interactions {};
    KnobCompanionPolicy companions {};
    bool differentiatesBeyondSize {true};
    bool chromeMayBeSuppressed {false};
    bool internalReadoutAllowed {false};
    bool valueLegibilityRequired {true};
    bool positionLegibilityRequired {true};
    bool trimCapable {false};
};

[[nodiscard]] constexpr ResolvedKnobModel resolveKnobModel(UiKnobRole role,
                                                           Density density = Density::Standard) noexcept
{
    ResolvedKnobModel model;
    model.publicRole = role;
    model.density = density;

    switch (role)
    {
    case UiKnobRole::Hero:
        model.readoutOwnership = KnobReadoutOwnership::Internal;
        model.compositionPattern = KnobCompositionPattern::HeroStack;
        model.companionReadoutRole = UiReadoutRole::Standard;
        model.internalReadoutAllowed = true;
        model.chromeMayBeSuppressed = false;
        model.companions.caption = false;
        model.trimCapable = true;
        return model;

    case UiKnobRole::Primary:
        model.readoutOwnership = KnobReadoutOwnership::ExternalCompanion;
        model.compositionPattern = KnobCompositionPattern::StandardControlStack;
        model.companionReadoutRole = UiReadoutRole::Standard;
        model.internalReadoutAllowed = false;
        model.chromeMayBeSuppressed = false;
        model.companions.caption = false;
        model.trimCapable = false;
        return model;

    case UiKnobRole::Secondary:
        model.readoutOwnership = KnobReadoutOwnership::ExternalCompanion;
        model.compositionPattern = KnobCompositionPattern::CompactControlStack;
        model.companionReadoutRole = UiReadoutRole::Compact;
        model.internalReadoutAllowed = false;
        model.chromeMayBeSuppressed = true;
        model.companions.caption = true;
        model.trimCapable = false;
        return model;
    }

    return model;
}

[[nodiscard]] inline ResolvedKnobModel resolveKnobModel(UiKnobRole role, const ThemeSnapshot& theme) noexcept
{
    return resolveKnobModel(role, theme.density);
}

[[nodiscard]] constexpr bool usesInternalReadout(const ResolvedKnobModel& model) noexcept
{
    return model.readoutOwnership == KnobReadoutOwnership::Internal;
}

[[nodiscard]] constexpr bool usesExternalCompanionReadout(const ResolvedKnobModel& model) noexcept
{
    return model.readoutOwnership == KnobReadoutOwnership::ExternalCompanion;
}

[[nodiscard]] constexpr ArcGeometryModel resolveArcGeometry(UiKnobRole role) noexcept
{
    (void)role;
    return {};
}

} // namespace bws::ui::kernel

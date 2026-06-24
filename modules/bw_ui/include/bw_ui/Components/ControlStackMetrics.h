// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "bw_ui/adapters/UiLayoutMetrics.h"
#include "bw_ui/adapters/UiThemeKernelAdapter.h"
#include "bw_ui/Components/ComponentSize.h"
#include "bw_ui/foundation/UiTheme.h"

namespace bws::ui::editor
{

struct ControlStackLayoutMetrics
{
    int labelHeight {0};
    int labelGap {0};
    int interactiveHeight {0};
    int readoutHeight {0};
    int totalHeight {0};
};

inline int knobInteractiveSize(UiKnobRole role, float scale, const UiThemeResolved& theme)
{
    const auto metrics = UiKnobs::metrics(role, scale, theme);
    return (int)std::round(metrics.diameter + metrics.hitPadding * 2.0f);
}

inline int knobLabelBlockHeight(UiKnobRole role, float scale, const UiThemeResolved& theme)
{
    const auto metrics = UiKnobs::metrics(role, scale, theme);
    const auto kernelTheme = adapters::makeKernelThemeSnapshot(theme);
    const auto labelHeight = adapters::scaledFontSize(kernelTheme, kernel::TextRole::SectionLabel, scale);
    return (int)std::round(labelHeight + metrics.labelPadding);
}

inline int knobReadoutBlockHeight(UiKnobRole role, float scale, const UiThemeResolved& theme, UiReadoutRole readoutRole)
{
    const auto metrics = UiKnobs::metrics(role, scale, theme);
    const auto readoutMetrics = UiReadouts::metrics(readoutRole, scale, theme);
    return (int)std::round(readoutMetrics.height + metrics.readoutPadding);
}

inline int knobStackHeight(UiKnobRole role, float scale, const UiThemeResolved& theme, UiReadoutRole readoutRole)
{
    return knobLabelBlockHeight(role, scale, theme) + knobInteractiveSize(role, scale, theme) +
           knobReadoutBlockHeight(role, scale, theme, readoutRole);
}

inline int labeledSliderTrackHeight(float scale, const UiThemeResolved& theme)
{
    return (int)std::round(theme.spacing.xl * scale * 0.75f);
}

inline int labeledSliderBlockHeight(float scale, const UiThemeResolved& theme)
{
    const int track = labeledSliderTrackHeight(scale, theme);
    const int padding = (int)std::round(theme.spacing.sm * scale);
    return track + padding * 2;
}

inline int labeledSliderStackHeight(float scale, const UiThemeResolved& theme, UiReadoutRole readoutRole,
                                    UiKnobRole spacingRole = UiKnobRole::Primary)
{
    return knobLabelBlockHeight(spacingRole, scale, theme) + labeledSliderBlockHeight(scale, theme) +
           knobReadoutBlockHeight(spacingRole, scale, theme, readoutRole);
}

inline int knobVariantDiameter(ComponentSize size)
{
    switch (size)
    {
    case ComponentSize::XS:
        return 32;
    case ComponentSize::SM:
        return 48;
    case ComponentSize::MD:
        return 64;
    case ComponentSize::LG:
        return 96;
    case ComponentSize::XL:
        return 128;
    }

    return 64;
}

inline int labeledSliderVariantWidth(ComponentSize size)
{
    switch (size)
    {
    case ComponentSize::XS:
        return 120;
    case ComponentSize::SM:
        return 160;
    case ComponentSize::MD:
        return 200;
    case ComponentSize::LG:
        return 280;
    case ComponentSize::XL:
        return 360;
    }

    return 200;
}

inline ControlStackLayoutMetrics knobStackLayout(UiKnobRole role, float scale, const UiThemeResolved& theme,
                                                 const kernel::ThemeSnapshot& kernelTheme, UiReadoutRole readoutRole,
                                                 bool hasLabel = true)
{
    const auto knobMetrics = UiKnobs::metrics(role, scale, theme);
    const auto readoutMetrics = UiReadouts::metrics(readoutRole, scale, theme);

    ControlStackLayoutMetrics layout;
    layout.labelHeight = hasLabel ? adapters::sectionTextHeight(kernelTheme, kernel::TextRole::SectionLabel, scale) : 0;
    layout.labelGap = hasLabel ? (int)std::round(knobMetrics.labelPadding) : 0;
    layout.interactiveHeight = knobInteractiveSize(role, scale, theme);
    layout.readoutHeight = (int)std::round(readoutMetrics.height);
    const int readoutGap = (int)std::round(knobMetrics.readoutPadding);
    layout.totalHeight =
        layout.labelHeight + layout.labelGap + layout.interactiveHeight + readoutGap + layout.readoutHeight;
    return layout;
}

inline ControlStackLayoutMetrics labeledSliderStackLayout(float scale, const UiThemeResolved& theme,
                                                          const kernel::ThemeSnapshot& kernelTheme,
                                                          UiReadoutRole readoutRole,
                                                          UiKnobRole spacingRole = UiKnobRole::Primary,
                                                          bool hasLabel = true)
{
    const auto knobMetrics = UiKnobs::metrics(spacingRole, scale, theme);
    const auto readoutMetrics = UiReadouts::metrics(readoutRole, scale, theme);

    ControlStackLayoutMetrics layout;
    layout.labelHeight = hasLabel ? adapters::sectionTextHeight(kernelTheme, kernel::TextRole::SectionLabel, scale) : 0;
    layout.labelGap = hasLabel ? (int)std::round(knobMetrics.labelPadding) : 0;
    layout.interactiveHeight = labeledSliderBlockHeight(scale, theme);
    layout.readoutHeight = (int)std::round(readoutMetrics.height);
    const int readoutGap = (int)std::round(knobMetrics.readoutPadding);
    layout.totalHeight =
        layout.labelHeight + layout.labelGap + layout.interactiveHeight + readoutGap + layout.readoutHeight;
    return layout;
}

inline int knobVariantHeight(ComponentSize size, UiKnobRole role, float scale, const UiThemeResolved& theme,
                             const kernel::ThemeSnapshot& kernelTheme, UiReadoutRole readoutRole, bool hasLabel = true)
{
    const auto layout = knobStackLayout(role, scale, theme, kernelTheme, readoutRole, hasLabel);
    const int readoutGap = juce::jmax(0, layout.totalHeight - layout.labelHeight - layout.labelGap -
                                             layout.interactiveHeight - layout.readoutHeight);
    return layout.labelHeight + layout.labelGap + knobVariantDiameter(size) + readoutGap + layout.readoutHeight;
}

inline int labeledSliderVariantHeight(ComponentSize size, float scale, const UiThemeResolved& theme,
                                      const kernel::ThemeSnapshot& kernelTheme, UiReadoutRole readoutRole,
                                      UiKnobRole spacingRole = UiKnobRole::Primary, bool hasLabel = true)
{
    juce::ignoreUnused(size);
    return labeledSliderStackLayout(scale, theme, kernelTheme, readoutRole, spacingRole, hasLabel).totalHeight;
}

inline juce::Rectangle<int> fitStackInTile(juce::Rectangle<int> tile, int desiredW, int desiredH, int paddingPx)
{
    auto area = tile.reduced(paddingPx);
    const int availW = juce::jmax(0, area.getWidth());
    const int availH = juce::jmax(0, area.getHeight());
    const int width = juce::jmin(desiredW, availW);
    const int height = juce::jmin(desiredH, availH);
    return area.withSizeKeepingCentre(width, height);
}

} // namespace bws::ui::editor

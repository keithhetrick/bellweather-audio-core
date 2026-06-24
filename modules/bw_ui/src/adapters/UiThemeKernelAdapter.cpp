// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/adapters/UiThemeKernelAdapter.h"
#include "bw_ui/internal/UiBootstrap.h"

namespace bws::ui::adapters
{
namespace
{
float roleFontSize(const kernel::ThemeSnapshot& theme, kernel::TextRole role, float scale) noexcept
{
    return kernel::textStyle(theme, role).height * juce::jmax(0.5f, theme.densityScale * scale);
}

kernel::TextStyle toKernelTextStyle(const UiTypographySpec& spec) noexcept
{
    return {spec.height, toKernelFontWeight(spec.weight), spec.tracking, spec.uppercase};
}

kernel::ReadoutMetrics toKernelReadoutMetrics(const UiReadoutMetrics& metrics) noexcept
{
    return {metrics.minWidth, metrics.height, metrics.paddingX, metrics.paddingY, 34.0f, 130.0f};
}

juce::Font makeFontFromTextStyle(const kernel::TextStyle& style, float scale, float densityScale)
{
    const float height = style.height * juce::jmax(0.5f, densityScale * scale);
    const bool useTitleFace = style.weight == kernel::FontWeight::SemiBold || style.weight == kernel::FontWeight::Bold;

    bw::Fonts::init();
    UiBootstrap::init();

    auto font = useTitleFace ? bw::Fonts::title(height) : bw::Fonts::body(height);
    font.setExtraKerningFactor(style.tracking);
    return font;
}
} // namespace

kernel::FontWeight toKernelFontWeight(bws::ui::FontWeight weight) noexcept
{
    switch (weight)
    {
    case bws::ui::FontWeight::regular:
        return kernel::FontWeight::Regular;
    case bws::ui::FontWeight::medium:
        return kernel::FontWeight::Medium;
    case bws::ui::FontWeight::semiBold:
        return kernel::FontWeight::SemiBold;
    case bws::ui::FontWeight::bold:
        return kernel::FontWeight::Bold;
    }

    return kernel::FontWeight::Regular;
}

kernel::TextRole toKernelTextRole(UiTypographyRole role) noexcept
{
    switch (role)
    {
    case UiTypographyRole::BrandSmall:
        return kernel::TextRole::BrandSmall;
    case UiTypographyRole::Title:
        return kernel::TextRole::Title;
    case UiTypographyRole::SectionLabel:
        return kernel::TextRole::SectionLabel;
    case UiTypographyRole::ControlText:
        return kernel::TextRole::ControlText;
    case UiTypographyRole::Readout:
        return kernel::TextRole::Readout;
    case UiTypographyRole::Tab:
        return kernel::TextRole::Tab;
    case UiTypographyRole::Overlay:
        return kernel::TextRole::Overlay;
    case UiTypographyRole::HeaderBrand:
        return kernel::TextRole::HeaderBrand;
    case UiTypographyRole::HeaderPreset:
        return kernel::TextRole::HeaderPreset;
    case UiTypographyRole::Annotation:
        return kernel::TextRole::Annotation;
    }

    return kernel::TextRole::ControlText;
}

kernel::Density toKernelDensity(UiDensity density) noexcept
{
    return density == UiDensity::Compact ? kernel::Density::Compact : kernel::Density::Standard;
}

kernel::ThemeSnapshot makeKernelThemeSnapshot(const UiThemeResolved& theme,
                                              kernel::ProductThemeId productTheme) noexcept
{
    kernel::ThemeSnapshot snapshot;
    snapshot.productTheme = productTheme;
    snapshot.density = toKernelDensity(theme.density);
    snapshot.densityScale = theme.densityScale;
    snapshot.spacing = {theme.spacing.xxs, theme.spacing.xs, theme.spacing.sm, theme.spacing.md,
                        theme.spacing.lg,  theme.spacing.xl, theme.spacing.xxl};
    snapshot.headerSpacing = {theme.header.brandZoneWidth,
                              theme.header.iconSize,
                              theme.header.iconPadding,
                              theme.header.nameIconGap,
                              theme.header.nameRightPad,
                              theme.header.nameCategoryGap,
                              theme.header.funcLeftOffset,
                              theme.header.navWidth,
                              theme.header.saveWidth,
                              theme.header.abWidth,
                              theme.header.settingsWidth,
                              theme.header.rightInset,
                              theme.header.gap,
                              theme.header.renamePadV};
    snapshot.readouts = {toKernelReadoutMetrics(theme.readouts.standard),
                         toKernelReadoutMetrics(theme.readouts.compact)};

    snapshot.textStyles[kernel::toIndex(kernel::TextRole::BrandSmall)] = toKernelTextStyle(theme.typography.brandSmall);
    snapshot.textStyles[kernel::toIndex(kernel::TextRole::Title)] = toKernelTextStyle(theme.typography.title);
    snapshot.textStyles[kernel::toIndex(kernel::TextRole::SectionLabel)] =
        toKernelTextStyle(theme.typography.sectionLabel);
    snapshot.textStyles[kernel::toIndex(kernel::TextRole::ControlText)] =
        toKernelTextStyle(theme.typography.controlText);
    snapshot.textStyles[kernel::toIndex(kernel::TextRole::Readout)] = toKernelTextStyle(theme.typography.readout);
    snapshot.textStyles[kernel::toIndex(kernel::TextRole::Tab)] = toKernelTextStyle(theme.typography.tab);
    snapshot.textStyles[kernel::toIndex(kernel::TextRole::Overlay)] = toKernelTextStyle(theme.typography.overlay);
    snapshot.textStyles[kernel::toIndex(kernel::TextRole::HeaderBrand)] =
        toKernelTextStyle(theme.typography.headerBrand);
    snapshot.textStyles[kernel::toIndex(kernel::TextRole::HeaderPreset)] =
        toKernelTextStyle(theme.typography.headerPreset);
    snapshot.textStyles[kernel::toIndex(kernel::TextRole::Annotation)] = toKernelTextStyle(theme.typography.annotation);

    return snapshot;
}

rendering::FontWeight toRendererFontWeight(kernel::FontWeight weight) noexcept
{
    return weight == kernel::FontWeight::Bold       ? rendering::FontWeight::Bold
           : weight == kernel::FontWeight::SemiBold ? rendering::FontWeight::Bold
                                                    : rendering::FontWeight::Regular;
}

float scaledFontSize(const kernel::ThemeSnapshot& theme, kernel::TextRole role, float scale) noexcept
{
    return roleFontSize(theme, role, scale);
}

juce::Font makeFont(const kernel::ThemeSnapshot& theme, kernel::TextRole role, float scale)
{
    return makeFontFromTextStyle(kernel::textStyle(theme, role), scale, theme.densityScale);
}

juce::Font fontFittedToHeight(const kernel::ThemeSnapshot& theme, kernel::TextRole role, float targetHeight)
{
    const float baseHeight = scaledFontSize(theme, role, 1.0f);
    const float scale = targetHeight / juce::jmax(1.0f, baseHeight);
    return makeFont(theme, role, scale);
}

void applyGraphicsFont(juce::Graphics& graphics, const kernel::ThemeSnapshot& theme, kernel::TextRole role, float scale)
{
    graphics.setFont(makeFont(theme, role, scale));
}

void applyRendererFont(rendering::IRenderer& renderer, const kernel::ThemeSnapshot& theme, kernel::TextRole role,
                       float scale)
{
    renderer.setFont(roleFontSize(theme, role, scale), toRendererFontWeight(kernel::textStyle(theme, role).weight));
}

juce::String applyTextCasing(const kernel::ThemeSnapshot& theme, kernel::TextRole role, const juce::String& text)
{
    juce::ignoreUnused(theme);
    return kernel::textStyle(theme, role).uppercase ? text.toUpperCase() : text;
}

} // namespace bws::ui::adapters

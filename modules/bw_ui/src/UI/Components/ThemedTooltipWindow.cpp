// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/Components/ThemedTooltipWindow.h"

#include "bw_ui/adapters/UiThemeKernelAdapter.h"
#include "bw_ui/generated/BwTokens.h"

#include <algorithm>

namespace bws::ui
{
namespace
{
juce::TextLayout makeTooltipLayout(const juce::String& text, const kernel::ThemeSnapshot& kernelTheme,
                                   const UiThemeResolved& theme, const UiTooltipMetrics& metrics, float scale,
                                   float maxWidth)
{
    juce::AttributedString attr;
    attr.setJustification(juce::Justification::topLeft);
    attr.append(text, adapters::makeFont(kernelTheme, kernel::TextRole::Annotation, scale * metrics.fontScale),
                theme.colors.text0);

    juce::TextLayout layout;
    layout.createLayoutWithBalancedLineLengths(attr, maxWidth);
    return layout;
}
} // namespace

struct ThemedTooltipWindow::Impl
{
    struct LookAndFeel final : public juce::LookAndFeel_V4
    {
        explicit LookAndFeel(const UiThemeResolved& initialTheme)
            : theme(initialTheme)
            , kernelTheme(adapters::makeKernelThemeSnapshot(initialTheme))
        {}

        void setTheme(const UiThemeResolved& newTheme)
        {
            theme = newTheme;
            kernelTheme = adapters::makeKernelThemeSnapshot(newTheme);
        }

        void setScale(float newScale) noexcept { scale = newScale; }

        juce::Rectangle<int> getTooltipBounds(const juce::String& tipText, juce::Point<int> screenPos,
                                              juce::Rectangle<int> parentArea) override
        {
            const auto metrics = UiTooltips::metrics(scale, theme);
            const float maxWidth = std::clamp(static_cast<float>(parentArea.getWidth()) * metrics.maxWidthRatio,
                                              metrics.minWidth, metrics.maxWidth);
            const auto layout = makeTooltipLayout(tipText, kernelTheme, theme, metrics, scale, maxWidth);

            const int padX = static_cast<int>(std::lround(metrics.paddingX));
            const int padY = static_cast<int>(std::lround(metrics.paddingY));
            const int width = static_cast<int>(std::ceil(layout.getWidth())) + padX * 2;
            const int height = static_cast<int>(std::ceil(layout.getHeight())) + padY * 2;
            const int xOffset = static_cast<int>(std::lround(metrics.offsetX));
            const int yOffset = static_cast<int>(std::lround(metrics.offsetY));

            return juce::Rectangle<int>(
                       screenPos.x > parentArea.getCentreX() ? screenPos.x - (width + xOffset) : screenPos.x + xOffset,
                       screenPos.y > parentArea.getCentreY() ? screenPos.y - (height + yOffset) : screenPos.y + yOffset,
                       width, height)
                .constrainedWithin(parentArea.reduced(static_cast<int>(std::lround(metrics.parentInset))));
        }

        void drawTooltip(juce::Graphics& g, const juce::String& text, int width, int height) override
        {
            const auto metrics = UiTooltips::metrics(scale, theme);
            const auto bounds =
                juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height))
                    .reduced(bws::tokens::shared::geometry::STROKE_HALF_PX);
            const float cornerRadius = theme.surfaces.radiusSm * metrics.cornerRadiusScale;
            juce::Path shellPath;
            shellPath.addRoundedRectangle(bounds, cornerRadius);

            juce::DropShadow(theme.colors.bg0.withAlpha(metrics.shadowAlpha),
                             static_cast<int>(std::lround(metrics.shadowBlur)),
                             {0, static_cast<int>(std::lround(metrics.shadowOffsetY))})
                .drawForPath(g, shellPath);

            g.setColour(theme.colors.bg1);
            g.fillPath(shellPath);

            g.setColour(theme.colors.outline0.withAlpha(bws::tokens::shared::opacity::outline::DIVIDER));
            g.strokePath(shellPath, juce::PathStrokeType(theme.surfaces.strokeHairline));

            const int padX = static_cast<int>(std::lround(metrics.paddingX));
            const int padY = static_cast<int>(std::lround(metrics.paddingY));
            auto layout = makeTooltipLayout(text, kernelTheme, theme, metrics, scale,
                                            std::max(metrics.minTextWidth, static_cast<float>(width - padX * 2)));
            layout.draw(g, bounds.reduced(static_cast<float>(padX), static_cast<float>(padY)));
        }

        UiThemeResolved theme;
        kernel::ThemeSnapshot kernelTheme;
        float scale {1.0f};
    };

    explicit Impl(const UiThemeResolved& theme)
        : lookAndFeel(theme)
    {}

    LookAndFeel lookAndFeel;
};

ThemedTooltipWindow::ThemedTooltipWindow(juce::Component* parentComp, const UiThemeResolved& theme, int delayMs)
    : juce::TooltipWindow(parentComp, delayMs)
    , impl_(std::make_unique<Impl>(theme))
{
    setOpaque(false);
    setLookAndFeel(&impl_->lookAndFeel);
}

ThemedTooltipWindow::~ThemedTooltipWindow()
{
    setLookAndFeel(nullptr);
}

void ThemedTooltipWindow::setTheme(const UiThemeResolved& newTheme)
{
    impl_->lookAndFeel.setTheme(newTheme);
    repaint();
}

void ThemedTooltipWindow::setScale(float newScale) noexcept
{
    impl_->lookAndFeel.setScale(newScale);
    repaint();
}

} // namespace bws::ui

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * @file AboutOverlay.h
 * @brief Shared "About" overlay for Weather Design System plugins.
 *
 * Renders a dark translucent backdrop with a styled card showing
 * plugin name, version, license status, description, and studio branding.
 * Hosts create this overlay from their settings or about-menu affordance.
 *
 * AboutConfig is immutable after construction - set all fields before
 * creating the overlay. The card height adjusts automatically when
 * licenseStatus is provided.
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include "bw_ui/adapters/UiThemeKernelAdapter.h"
#include "bw_ui/foundation/Fonts.h"
#include "bw_ui/foundation/UiTheme.h"
#include "bw_ui/generated/BwTokens.h"

namespace bws::weather
{

struct AboutConfig
{
    juce::String pluginName;
    juce::String version;       // e.g. JucePlugin_VersionString
    juce::String description;   // newline-separated lines
    juce::String manualUrl;     // opened in default browser
    juce::String licenseStatus; // "Free", "Licensed", "Trial" - empty to hide
    juce::String dismissLabel;  // empty falls back to OK
};

class AboutOverlay : public juce::Component
{
public:
    explicit AboutOverlay(const AboutConfig& config, std::function<void()> onDismiss)
        : config_(config)
        , dismissCallback_(std::move(onDismiss))
    {
        closeButton_.setButtonText(config_.dismissLabel.isNotEmpty() ? config_.dismissLabel : juce::String("OK"));
        closeButton_.onClick = [this]() {
            dismiss();
        };
        addAndMakeVisible(closeButton_);
        setWantsKeyboardFocus(true);
    }

    void paint(juce::Graphics& g) override
    {
        static const auto kDefault = bws::ui::resolveTheme(bws::ui::defaultUiThemeBase(), {});
        const auto& t = theme_ ? *theme_ : kDefault;
        const auto kernelTheme = theme_ ? bws::ui::adapters::makeKernelThemeSnapshot(t)
                                        : bws::ui::adapters::makeKernelThemeSnapshot(kDefault);

        // Translucent backdrop
        g.fillAll(juce::Colours::black.withAlpha(bws::tokens::shared::opacity::dialog::BACKDROP));

        auto card = getCardBounds();

        // Card background
        g.setColour(juce::Colour(bws::tokens::shared::brass::colors::BG_ELEVATED));
        g.fillRoundedRectangle(card, 8.0f);
        g.setColour(juce::Colour(bws::tokens::shared::brass::colors::BORDER_DARK));
        g.drawRoundedRectangle(card, 8.0f, 1.0f);

        // Brass accent line at top
        g.setColour(juce::Colour(bws::tokens::shared::brass::colors::ACCENT)
                        .withAlpha(bws::tokens::shared::opacity::about_overlay::ACCENT_BORDER));
        g.fillRoundedRectangle(card.getX() + 40.0f, card.getY(), card.getWidth() - 80.0f, 2.0f, 1.0f);

        float y = card.getY() + 24.0f;

        // Plugin name
        g.setFont(bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::Title, 1.0f));
        g.setColour(juce::Colour(bws::tokens::shared::brass::colors::TEXT_PRIMARY));
        g.drawText(config_.pluginName, card.withY(y).withHeight(28.0f), juce::Justification::centred);
        y += 30.0f;

        // Version
        g.setFont(bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::Readout, 1.0f));
        g.setColour(juce::Colour(bws::tokens::shared::brass::colors::ACCENT));
        g.drawText("v" + config_.version, card.withY(y).withHeight(18.0f), juce::Justification::centred);
        y += 22.0f;

        // License status (optional - rendered only when set)
        if (config_.licenseStatus.isNotEmpty())
        {
            g.setFont(bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::Annotation, 1.0f));
            g.setColour(juce::Colour(bws::tokens::shared::brass::colors::TEXT_TERTIARY));
            g.drawText(config_.licenseStatus, card.withY(y).withHeight(14.0f), juce::Justification::centred);
            y += 18.0f;
        }
        else
        {
            y += 6.0f; // Restore original spacing when no status
        }

        // Divider
        g.setColour(juce::Colour(bws::tokens::shared::brass::colors::BORDER_DARK));
        g.fillRect(card.getX() + 30.0f, y, card.getWidth() - 60.0f, 1.0f);
        y += 16.0f;

        // Description
        g.setFont(bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::ControlText, 1.0f));
        g.setColour(juce::Colour(bws::tokens::shared::brass::colors::TEXT_SECONDARY));
        g.drawFittedText(config_.description,
                         juce::Rectangle<float>(card.getX() + 24.0f, y, card.getWidth() - 48.0f, 60.0f).toNearestInt(),
                         juce::Justification::centred, 4);
        y += 68.0f;

        // Studio branding
        g.setFont(bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::BrandSmall, 1.0f));
        g.setColour(juce::Colour(bws::tokens::shared::brass::colors::TEXT_TERTIARY));
        g.drawText("Bellweather Studios", card.withY(y).withHeight(16.0f), juce::Justification::centred);
    }

    void resized() override
    {
        auto card = getCardBounds().toNearestInt();
        closeButton_.setBounds(card.getCentreX() - 50, card.getBottom() - 48, 100, 30);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        // Click outside card dismisses
        if (!getCardBounds().contains(e.position))
            dismiss();
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::escapeKey || key == juce::KeyPress::returnKey)
        {
            dismiss();
            return true;
        }
        return false;
    }

private:
    juce::Rectangle<float> getCardBounds() const
    {
        constexpr int cardW = 320;
        const int cardH = config_.licenseStatus.isEmpty() ? 280 : 300;
        return juce::Rectangle<int>((getWidth() - cardW) / 2, (getHeight() - cardH) / 2, cardW, cardH).toFloat();
    }

    void dismiss()
    {
        if (dismissCallback_)
            dismissCallback_();
    }

    const AboutConfig config_;
    juce::TextButton closeButton_;
    std::function<void()> dismissCallback_;

public:
    void setTheme(const bws::ui::UiThemeResolved& t) { theme_ = &t; }

private:
    const bws::ui::UiThemeResolved* theme_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutOverlay)
};

} // namespace bws::weather

// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * =============================================================================
 * ResizableEditor.h - Resizable Editor Base Class for Weather Instrument Suite
 * =============================================================================
 *
 * Bellweather Studios - Weather Instrument Design System
 *
 * Provides universal resize/scale/fullscreen support for all Weather plugins:
 *   - Size presets (Medium/Large/ExtraLarge)
 *   - Scaling submenu (75%-200%)
 *   - Full-screen mode with ESC to exit
 *   - Preference persistence (per monitor type)
 *   - Resize corner indicator
 *   - Full-screen button
 *
 * Usage:
 *   class MyPluginEditor : public bws::weather::ResizableEditor
 *   {
 *   public:
 * MyPluginEditor(Processor& p)
 *           : ResizableEditor(p, getBasePluginSize()) { }
 *
 *       juce::Rectangle<int> getBasePluginSize() const override
 *       {
 *           return { 0, 0, 400, 500 };
 *       }
 *
 *   protected:
 *       void layoutComponents() override
 *       {
 *           // Layout your components using scaled() for dimensions
 *       }
 *   };
 *
 * =============================================================================
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>
#include <limits>
#include <optional>
#include "bw_ui/Components/ThemedPopupMenuLookAndFeel.h"
#include "bw_ui/foundation/UiTheme.h"
#include "bw_ui/windowing/HostedWindowGeometry.h"

namespace bws
{
namespace weather
{

/**
 * Size presets for Weather plugins.
 * Shared named sizes for Weather plugins.
 */
enum class SizePreset
{
    Medium,    // Default reference: 600x720 (1.0x)
    Large,     // 900x1080 (1.5x of Medium)
    ExtraLarge // 1200x1440 (2.0x of Medium)
};

/**
 * @brief Resizable editor base class for Weather Instrument plugins.
 *
 * Provides FabFilter-style resize and scale functionality:
 * - Size presets via right-click menu or resize corner
 * - Scaling from 75% to 200%
 * - Full-screen mode (ESC to exit)
 * - Persistent preferences per plugin
 */
class ResizableEditor
    : public juce::AudioProcessorEditor
    , public juce::KeyListener
{
public:
    /**
     * Constructor.
     * @param processor The audio processor
     * @param baseSize The plugin's base size at 100% scale
     */
    ResizableEditor(juce::AudioProcessor& processor, juce::Rectangle<int> baseSize);

    ~ResizableEditor() override;

    // =========================================================================
    // Size and Scale API
    // =========================================================================

    /** Set size from preset */
    void setSizePreset(SizePreset preset);

    /** Get current size preset */
    SizePreset getCurrentPreset() const { return currentPreset_; }

    /** Set scaling factor (75% to 200% in windowed mode; fullscreen can exceed this) */
    void setScaleFactor(float scale) override;

    /** Get current scale factor */
    float getScaleFactor() const { return scaleFactor_; }

    /** Chrome scale: min(windowScale, cap). Default cap +inf ⇒ == windowScale. */
    float getChromeScaleFactor() const { return juce::jmin(scaleFactor_, chromeScaleCap_); }

    /** Set the chrome-scale cap. */
    void setChromeScaleCap(float cap)
    {
        chromeScaleCap_ = cap;
        resized();
    }

    /** Round baseValue at chrome scale. */
    int chromeScaled(int baseValue) const
    {
        return juce::roundToInt(static_cast<float>(baseValue) * getChromeScaleFactor());
    }

    // =========================================================================
    // Theme Access
    // =========================================================================

    /** Get the resolved UI theme. Const ref - components must not store a copy. */
    const bws::ui::UiThemeResolved& getTheme() const { return theme_; }

    /** Set the resolved theme. Calls onThemeChanged(). */
    void setTheme(const bws::ui::UiThemeResolved& theme)
    {
        theme_ = theme;
        if (resizeMenuLookAndFeel_)
        {
            resizeMenuLookAndFeel_->setTheme(theme_);
            resizeMenuLookAndFeel_->setScale(scaleFactor_);
        }
        onThemeChanged();
    }

    /** Toggle full-screen mode */
    void toggleFullScreen();

    /** Check if in full-screen mode */
    bool isFullScreen() const { return isFullScreen_; }

    // =========================================================================
    // Base Size (Override in Derived Classes)
    // =========================================================================

    /**
     * Override to define your plugin's base size at 1.0 scale.
     * This is the live source of truth for all scaling, fullscreen, and
     * size-preset calculations.  The default returns the construction-time
     * baseSize_.  Plugins with dynamic content height (e.g. collapsible
     * sections) MUST override this to return the current dimensions.
     */
    virtual juce::Rectangle<int> getBasePluginSize() const { return baseSize_; }

    // =========================================================================
    // juce::Component Overrides
    // =========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;
    void parentHierarchyChanged() override;

    // juce::KeyListener override (bring base class method into scope)
    using juce::Component::keyPressed;
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;

protected:
    // =========================================================================
    // Protected API for Derived Classes
    // =========================================================================

    /** Scale an integer dimension by the current scale factor */
    int scaled(int baseValue) const { return juce::roundToInt(static_cast<float>(baseValue) * scaleFactor_); }

    /** Scale a float dimension by the current scale factor */
    float scaledF(float baseValue) const { return baseValue * scaleFactor_; }

    /** Get the scaled plugin size */
    juce::Rectangle<int> getScaledSize() const;

    /**
     * Override to layout your components.
     * Called after resize. Use scaled() for all dimensions.
     * Default implementation is empty to allow safe construction.
     */
    virtual void layoutComponents() {}

    /**
     * Override to paint your plugin content.
     * Called from paint() after background is drawn.
     */
    virtual void paintContent(juce::Graphics& /*g*/) {}

    /**
     * Override to get the plugin name for preferences storage.
     * Default returns "Plugin".
     */
    virtual juce::String getPluginName() const { return "WeatherPlugin"; }

    /**
     * Callback triggered when scale changes.
     * Override to update scale-dependent resources.
     */
    virtual void onScaleChanged() {}

    /**
     * Callback triggered when theme changes.
     * Override to update theme-dependent resources.
     * Default implementation calls repaint().
     */
    virtual void onThemeChanged() { repaint(); }

    /** Show or hide the fullscreen button. Hidden by default for plugins
     *  that manage fullscreen via their own settings menu. */
    void setFullScreenButtonVisible(bool visible);

    // =========================================================================
    // Preference Persistence
    // =========================================================================

    /** Save current size/scale preferences */
    void savePreferences();

    /** Load saved size/scale preferences */
    void loadPreferences();

private:
    // =========================================================================
    // Internal Components
    // =========================================================================

    class ResizeCornerButton;
    class FullScreenButton;

    std::unique_ptr<ResizeCornerButton> resizeCorner_;
    std::unique_ptr<FullScreenButton> fullScreenButton_;
    std::unique_ptr<juce::ComponentBoundsConstrainer> constrainer_;
    std::unique_ptr<bws::ui::ThemedPopupMenuLookAndFeel> resizeMenuLookAndFeel_;

    // =========================================================================
    // State
    // =========================================================================

    juce::Rectangle<int> baseSize_; // construction-time default; getBasePluginSize() is the live truth
    SizePreset currentPreset_ = SizePreset::Medium;
    float scaleFactor_ = 1.0f;
    float chromeScaleCap_ = std::numeric_limits<float>::infinity();
    bws::ui::UiThemeResolved theme_;
    bool isFullScreen_ = false;

    // Saved state for exiting full-screen
    juce::Rectangle<int> normalBounds_;
    float normalScaleFactor_ = 1.0f;
    std::optional<bws::ui::windowing::HostedWindowSessionState> hostedWindowSessionState_;

    // =========================================================================
    // Internal Methods
    // =========================================================================

    void showResizeMenu();
    void enterFullScreen();
    void exitFullScreen();

    float getScaleForPreset(SizePreset preset) const;
    SizePreset getPresetForScale(float scale) const;

    juce::PropertiesFile* getPropertiesFile();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResizableEditor)
};

// =============================================================================
// ResizeCornerButton - Bottom-Right Resize Indicator
// =============================================================================

/**
 * Custom resize corner with Weather aesthetic.
 * Draws a brass-colored resize indicator and shows menu on click.
 */
class ResizableEditor::ResizeCornerButton : public juce::Component
{
public:
    explicit ResizeCornerButton(ResizableEditor& editor);
    ~ResizeCornerButton() override = default;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

private:
    ResizableEditor& editor_;
    bool isHovering_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResizeCornerButton)
};

// =============================================================================
// FullScreenButton - Top-Right Full-Screen Toggle
// =============================================================================

/**
 * Full-screen toggle button with expand/collapse icon.
 */
class ResizableEditor::FullScreenButton
    : public juce::Component
    , public juce::SettableTooltipClient
{
public:
    explicit FullScreenButton(ResizableEditor& editor);
    ~FullScreenButton() override = default;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

    void updateTooltip();

private:
    ResizableEditor& editor_;
    bool isHovering_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FullScreenButton)
};

} // namespace weather
} // namespace bws

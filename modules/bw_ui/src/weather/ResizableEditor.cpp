// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

/**
 * =============================================================================
 * ResizableEditor.cpp - Implementation
 * =============================================================================
 */

#include "bw_ui/weather/containers/ResizableEditor.h"
#include <bw_ui/rendering/JuceRenderer.h>
#include "bw_ui/foundation/UiTheme.h"
#include <bw_ui/windowing/HostedWindowGeometry.h>

namespace bws
{
namespace weather
{

namespace
{
juce::Rectangle<int> availableDisplayAreaFor(const juce::Component& component)
{
    if (auto* display = juce::Desktop::getInstance().getDisplays().getDisplayForRect(component.getScreenBounds()))
        return display->userArea;

    if (auto* primary = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        return primary->userArea;

    return {0, 0, 1920, 1080};
}

#if JUCE_WINDOWS
void selectSoftwareRenderer(juce::ComponentPeer& peer)
{
    const auto engines = peer.getAvailableRenderingEngines();
    for (int i = 0; i < engines.size(); ++i)
    {
        if (engines[i].containsIgnoreCase("Software"))
        {
            peer.setCurrentRenderingEngine(i);
            return;
        }
    }
}

void selectHostedSoftwareRenderer(juce::Component& component)
{
    if (auto* peer = component.getPeer())
    {
        selectSoftwareRenderer(*peer);
        return;
    }

    if (auto* peer = component.getTopLevelComponent()->getPeer())
        selectSoftwareRenderer(*peer);
}
#endif
} // namespace

// =============================================================================
// ResizableEditor Implementation
// =============================================================================

ResizableEditor::ResizableEditor(juce::AudioProcessor& processor, juce::Rectangle<int> baseSize)
    : juce::AudioProcessorEditor(processor)
    , baseSize_(baseSize)
    , theme_(bws::ui::resolveTheme(bws::ui::defaultUiThemeBase(), {}))
{
    // Create bounds constrainer with initial limits from baseSize_.
    // Derived classes with dynamic heights (e.g. Barometer's LUFS graph)
    // will re-initialize these in their constructor body.
    constrainer_ = std::make_unique<juce::ComponentBoundsConstrainer>();

    const int minWidth = juce::roundToInt(baseSize_.getWidth() * 0.5f);
    const int minHeight = juce::roundToInt(baseSize_.getHeight() * 0.5f);
    const int maxWidth = juce::roundToInt(baseSize_.getWidth() * 3.0f);
    const int maxHeight = juce::roundToInt(baseSize_.getHeight() * 3.0f);

    constrainer_->setSizeLimits(minWidth, minHeight, maxWidth, maxHeight);
    constrainer_->setFixedAspectRatio(static_cast<double>(baseSize_.getWidth()) /
                                      static_cast<double>(baseSize_.getHeight()));

    // Create resize corner
    resizeCorner_ = std::make_unique<ResizeCornerButton>(*this);
    addAndMakeVisible(resizeCorner_.get());

    // Create full-screen button
    fullScreenButton_ = std::make_unique<FullScreenButton>(*this);
    addAndMakeVisible(fullScreenButton_.get());

    resizeMenuLookAndFeel_ =
        std::make_unique<bws::ui::ThemedPopupMenuLookAndFeel>(theme_, bws::ui::UiDropdownRole::UtilityMenu);
    resizeMenuLookAndFeel_->setScale(scaleFactor_);

    // Add keyboard listener
    addKeyListener(this);

    // Declare that this editor wants keyboard focus so hosts (Reaper, Pro
    // Tools, Live) route key events to the plugin window instead of capturing
    // them for transport shortcuts. Without this, spacebar typed into a
    // preset dialog's text field leaks to the host and triggers play/stop.
    // Benefits every plugin inheriting this base - not just dialog flows.
    setWantsKeyboardFocus(true);

    // Load saved preferences
    loadPreferences();
    if (resizeMenuLookAndFeel_)
        resizeMenuLookAndFeel_->setScale(scaleFactor_);

    // Set initial size
    const auto scaledSize = getScaledSize();
    setSize(scaledSize.getWidth(), scaledSize.getHeight());
}

ResizableEditor::~ResizableEditor()
{
    removeKeyListener(this);
    if (isFullScreen_)
        scaleFactor_ = normalScaleFactor_;
    savePreferences();
}

void ResizableEditor::setFullScreenButtonVisible(bool visible)
{
    if (fullScreenButton_)
        fullScreenButton_->setVisible(visible);
}

// =============================================================================
// Size and Scale API
// =============================================================================

void ResizableEditor::setSizePreset(SizePreset preset)
{
    currentPreset_ = preset;
    setScaleFactor(getScaleForPreset(preset));
}

void ResizableEditor::setScaleFactor(float scale)
{
    // Clamp to valid range
    scale = juce::jlimit(0.75f, 2.0f, scale);

    if (std::abs(scaleFactor_ - scale) < 0.001f)
        return;

    scaleFactor_ = scale;
    currentPreset_ = getPresetForScale(scale);

    if (resizeMenuLookAndFeel_)
        resizeMenuLookAndFeel_->setScale(scaleFactor_);

    // Update size - but not in fullscreen, where kiosk mode owns the bounds.
    // The caller is responsible for triggering resized() in fullscreen.
    if (!isFullScreen_)
    {
        const auto newSize = getScaledSize();
        setSize(newSize.getWidth(), newSize.getHeight());
    }

    // Notify derived class
    onScaleChanged();

    // Force a full repaint - setSize() alone may not repaint children
    // when the window bounds change triggers are batched or deferred.
    repaint();

    // Save preferences - but not during fullscreen, where the scale is
    // transient.  exitFullScreen() persists the restored value instead.
    if (!isFullScreen_)
        savePreferences();
}

void ResizableEditor::toggleFullScreen()
{
    if (isFullScreen_)
        exitFullScreen();
    else
        enterFullScreen();
}

juce::Rectangle<int> ResizableEditor::getScaledSize() const
{
    const auto base = getBasePluginSize();
    return {0, 0, juce::roundToInt(base.getWidth() * scaleFactor_), juce::roundToInt(base.getHeight() * scaleFactor_)};
}

// =============================================================================
// juce::Component Overrides
// =============================================================================

void ResizableEditor::paint(juce::Graphics& g)
{
    // Draw background
    bws::ui::rendering::JuceRenderer renderer(g);
    renderer.setColour(theme_.weatherColors.bgMain.getARGB());
    auto bounds = getLocalBounds().toFloat();
    renderer.fillRect(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight());

    // Let derived class paint content (stays on raw g - subclasses migrate independently)
    paintContent(g);
}

void ResizableEditor::resized()
{
#if JUCE_WINDOWS
    selectHostedSoftwareRenderer(*this);
#endif

    // In fullscreen, derive scale from the ACTUAL window bounds so it always
    // matches reality - even if kiosk mode failed or the OS resized us.
    if (isFullScreen_)
    {
        const auto base = getBasePluginSize();
        const float wScale = static_cast<float>(getWidth()) / static_cast<float>(base.getWidth());
        const float hScale = static_cast<float>(getHeight()) / static_cast<float>(base.getHeight());
        scaleFactor_ = std::min(wScale, hScale);

        // Safety: if the window is clearly not fullscreen-sized (e.g. kiosk
        // mode was externally cancelled), bail out to the normal state.
        auto* display = juce::Desktop::getInstance().getDisplays().getDisplayForPoint(getScreenBounds().getCentre());
        if (display && getWidth() < display->userArea.getWidth() / 2)
        {
            exitFullScreen();
            return;
        }
    }

    const int cornerSize = scaled(20);
    const int buttonSize = scaled(18);
    const int margin = scaled(8);

    // Position resize corner (bottom-right)
    resizeCorner_->setBounds(getWidth() - cornerSize, getHeight() - cornerSize, cornerSize, cornerSize);

    // Position full-screen button (top-right)
    fullScreenButton_->setBounds(getWidth() - buttonSize - margin, margin, buttonSize, buttonSize);

    // Let derived class layout components
    layoutComponents();

    if (!isFullScreen_)
        bws::ui::windowing::clampHostedWindowToVisibleFrame(*this);
}

void ResizableEditor::visibilityChanged()
{
    juce::AudioProcessorEditor::visibilityChanged();

#if JUCE_WINDOWS
    selectHostedSoftwareRenderer(*this);
#endif
}

void ResizableEditor::parentHierarchyChanged()
{
    juce::AudioProcessorEditor::parentHierarchyChanged();

#if JUCE_WINDOWS
    selectHostedSoftwareRenderer(*this);
#endif

    // Update full-screen button tooltip based on parent
    if (fullScreenButton_)
        fullScreenButton_->updateTooltip();

    if (!isFullScreen_)
    {
        bws::ui::windowing::settleHostedWindowPlacement(*this);
    }
}

bool ResizableEditor::keyPressed(const juce::KeyPress& key, juce::Component* /*originatingComponent*/)
{
    // ESC exits full-screen mode
    if (isFullScreen_ && key == juce::KeyPress::escapeKey)
    {
        exitFullScreen();
        return true;
    }

    // Cmd/Ctrl+0 resets to 100% scale
    if (key.getModifiers().isCommandDown() && key.getKeyCode() == '0')
    {
        setScaleFactor(1.0f);
        return true;
    }

    // Cmd/Ctrl+Plus zooms in
    if (key.getModifiers().isCommandDown() && (key.getKeyCode() == '+' || key.getKeyCode() == '='))
    {
        setScaleFactor(scaleFactor_ + 0.25f);
        return true;
    }

    // Cmd/Ctrl+Minus zooms out
    if (key.getModifiers().isCommandDown() && key.getKeyCode() == '-')
    {
        setScaleFactor(scaleFactor_ - 0.25f);
        return true;
    }

    return false;
}

// =============================================================================
// Full-Screen Mode
// =============================================================================

void ResizableEditor::enterFullScreen()
{
    if (isFullScreen_)
        return;

    // Save current state
    normalBounds_ = getBounds();
    normalScaleFactor_ = scaleFactor_;
    hostedWindowSessionState_ = std::nullopt;

    isFullScreen_ = true;

    // Enter kiosk mode - this will resize the component to fill the screen,
    // triggering resized() which derives the scale from actual window bounds.
    // We intentionally do NOT precompute scaleFactor_ here; resized() handles
    // it so the scale always matches reality even if kiosk mode fails.
    juce::Desktop::getInstance().setKioskModeComponent(this);

    // Hosted plugin windows carry native frame chrome that JUCE's editor-view
    // bounds do not account for. Fit the native host window to the visible
    // frame first; fall back to view-sized bounds only when no hosted window
    // geometry is available.
    hostedWindowSessionState_ = bws::ui::windowing::enterHostedFullscreenSession(*this);
    if (!hostedWindowSessionState_.has_value())
    {
        const auto fullscreenBounds = availableDisplayAreaFor(*this);
        if (!fullscreenBounds.isEmpty())
            setBounds(fullscreenBounds);
    }

    // Update button tooltip
    if (fullScreenButton_)
        fullScreenButton_->updateTooltip();

    // Notify derived class
    onScaleChanged();

    // Force layout update (resized may have already fired from
    // setKioskModeComponent, but call again to be safe)
    resized();
}

void ResizableEditor::exitFullScreen()
{
    if (!isFullScreen_)
        return;

    isFullScreen_ = false;

    // Exit kiosk mode
    juce::Desktop::getInstance().setKioskModeComponent(nullptr);

    // Restore saved scale, but recompute size from the CURRENT base dimensions.
    // The base size may have changed while in fullscreen (e.g. Barometer's LUFS
    // graph toggle), so normalBounds_ dimensions could be stale.
    scaleFactor_ = normalScaleFactor_;
    currentPreset_ = getPresetForScale(scaleFactor_);
    const auto newSize = getScaledSize(); // uses current getBasePluginSize()
    if (hostedWindowSessionState_.has_value())
    {
        // In hosted mode, Bellweather's native window module is the sole
        // decider for restored outer-frame position. ResizableEditor restores
        // content size/scale only and leaves x/y to HostedWindowGeometry*.
        setSize(newSize.getWidth(), newSize.getHeight());
        bws::ui::windowing::exitHostedFullscreenSession(*this, *hostedWindowSessionState_);
    }
    else
    {
        setBounds(normalBounds_.getX(), normalBounds_.getY(), newSize.getWidth(), newSize.getHeight());
    }

    // Persist the restored scale so it survives editor reload.
    // The fullscreen path may have overwritten this via setScaleFactor().
    savePreferences();

    // Update button tooltip
    if (fullScreenButton_)
        fullScreenButton_->updateTooltip();

    // Notify derived class and ensure layout is updated
    onScaleChanged();

    // Force a resized() + repaint() to ensure layoutComponents() runs
    // and all children redraw at the restored scale.
    resized();
    repaint();

    hostedWindowSessionState_.reset();
}

// =============================================================================
// Resize Menu
// =============================================================================

void ResizableEditor::showResizeMenu()
{
    juce::PopupMenu menu;
    const auto availableArea = availableDisplayAreaFor(*this);
    const int fitMargin = scaled(24);
    const auto fitsScale = [this, availableArea, fitMargin](float scale) {
        const auto base = getBasePluginSize();
        const auto width = juce::roundToInt(static_cast<float>(base.getWidth()) * scale);
        const auto height = juce::roundToInt(static_cast<float>(base.getHeight()) * scale);
        return width <= (availableArea.getWidth() - fitMargin) && height <= (availableArea.getHeight() - fitMargin);
    };
    const auto isScaleSelected = [this](float candidate) {
        return std::abs(scaleFactor_ - candidate) < 0.01f;
    };

    // Primary named sizes
    menu.addItem(3, "Medium (100%)", fitsScale(1.0f), isScaleSelected(1.0f));
    menu.addItem(4, "Large (150%)", fitsScale(1.5f), isScaleSelected(1.5f));
    menu.addItem(5, "Extra Large (200%)", fitsScale(2.0f), isScaleSelected(2.0f));

    menu.addSeparator();

    juce::PopupMenu scaleMenu;
    scaleMenu.addItem(11, "75%", fitsScale(0.75f), isScaleSelected(0.75f));
    scaleMenu.addItem(12, "100%", fitsScale(1.0f), isScaleSelected(1.0f));
    scaleMenu.addItem(13, "125%", fitsScale(1.25f), isScaleSelected(1.25f));
    scaleMenu.addItem(14, "150%", fitsScale(1.5f), isScaleSelected(1.5f));
    scaleMenu.addItem(15, "175%", fitsScale(1.75f), isScaleSelected(1.75f));
    scaleMenu.addItem(16, "200%", fitsScale(2.0f), isScaleSelected(2.0f));
    menu.addSubMenu("Scale", scaleMenu, true);

    menu.addSeparator();

    // Full-screen option
    menu.addItem(20, isFullScreen_ ? "Exit Full Screen" : "Full Screen", true, isFullScreen_);

    if (resizeMenuLookAndFeel_)
        menu.setLookAndFeel(resizeMenuLookAndFeel_.get());

    // Show menu
    menu.showMenuAsync(bws::ui::popup_menu::makePopupMenuOptionsForComponent(
                           *resizeCorner_, this, theme_, scaleFactor_, bws::ui::UiDropdownRole::UtilityMenu, 0, this),
                       [this](int result) {
                           switch (result)
                           {
                           case 3:
                               setSizePreset(SizePreset::Medium);
                               break;
                           case 4:
                               setSizePreset(SizePreset::Large);
                               break;
                           case 5:
                               setSizePreset(SizePreset::ExtraLarge);
                               break;

                           case 11:
                               setScaleFactor(0.75f);
                               break;
                           case 12:
                               setScaleFactor(1.0f);
                               break;
                           case 13:
                               setScaleFactor(1.25f);
                               break;
                           case 14:
                               setScaleFactor(1.5f);
                               break;
                           case 15:
                               setScaleFactor(1.75f);
                               break;
                           case 16:
                               setScaleFactor(2.0f);
                               break;

                           case 20:
                               toggleFullScreen();
                               break;

                           default:
                               break;
                           }
                       });
}

// =============================================================================
// Preset/Scale Conversion
// =============================================================================

float ResizableEditor::getScaleForPreset(SizePreset preset) const
{
    switch (preset)
    {
    case SizePreset::Medium:
        return 1.0f;
    case SizePreset::Large:
        return 1.5f;
    case SizePreset::ExtraLarge:
        return 2.0f;
    default:
        return 1.0f;
    }
}

SizePreset ResizableEditor::getPresetForScale(float scale) const
{
    if (scale <= 1.25f)
        return SizePreset::Medium;
    if (scale <= 1.75f)
        return SizePreset::Large;
    return SizePreset::ExtraLarge;
}

// =============================================================================
// Preferences
// =============================================================================

void ResizableEditor::loadPreferences()
{
    if (auto* props = getPropertiesFile())
    {
        const juce::String key = getPluginName() + "_scale";
        scaleFactor_ = static_cast<float>(props->getDoubleValue(key, 1.0));
        scaleFactor_ = juce::jlimit(0.75f, 2.0f, scaleFactor_);
        currentPreset_ = getPresetForScale(scaleFactor_);
    }
}

void ResizableEditor::savePreferences()
{
    if (auto* props = getPropertiesFile())
    {
        const juce::String key = getPluginName() + "_scale";
        props->setValue(key, static_cast<double>(scaleFactor_));
        props->saveIfNeeded();
    }
}

juce::PropertiesFile* ResizableEditor::getPropertiesFile()
{
    auto appDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                      .getChildFile("Bellweather Studios")
                      .getChildFile("Preferences");

    if (!appDir.exists())
        appDir.createDirectory();

    juce::PropertiesFile::Options options;
    options.applicationName = "WeatherPlugins";
    options.filenameSuffix = ".settings";
    options.folderName = appDir.getFullPathName();
    options.osxLibrarySubFolder = "";
    options.commonToAllUsers = false;
    options.ignoreCaseOfKeyNames = true;
    options.storageFormat = juce::PropertiesFile::storeAsXML;

    static juce::PropertiesFile propsFile(appDir.getChildFile("WeatherPlugins.settings"), options);
    return &propsFile;
}

// =============================================================================
// ResizeCornerButton Implementation
// =============================================================================

ResizableEditor::ResizeCornerButton::ResizeCornerButton(ResizableEditor& editor)
    : editor_(editor)
{
    setMouseCursor(juce::MouseCursor::BottomRightCornerResizeCursor);
}

void ResizableEditor::ResizeCornerButton::paint(juce::Graphics& g)
{
    bws::ui::rendering::JuceRenderer renderer(g);
    const float size = static_cast<float>(getWidth());
    const float lineThickness = juce::jmax(1.0f, size * 0.055f);
    const float inset = juce::jmax(3.0f, size * 0.18f);
    const float gridStep = juce::jmax(3.0f, size * 0.19f);

    const auto& colors = editor_.getTheme().weatherColors;
    const auto indicator = (isHovering_ ? colors.accent : colors.borderLight)
                               .withAlpha(isHovering_ ? bws::tokens::shared::opacity::resize_corner::HOVER
                                                      : bws::tokens::shared::opacity::resize_corner::IDLE);

    renderer.setColour(indicator.getARGB());
    for (int i = 0; i < 3; ++i)
    {
        const float shift = static_cast<float>(i) * gridStep;
        renderer.drawLine(size - inset - shift, size - 2.0f, size - 2.0f, size - inset - shift, lineThickness);
    }
}

void ResizableEditor::ResizeCornerButton::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isLeftButtonDown())
    {
        editor_.showResizeMenu();
    }
}

void ResizableEditor::ResizeCornerButton::mouseEnter(const juce::MouseEvent&)
{
    isHovering_ = true;
    repaint();
}

void ResizableEditor::ResizeCornerButton::mouseExit(const juce::MouseEvent&)
{
    isHovering_ = false;
    repaint();
}

// =============================================================================
// FullScreenButton Implementation
// =============================================================================

ResizableEditor::FullScreenButton::FullScreenButton(ResizableEditor& editor)
    : editor_(editor)
{
    updateTooltip();
}

void ResizableEditor::FullScreenButton::paint(juce::Graphics& g)
{
    bws::ui::rendering::JuceRenderer renderer(g);
    const float size = static_cast<float>(getWidth());
    const float padding = size * 0.2f;
    const float lineThickness = 1.5f;
    const float arrowSize = size * 0.25f;

    // Background on hover
    if (isHovering_)
    {
        auto b = getLocalBounds().toFloat();
        renderer.setColour(editor_.getTheme().weatherColors.bgElevated.getARGB());
        renderer.fillRoundedRect(b.getX(), b.getY(), b.getWidth(), b.getHeight(), 3.0f);
    }

    // Draw icon
    const auto colour =
        isHovering_ ? editor_.getTheme().weatherColors.accent : editor_.getTheme().weatherColors.textSecondary;
    renderer.setColour(colour.getARGB());

    if (editor_.isFullScreen())
    {
        // Collapse icon (arrows pointing inward)
        // Top-left
        renderer.drawLine(padding, padding, padding + arrowSize, padding, lineThickness);
        renderer.drawLine(padding, padding, padding, padding + arrowSize, lineThickness);

        // Bottom-right
        renderer.drawLine(size - padding, size - padding, size - padding - arrowSize, size - padding, lineThickness);
        renderer.drawLine(size - padding, size - padding, size - padding, size - padding - arrowSize, lineThickness);
    }
    else
    {
        // Expand icon (arrows pointing outward from corners)
        // Top-left corner
        renderer.drawLine(padding, padding + arrowSize, padding, padding, lineThickness);
        renderer.drawLine(padding, padding, padding + arrowSize, padding, lineThickness);

        // Bottom-right corner
        renderer.drawLine(size - padding - arrowSize, size - padding, size - padding, size - padding, lineThickness);
        renderer.drawLine(size - padding, size - padding, size - padding, size - padding - arrowSize, lineThickness);
    }
}

void ResizableEditor::FullScreenButton::mouseDown(const juce::MouseEvent&)
{
    editor_.toggleFullScreen();
}

void ResizableEditor::FullScreenButton::mouseEnter(const juce::MouseEvent&)
{
    isHovering_ = true;
    repaint();
}

void ResizableEditor::FullScreenButton::mouseExit(const juce::MouseEvent&)
{
    isHovering_ = false;
    repaint();
}

void ResizableEditor::FullScreenButton::updateTooltip()
{
    setTooltip(editor_.isFullScreen() ? "Exit Full Screen (ESC)" : "Enter Full Screen");
}

} // namespace weather
} // namespace bws

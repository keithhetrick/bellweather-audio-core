// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include "bw_ui/adapters/UiThemeKernelAdapter.h"
#include "bw_ui/Components/ComponentSize.h"
#include "bw_ui/Components/UiLabel.h"
#include "bw_ui/Components/UiReadout.h"
#include "bw_ui/interaction/DoubleClickAction.h"
#include "bw_ui/interaction/InteractionPolicy.h"
#include "bw_ui/kernel/Knob.h"
#include "bw_ui/foundation/UiTheme.h"

namespace bws::ui
{

class UiKnob : public juce::Component
{
public:
    UiKnob(UiKnobRole role, const UiThemeResolved& theme);
    ~UiKnob() override;

    // Size-variant factory methods
    static std::unique_ptr<UiKnob> xs(UiKnobRole role, const UiThemeResolved& theme, const juce::String& label = "");
    static std::unique_ptr<UiKnob> sm(UiKnobRole role, const UiThemeResolved& theme, const juce::String& label = "");
    static std::unique_ptr<UiKnob> md(UiKnobRole role, const UiThemeResolved& theme, const juce::String& label = "");
    static std::unique_ptr<UiKnob> lg(UiKnobRole role, const UiThemeResolved& theme, const juce::String& label = "");
    static std::unique_ptr<UiKnob> xl(UiKnobRole role, const UiThemeResolved& theme, const juce::String& label = "");

    // Set size variant programmatically
    void setSizeVariant(ComponentSize size);
    ComponentSize getSizeVariant() const { return currentSize; }

    juce::Slider& getSlider() noexcept { return slider; }
    const juce::Slider& getSlider() const noexcept { return slider; }
    UiReadout& getReadout() noexcept { return readout; }
    const UiReadout& getReadout() const noexcept { return readout; }
    UiReadoutRole getReadoutRole() const noexcept { return readoutRole; }
    const kernel::ResolvedKnobModel& getResolvedKnobModel() const noexcept { return resolvedModel; }
    bool isLocked() const noexcept { return locked; }

    void setLabelText(const juce::String& text);
    void setLabelVisible(bool shouldShow);
    void setRange(double newMin, double newMax, double newInterval = 0.0)
    {
        slider.setRange(newMin, newMax, newInterval);
    }
    void setUnit(ReadoutUnit unit);
    void setDefaultValue(double value);

    /// Magnetic center detent at the given proportional position (0..1).
    /// Paints a tick; drag snaps within ±1.5% of the position.
    void setCenterDetent(bool enable, double centerProportion = 0.5);
    bool hasCenterDetent() const noexcept;
    double getCenterDetentPosition() const noexcept;
    void setScale(float newScale);
    void setTheme(const UiThemeResolved& newTheme);
    void setFormatter(std::function<juce::String(double)> formatterFn);
    void setTooltip(const juce::String& text);
    void setLocked(bool shouldLock);
    void cancelUserInteraction();

    using DoubleClickAction = bws::ui::DoubleClickAction;
    using InteractionPolicy = bws::ui::interaction::InteractionPolicy;

    // Message-thread only. Custom callback (if used) must outlive this UiKnob
    // and must not synchronously destroy it.
    void setDoubleClickAction(DoubleClickAction action);
    void setCustomDoubleClickCallback(std::function<void()> callback);
    DoubleClickAction getDoubleClickAction() const noexcept;

    void setInteractionPolicy(InteractionPolicy newPolicy);
    const InteractionPolicy& getInteractionPolicy() const noexcept;

    // Programmatically set the displayed value WITHOUT firing onValueChange,
    // SliderAttachment listeners, automation gestures, or preset-dirty state.
    // Pairs juce::Slider::setValue(.., dontSendNotification) with refreshReadout()
    // so the rotary indicator and numeric readout stay in sync - naive
    // dontSendNotification skips onValueChange and leaves the readout stale.
    // NaN/Inf input is dropped; JUCE Slider does not clamp non-finite values.
    void setDisplayValue(double newValue);

    void resized() override;

#if BWS_TESTING
    juce::Rectangle<int> testReadoutBounds() const { return readout.getBounds(); }
#endif

private:
    class KnobSlider : public juce::Slider
    {
    public:
        KnobSlider();
        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDoubleClick(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;
        void mouseUp(const juce::MouseEvent& e) override;
        void mouseEnter(const juce::MouseEvent& e) override;
        void mouseExit(const juce::MouseEvent& e) override;
        void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
        void focusGained(FocusChangeType cause) override;
        void focusLost(FocusChangeType cause) override;
        bool keyPressed(const juce::KeyPress& key) override;
        void setBaseSensitivity(int value) { baseSensitivity = value; }
        void setResolvedKnobModel(const kernel::ResolvedKnobModel& model) { resolvedModel = model; }
        void setLocked(bool shouldLock) { locked = shouldLock; }
        void setInteractionSuppressed(bool shouldSuppress) { interactionSuppressed = shouldSuppress; }
        std::function<void(bool)> onHoverChanged;
        std::function<void(bool)> onDragChanged;
        std::function<void(bool)> onFocusChanged;
        std::function<void()> onRequestReset;
        std::function<void()> onRequestTextEntry;

        bws::ui::interaction::InteractionPolicy policy_ {
            .doubleClick = {.action = DoubleClickAction::TextEntry},
        };

    private:
        int baseSensitivity {400};
        int fineSensitivity {1600};
        kernel::ResolvedKnobModel resolvedModel {};
        bool locked {false};
        bool interactionSuppressed {false};
    };

    class KnobLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        KnobLookAndFeel(const UiThemeResolved* themeIn, UiKnobRole roleIn, float* scaleRef);
        void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional,
                              float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;

    private:
        const UiThemeResolved* theme {nullptr};
        UiKnobRole role;
        float* scalePtr {nullptr};
    };

    void refreshReadout();
    void refreshInteractionState();
    void applySizeVariant(ComponentSize size);
    struct EditSessionContext
    {
        juce::String displayText;
        juce::String seedText;
        bool active {false};
    };

    EditSessionContext buildEditSessionContext() const;
    juce::String makeEditableSeedText();
    bool commitEditedText(const juce::String& candidate, UiReadout::EditCommitTrigger trigger);
    void resetToDefaultValue();
    void beginCompanionEditing();

    enum class InteractionState
    {
        Normal,
        Hovered,
        Active,
        Editing
    };

    ComponentSize currentSize {ComponentSize::MD};
    UiKnobRole role;
    kernel::ResolvedKnobModel resolvedModel;
    UiThemeResolved theme;
    kernel::ThemeSnapshot kernelTheme;
    float scale {1.0f};
    KnobSlider slider;
    KnobLookAndFeel knobLnf;
    UiLabel label;
    UiReadoutRole readoutRole {UiReadoutRole::Standard};
    UiReadout readout;
    ReadoutUnit unit {ReadoutUnit::Plain};
    double defaultValue {0.0};
    std::function<juce::String(double)> formatter;
    bool showLabel {true};
    bool locked {false};
    bool sliderHovered {false};
    bool sliderDragging {false};
    bool sliderFocused {false};
    bool readoutHovered {false};
    bool readoutEditing {false};
    EditSessionContext activeEditSessionContext;
    InteractionState interactionState {InteractionState::Normal};
};

} // namespace bws::ui

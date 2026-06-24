// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <bw_ui/foundation/UiTheme.h>
#include <bw_ui/preset_system/WeatherDiscoveryPrimitives.h>

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace bws::ui
{

class WeatherNumericEntryDialog : public juce::Component
{
public:
    using OnCommit = std::function<void(const juce::String&)>;

    WeatherNumericEntryDialog(const UiThemeResolved& theme, float scaleFactor, juce::String title, juce::String message,
                              juce::String initialValue, OnCommit onCommit, std::function<void()> onDismiss);

    void setDismissCallback(std::function<void()> cb) { dismissCallback_ = std::move(cb); }
    void grabInitialKeyboardFocus();

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    juce::Rectangle<int> getCardBounds() const;
    void commit();
    void dismiss();

    const UiThemeResolved& theme_;
    float scaleFactor_ {1.0f};
    juce::String title_;
    juce::String message_;
    OnCommit onCommit_;
    std::function<void()> dismissCallback_;

    bws::weather::WeatherFloatingPanelShell shell_;
    juce::Label titleLabel_;
    juce::Label messageLabel_;
    juce::TextEditor valueEditor_;
    bws::weather::WeatherChipButton okButton_;
    bws::weather::WeatherChipButton cancelButton_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WeatherNumericEntryDialog)
};

void showNumericEntryDialog(const UiThemeResolved& theme, float scaleFactor, juce::String title, juce::String message,
                            juce::String initialValue, WeatherNumericEntryDialog::OnCommit onCommit,
                            juce::Component* parent, bool desktopPromote = false);

} // namespace bws::ui

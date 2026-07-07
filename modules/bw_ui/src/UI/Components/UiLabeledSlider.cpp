// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/Components/UiLabeledSlider.h"
#include "bw_ui/generated/BwTokens.h"

namespace bws::ui
{

UiLabeledSlider::SliderLookAndFeel::SliderLookAndFeel(const UiThemeResolved* themeIn, float* scaleRef)
    : theme(themeIn)
    , scalePtr(scaleRef)
{}

void UiLabeledSlider::SliderLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                                          float sliderPos, float /*minSliderPos*/,
                                                          float /*maxSliderPos*/, juce::Slider::SliderStyle style,
                                                          juce::Slider& slider)
{
    juce::ignoreUnused(sliderPos, style);

    if (theme == nullptr || scalePtr == nullptr)
        return;

    const float s = *scalePtr;
    const float trackH = (float)editor::labeledSliderTrackHeight(s, *theme);
    const float radius = juce::jmax(4.0f, trackH * 0.35f);

    auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height);
    auto track = bounds.withHeight(trackH).withCentre(bounds.getCentre());

    namespace ops = bws::tokens::shared::opacity::ui_slider;
    g.setColour(theme->colors.bg2.withAlpha(ops::TRACK_BG));
    g.fillRoundedRectangle(track, trackH * 0.5f);

    g.setColour(theme->colors.outline0.withAlpha(bws::tokens::shared::opacity::outline::EMPHASIZED));
    g.drawRoundedRectangle(track, trackH * 0.5f, theme->surfaces.strokeThin);

    const float proportion = juce::jlimit(0.0f, 1.0f, (float)slider.valueToProportionOfLength(slider.getValue()));
    auto fill = track.withWidth(track.getWidth() * proportion);
    g.setColour(theme->colors.accent0.withAlpha(bws::tokens::shared::opacity::ui_slider::FILL));
    g.fillRoundedRectangle(fill, trackH * 0.5f);

    const float thumbX = track.getX() + track.getWidth() * proportion;
    const float thumbY = track.getCentreY();
    g.setColour(theme->colors.outline0.withAlpha(bws::tokens::shared::opacity::ui_slider::THUMB_OUTLINE));
    g.fillEllipse(thumbX - radius, thumbY - radius, radius * 2.0f, radius * 2.0f);
    g.setColour(theme->colors.accent1);
    g.drawEllipse(thumbX - radius, thumbY - radius, radius * 2.0f, radius * 2.0f, theme->surfaces.strokeThin);
}

int UiLabeledSlider::SliderLookAndFeel::getSliderThumbRadius(juce::Slider&)
{
    if (theme == nullptr || scalePtr == nullptr)
        return 6;

    const float s = *scalePtr;
    const float trackH = (float)editor::labeledSliderTrackHeight(s, *theme);
    return (int)std::round(juce::jmax(6.0f, trackH * 0.35f));
}

UiLabeledSlider::UiLabeledSlider(const UiThemeResolved& themeIn)
    : theme(themeIn)
    , kernelTheme(adapters::makeKernelThemeSnapshot(themeIn))
    , label(UiTypographyRole::SectionLabel, themeIn)
    , readout(readoutRole, themeIn)
    , sliderLnf(&theme, &scale)
{
    label.setKernelTheme(kernelTheme);
    label.setTextRole(kernel::TextRole::SectionLabel);
    sliderComp.setLookAndFeel(&sliderLnf);
    sliderComp.setSliderStyle(juce::Slider::LinearHorizontal);
    sliderComp.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    sliderComp.onValueChange = [this]() {
        refreshReadout();
    };

    label.setUseSecondaryColour(true);

    addAndMakeVisible(label);
    addAndMakeVisible(sliderComp);
    addAndMakeVisible(readout);
}

UiLabeledSlider::~UiLabeledSlider()
{
    sliderComp.setLookAndFeel(nullptr);
}

void UiLabeledSlider::setLabelText(const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
}

void UiLabeledSlider::setScale(float newScale)
{
    scale = newScale;
    label.setScale(newScale);
    readout.setScale(newScale);
    repaint();
    resized();
}

void UiLabeledSlider::setTheme(const UiThemeResolved& newTheme)
{
    theme = newTheme;
    kernelTheme = adapters::makeKernelThemeSnapshot(newTheme);
    label.setTheme(newTheme);
    label.setKernelTheme(kernelTheme);
    readout.setTheme(newTheme);
    sliderComp.repaint();
    repaint();
}

void UiLabeledSlider::setReadoutRole(UiReadoutRole newRole)
{
    readoutRole = newRole;
    readout.setRole(newRole);
    resized();
}

void UiLabeledSlider::setUnit(ReadoutUnit newUnit)
{
    unit = newUnit;
    readout.setUnit(newUnit);
    refreshReadout();
}

void UiLabeledSlider::setFormatter(std::function<juce::String(double)> formatterFn)
{
    formatter = std::move(formatterFn);
    refreshReadout();
}

void UiLabeledSlider::setFormat(const kernel::FormatSpec& spec, double displayScale)
{
    setFormatter(
        [spec, displayScale](double value) { return juce::String(kernel::formatValue(value * displayScale, spec)); });
}

void UiLabeledSlider::refreshReadout()
{
    if (formatter)
    {
        readout.setText(formatter(sliderComp.getValue()));
        return;
    }

    switch (unit)
    {
    case ReadoutUnit::Decibels:
        readout.setValue((float)sliderComp.getValue());
        break;
    case ReadoutUnit::Percent:
        readout.setValue((float)sliderComp.getValue());
        break;
    case ReadoutUnit::Plain:
    default:
        readout.setValue((float)sliderComp.getValue());
        break;
    }
}

void UiLabeledSlider::resized()
{
    auto bounds = getLocalBounds();
    const auto readMetrics = UiReadouts::metrics(readoutRole, scale, theme);

    const bool hasLabel = label.getText().isNotEmpty();
    const auto stack = editor::labeledSliderStackLayout(scale, theme, kernelTheme, readoutRole, spacingRole, hasLabel);
    const int labelBlock = stack.labelHeight + stack.labelGap;
    const int readoutBlock = stack.readoutHeight;
    const int sliderBlock = stack.interactiveHeight;

    auto labelArea = bounds.removeFromTop(labelBlock);
    auto readoutArea = bounds.removeFromBottom(readoutBlock);
    const int sliderHeight = juce::jmin(sliderBlock, bounds.getHeight());
    auto sliderArea = bounds.withHeight(sliderHeight).withCentre(bounds.getCentre());

    if (hasLabel)
        label.setBounds(labelArea);
    else
        label.setBounds({});
    sliderComp.setBounds(sliderArea);
    readout.setBounds(readoutArea.withSizeKeepingCentre(
        (int)juce::jmax(readMetrics.minWidth, (float)readoutArea.getWidth()), (int)std::round(readMetrics.height)));
}

// Factory methods
std::unique_ptr<UiLabeledSlider> UiLabeledSlider::xs(const UiThemeResolved& theme, const juce::String& label)
{
    auto slider = std::make_unique<UiLabeledSlider>(theme);
    slider->setSizeVariant(ComponentSize::XS);
    if (label.isNotEmpty())
        slider->setLabelText(label);
    return slider;
}

std::unique_ptr<UiLabeledSlider> UiLabeledSlider::sm(const UiThemeResolved& theme, const juce::String& label)
{
    auto slider = std::make_unique<UiLabeledSlider>(theme);
    slider->setSizeVariant(ComponentSize::SM);
    if (label.isNotEmpty())
        slider->setLabelText(label);
    return slider;
}

std::unique_ptr<UiLabeledSlider> UiLabeledSlider::md(const UiThemeResolved& theme, const juce::String& label)
{
    auto slider = std::make_unique<UiLabeledSlider>(theme);
    slider->setSizeVariant(ComponentSize::MD);
    if (label.isNotEmpty())
        slider->setLabelText(label);
    return slider;
}

std::unique_ptr<UiLabeledSlider> UiLabeledSlider::lg(const UiThemeResolved& theme, const juce::String& label)
{
    auto slider = std::make_unique<UiLabeledSlider>(theme);
    slider->setSizeVariant(ComponentSize::LG);
    if (label.isNotEmpty())
        slider->setLabelText(label);
    return slider;
}

std::unique_ptr<UiLabeledSlider> UiLabeledSlider::xl(const UiThemeResolved& theme, const juce::String& label)
{
    auto slider = std::make_unique<UiLabeledSlider>(theme);
    slider->setSizeVariant(ComponentSize::XL);
    if (label.isNotEmpty())
        slider->setLabelText(label);
    return slider;
}

void UiLabeledSlider::setSizeVariant(ComponentSize size)
{
    currentSizeVariant = size;
    applySizeVariant(size);
}

void UiLabeledSlider::applySizeVariant(ComponentSize size)
{
    const int width = editor::labeledSliderVariantWidth(size);
    const int height =
        editor::labeledSliderVariantHeight(size, 1.0f, theme, kernelTheme, readoutRole, spacingRole, true);
    Component::setSize(width, height);
}

} // namespace bws::ui

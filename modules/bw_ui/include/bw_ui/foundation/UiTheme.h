// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_graphics/juce_graphics.h>
#include <optional>

// Struct brace-initializer defaults below reference tokens directly so the
// fallback path (default-constructed metrics) stays in lockstep with the
// production path (defaultKnobs() / defaultDropdowns() / etc. in UiTheme.cpp
// that populate from these same token namespaces). See
// for the full flow.
#include "bw_ui/generated/BwTokens.h"

namespace bws::ui
{
enum class UiTypographyRole
{
    BrandSmall,
    Title,
    SectionLabel,
    ControlText,
    Readout,
    Tab,
    Overlay,
    HeaderBrand,
    HeaderPreset,
    Annotation // Spatial reference text: dB ticks, meter scales, axis labels, badges
};

enum class UiKnobRole
{
    Hero,     // Semantic focal control role; richer doctrine resolves through kernel::ResolvedKnobModel
    Primary,  // Semantic workhorse control role; renderer/layout should not reinterpret this locally
    Secondary // Semantic supporting control role; non-Hero roles externalize value readout
};

enum class UiDropdownRole
{
    Standard,
    UtilityMenu, // Shared middle lane for compact utility surfaces like settings,
                 // resize, and oversampling menus. Tighter than Standard,
                 // roomier than Overlay.
    Compact,
    Overlay // Micro-chrome for corner-overlay selectors. Smaller metrics
            // + Annotation font + filled-triangle arrow. Matches surrounding
            // toggle pills and hardware-inspired chrome.
};

enum class UiTabRole
{
    Standard
};

enum class UiReadoutRole
{
    Standard,
    Compact
};

enum class UiDensity
{
    Standard,
    Compact
};

enum class UiPanelFinish
{
    Matte,
    BrushedMetal
};

enum class ReadoutDecimalPolicy
{
    Auto1,
    Auto2
};

enum class FontWeight
{
    regular,
    medium,
    semiBold,
    bold
};

struct UiTypographySpec
{
    float height {12.0f};
    FontWeight weight {FontWeight::regular};
    float tracking {0.0f};
    bool uppercase {false};
};

struct UiKnobMetrics
{
    float diameter {::bws::tokens::shared::knobs::PRIMARY.diameter};
    float ringThickness {::bws::tokens::shared::knobs::PRIMARY.ringThickness};
    float arcThickness {::bws::tokens::shared::knobs::PRIMARY.arcThickness};
    float pointerLength {::bws::tokens::shared::knobs::PRIMARY.pointerLength};
    float pointerThickness {::bws::tokens::shared::knobs::PRIMARY.pointerThickness};
    float labelPadding {::bws::tokens::shared::knobs::PRIMARY.labelPadding};
    float readoutPadding {::bws::tokens::shared::knobs::PRIMARY.readoutPadding};
    float hitPadding {::bws::tokens::shared::knobs::PRIMARY.hitPadding};
};

struct UiDropdownMetrics
{
    float height {::bws::tokens::shared::dropdowns::standard::HEIGHT};
    float padding {::bws::tokens::shared::dropdowns::standard::PADDING};
    float arrowBoxWidth {::bws::tokens::shared::dropdowns::standard::ARROW_BOX_WIDTH};
    float cornerRadius {::bws::tokens::shared::dropdowns::standard::CORNER_RADIUS};
    float outlineThickness {::bws::tokens::shared::dropdowns::standard::OUTLINE_THICKNESS};
    float popupFontScale {::bws::tokens::shared::dropdowns::standard::POPUP_FONT_SCALE};
    float minPopupWidth {::bws::tokens::shared::dropdowns::standard::MIN_POPUP_WIDTH};
    // Dimensionless ratios - not density-scaled. The values they multiply
    // (font height) already carry density; double-scaling would be quadratic.
    float rowHeightFactor {::bws::tokens::shared::dropdowns::standard::ROW_HEIGHT_FACTOR};
    float tickGlyphFactor {::bws::tokens::shared::dropdowns::standard::TICK_GLYPH_FACTOR};
    float minTickGlyphRadius {::bws::tokens::shared::dropdowns::standard::MIN_TICK_GLYPH_RADIUS};
    float separatorInsetX {::bws::tokens::shared::dropdowns::standard::SEPARATOR_INSET_X};
    float shadowAlpha {::bws::tokens::shared::dropdowns::standard::SHADOW_ALPHA};
    float shadowBlur {::bws::tokens::shared::dropdowns::standard::SHADOW_BLUR};
    float shadowOffsetY {::bws::tokens::shared::dropdowns::standard::SHADOW_OFFSET_Y};
};

struct UiTooltipMetrics
{
    float minWidth {::bws::tokens::shared::tooltips::standard::MIN_WIDTH};
    float maxWidth {::bws::tokens::shared::tooltips::standard::MAX_WIDTH};
    float maxWidthRatio {::bws::tokens::shared::tooltips::standard::MAX_WIDTH_RATIO};
    float fontScale {::bws::tokens::shared::tooltips::standard::FONT_SCALE};
    float paddingX {::bws::tokens::shared::tooltips::standard::PADDING_X};
    float paddingY {::bws::tokens::shared::tooltips::standard::PADDING_Y};
    float offsetX {::bws::tokens::shared::tooltips::standard::OFFSET_X};
    float offsetY {::bws::tokens::shared::tooltips::standard::OFFSET_Y};
    float minTextWidth {::bws::tokens::shared::tooltips::standard::MIN_TEXT_WIDTH};
    float parentInset {::bws::tokens::shared::tooltips::standard::PARENT_INSET};
    float cornerRadiusScale {::bws::tokens::shared::tooltips::standard::CORNER_RADIUS_SCALE};
    float shadowAlpha {::bws::tokens::shared::tooltips::standard::SHADOW_ALPHA};
    float shadowBlur {::bws::tokens::shared::tooltips::standard::SHADOW_BLUR};
    float shadowOffsetY {::bws::tokens::shared::tooltips::standard::SHADOW_OFFSET_Y};
};

struct UiTabMetrics
{
    float height {::bws::tokens::shared::tabs::standard::HEIGHT};
    float paddingX {::bws::tokens::shared::tabs::standard::PADDING_X};
    float cornerRadius {::bws::tokens::shared::tabs::standard::CORNER_RADIUS};
    float outlineThickness {::bws::tokens::shared::tabs::standard::OUTLINE_THICKNESS};
};

struct UiReadoutMetrics
{
    float minWidth {::bws::tokens::shared::readouts::standard::MIN_WIDTH};
    float height {::bws::tokens::shared::readouts::standard::HEIGHT};
    float paddingX {::bws::tokens::shared::readouts::standard::PADDING_X};
    float paddingY {::bws::tokens::shared::readouts::standard::PADDING_Y};
    float cornerRadius {::bws::tokens::shared::readouts::standard::CORNER_RADIUS};
    float outlineThickness {::bws::tokens::shared::readouts::standard::OUTLINE_THICKNESS};
    ReadoutDecimalPolicy decimals {ReadoutDecimalPolicy::Auto1}; // enum - not a numeric token
};

struct UiSpacing
{
    float xxs {static_cast<float>(::bws::tokens::shared::spacing::XXS)};
    float xs {static_cast<float>(::bws::tokens::shared::spacing::XS)};
    float sm {static_cast<float>(::bws::tokens::shared::spacing::SM)};
    float md {static_cast<float>(::bws::tokens::shared::spacing::MD)};
    float lg {static_cast<float>(::bws::tokens::shared::spacing::LG)};
    float xl {static_cast<float>(::bws::tokens::shared::spacing::XL)};
    float xxl {static_cast<float>(::bws::tokens::shared::spacing::XXL)};
};

struct UiSurfaceTokens
{
    float radiusSm {::bws::tokens::shared::surfaces::RADIUS_SM};
    float radiusMd {::bws::tokens::shared::surfaces::RADIUS_MD};
    float radiusLg {::bws::tokens::shared::surfaces::RADIUS_LG};
    float strokeHairline {::bws::tokens::shared::surfaces::STROKE_HAIRLINE};
    float strokeThin {::bws::tokens::shared::surfaces::STROKE_THIN};
    float strokeMed {::bws::tokens::shared::surfaces::STROKE_MED};
    float elevationFlat {::bws::tokens::shared::surfaces::ELEVATION_FLAT};
    float elevationRaised {::bws::tokens::shared::surfaces::ELEVATION_RAISED};
    float elevationHero {::bws::tokens::shared::surfaces::ELEVATION_HERO};
    UiPanelFinish finish {UiPanelFinish::Matte}; // enum - not a numeric token
};

struct UiColorTokens
{
    juce::Colour bg0;
    juce::Colour bg1;
    juce::Colour bg2;
    juce::Colour text0;
    juce::Colour text1;
    juce::Colour textDisabled;
    juce::Colour outline0;
    juce::Colour accent0;
    juce::Colour accent1;
    juce::Colour ok;
    juce::Colour warn;
    juce::Colour danger;
    juce::Colour glowAccent;
};

struct UiKnobTokenSet
{
    UiKnobMetrics hero {};
    UiKnobMetrics primary {};
    UiKnobMetrics secondary {};
};

struct UiDropdownTokenSet
{
    UiDropdownMetrics standard {};
    UiDropdownMetrics utilityMenu {};
    UiDropdownMetrics compact {};
    UiDropdownMetrics overlay {};
};

struct UiReadoutTokenSet
{
    UiReadoutMetrics standard {};
    UiReadoutMetrics compact {};
};

struct UiTabTokenSet
{
    UiTabMetrics standard {};
};

struct UiTypographyTokenSet
{
    UiTypographySpec brandSmall {};
    UiTypographySpec title {};
    UiTypographySpec sectionLabel {};
    UiTypographySpec controlText {};
    UiTypographySpec readout {};
    UiTypographySpec tab {};
    UiTypographySpec overlay {};
    UiTypographySpec headerBrand {};
    UiTypographySpec headerPreset {};
    UiTypographySpec annotation {};
};

struct UiHeaderTokenSet
{
    int height {::bws::tokens::shared::header::HEIGHT};
    float brandZoneWidth {static_cast<float>(::bws::tokens::shared::header::BRAND_ZONE_WIDTH)};
    float iconSize {static_cast<float>(::bws::tokens::shared::header::ICON_SIZE)};
    float iconPadding {static_cast<float>(::bws::tokens::shared::header::ICON_PADDING)};
    float nameIconGap {static_cast<float>(::bws::tokens::shared::header::NAME_ICON_GAP)};
    float nameRightPad {static_cast<float>(::bws::tokens::shared::header::NAME_RIGHT_PAD)};
    float nameCategoryGap {static_cast<float>(::bws::tokens::shared::header::NAME_CATEGORY_GAP)};
    float funcLeftOffset {static_cast<float>(::bws::tokens::shared::header::FUNC_LEFT_OFFSET)};
    float navWidth {static_cast<float>(::bws::tokens::shared::header::NAV_WIDTH)};
    float saveWidth {static_cast<float>(::bws::tokens::shared::header::SAVE_WIDTH)};
    float abWidth {static_cast<float>(::bws::tokens::shared::header::AB_WIDTH)};
    float settingsWidth {static_cast<float>(::bws::tokens::shared::header::SETTINGS_WIDTH)};
    float rightInset {static_cast<float>(::bws::tokens::shared::header::RIGHT_INSET)};
    float gap {static_cast<float>(::bws::tokens::shared::header::GAP)};
    float renamePadV {static_cast<float>(::bws::tokens::shared::header::RENAME_PAD_V)};
};

struct UiShellLayoutTokens
{
    float shellInset {20.0f};
    float shellGapXs {4.0f};
    float shellGapSm {8.0f};
    float shellGapMd {12.0f};
    float shellGapLg {16.0f};
    float shellGapXl {24.0f};

    float footerHeight {100.0f};
    float footerTopPad {8.0f};
    float footerBottomPad {10.0f};
    float footerRowGap {8.0f};
    float footerToolbarHeight {20.0f};
    float footerSupportHeight {16.0f};
    float footerHeroHeight {28.0f};
    float footerHeroInsetX {28.0f};
    float footerSupportInsetX {6.0f};
    float footerToolbarInset {8.0f};

    float tabStripHeight {24.0f};
    float tabGap {4.0f};
    float tabInsetX {20.0f};
    float tabOffsetY {2.0f};
    float tabHeight {20.0f};
    float tabMinWidth {72.0f};
    float tabPadWidth {32.0f};
    float tabFontScale {0.96f};

    float headerHeightFallback {32.0f};
    float fullscreenCompactFallbackHeight {500.0f};

    float contentMargin {12.0f};
    float contentTopInset {6.0f};
    float contentSectionGap {6.0f};
    float contentReadoutSlack {16.0f};
    float contentReadoutHeight {16.0f};
    float contentLeadMinHeight {80.0f};
    float contentMiddleMinHeight {40.0f};
    float contentWidthCap {1200.0f};
    float contentOuterColumnRatio {0.16f};
    float contentLeadShare {0.70f};
    int contentTargetColumns {9};
    int contentMinColumns {5};
    int contentMaxColumns {9};

    float updateButtonWidth {130.0f};
    float updateButtonHeight {24.0f};
    float updateButtonMargin {10.0f};
};

struct UiWeatherColorSet
{
    juce::Colour accent {0xFFD4AF37};
    juce::Colour glow {0x59D4AF37};
    juce::Colour dim {0x99D4AF37};
    juce::Colour bgMain {0xFF1A1A1A};
    juce::Colour bgRaised {0xFF2A2A2A};
    juce::Colour bgElevated {0xFF3A3A3A};
    juce::Colour bgInput {0xFF252525};
    juce::Colour borderLight {0xFF4A4A4A};
    juce::Colour borderDark {0xFF1A1A1A};
    juce::Colour textBright {0xFFE0E0E0};
    juce::Colour textPrimary {0xFFFEFEF0};
    juce::Colour textSecondary {0xFFAAAAAA};
    juce::Colour textTertiary {0xFF888888};
    juce::Colour textDisabled {0xFF666666};
    juce::Colour surfacePrimary {0xFFA89968};
    juce::Colour surfaceHighlight {0xFFC0A575};
    juce::Colour surfaceDark {0xFF8B7F6A};
    juce::Colour ledBorder {0xFF1F1F1F};
    juce::Colour frameDark {0xFF5A4A2F};
    juce::Colour panelDeeper {0xFF0F0F0F};
    juce::Colour panelNearBlack {0xFF0A0A0A};
    juce::Colour panelDark {0xFF151515};
    juce::Colour linkBlue {0xFF4A90D9};
    juce::Colour precisionBlue {0xFF60C0E0};
    juce::Colour trimYellow {0xFFFFCC00};
    juce::Colour tickMark {0xFF6A6A6A};
    float radiusSm {2.0f};
    float radiusMd {4.0f};
    float radiusLg {6.0f};
};

struct UiSecondaryKnobStyle
{
    float warmth {1.0f};          // Weather/premium material bias vs neutral UI tones
    float depth {1.0f};           // Rim/bowl/seat contrast strength
    float arcEnergy {1.0f};       // Active arc lift and value-response liveliness
    float readoutEmphasis {1.0f}; // Compact external readout authority
};

// Live-trace cold/hot endpoints used by envelope displays. Plugins customize
// via per-product `bds/tokens/products/<plugin>.tokens.json` under
// `colors.envelope.trace.cold/hot`. Linear two-stop interpolation is the
// default; non-linear curves / multi-stop gradients are extension points.
// Nested namespace keeps this surface grouped by concern.
// `envelope.marker.*`, `envelope.grid.*`, etc. without renaming.
struct UiEnvelopeTraceTokens
{
    juce::Colour cold;
    juce::Colour hot;
};

struct UiEnvelopeTokens
{
    UiEnvelopeTraceTokens trace {};
};

// GR wash stops; plugins set colors.dynamicsWash.* per product.
struct UiDynamicsWashTokens
{
    juce::Colour stableMid;
    juce::Colour working;
    juce::Colour heavy;
};

// Envelope-display chrome by role; plugins set colors.envelopeChrome.* per product.
struct UiEnvelopeChromeTokens
{
    juce::Colour panel;
    juce::Colour outline;
    juce::Colour grid;
    juce::Colour gridFill;
    juce::Colour rangeFloor;
    juce::Colour autoReleaseBand;
    juce::Colour curve;
    juce::Colour curveGlow;
    juce::Colour markerBase;
    juce::Colour threshold;
};

struct UiThemeBase
{
    UiColorTokens colors {};
    UiSpacing spacing {};
    UiSurfaceTokens surfaces {};
    UiTypographyTokenSet typography {};
    UiHeaderTokenSet header {};
    UiWeatherColorSet weatherColors {};
    UiSecondaryKnobStyle secondaryKnobStyle {};
    UiKnobTokenSet knobs {};
    UiDropdownTokenSet dropdowns {};
    UiTooltipMetrics tooltips {};
    UiReadoutTokenSet readouts {};
    UiTabTokenSet tabs {};
    UiShellLayoutTokens shellLayout {};
    UiEnvelopeTokens envelope {};
    UiDynamicsWashTokens dynamicsWash {};
    UiEnvelopeChromeTokens envelopeChrome {};
    UiDensity density {UiDensity::Standard};
    float heroEmphasis {1.0f};
};

struct UiThemeOverride
{
    std::optional<UiColorTokens> colors {};
    std::optional<juce::Colour> accent0 {};
    std::optional<juce::Colour> accent1 {};
    std::optional<UiPanelFinish> panelFinish {};
    std::optional<UiDensity> density {};
    std::optional<float> heroEmphasis {};
    std::optional<UiTypographyTokenSet> typography {};
    std::optional<UiHeaderTokenSet> header {};
    std::optional<UiWeatherColorSet> weatherColors {};
    std::optional<UiSecondaryKnobStyle> secondaryKnobStyle {};
    std::optional<UiShellLayoutTokens> shellLayout {};
    std::optional<UiEnvelopeTokens> envelope {};
    std::optional<UiDynamicsWashTokens> dynamicsWash {};
    std::optional<UiEnvelopeChromeTokens> envelopeChrome {};
};

struct UiThemeResolved
{
    UiColorTokens colors {};
    UiSpacing spacing {};
    UiSurfaceTokens surfaces {};
    UiTypographyTokenSet typography {};
    UiHeaderTokenSet header {};
    UiWeatherColorSet weatherColors {};
    UiSecondaryKnobStyle secondaryKnobStyle {};
    UiKnobTokenSet knobs {};
    UiDropdownTokenSet dropdowns {};
    UiTooltipMetrics tooltips {};
    UiReadoutTokenSet readouts {};
    UiTabTokenSet tabs {};
    UiShellLayoutTokens shellLayout {};
    UiEnvelopeTokens envelope {};
    UiDynamicsWashTokens dynamicsWash {};
    UiEnvelopeChromeTokens envelopeChrome {};
    UiDensity density {UiDensity::Standard};
    // Theme-density multiplier - a separate scale axis from windowScale/chromeScale
    // (a multiplier, not a min-cap; intentionally distinct from chromeScale).
    float densityScale {1.0f};
    float heroEmphasis {1.0f};
};

UiThemeBase defaultUiThemeBase();
UiThemeResolved resolveTheme(const UiThemeBase& base, const UiThemeOverride& patch);

namespace UiKnobs
{
UiKnobMetrics metrics(UiKnobRole role, float scale, const UiThemeResolved& theme);
} // namespace UiKnobs

namespace UiDropdowns
{
UiDropdownMetrics metrics(UiDropdownRole role, float scale, const UiThemeResolved& theme);
} // namespace UiDropdowns

namespace UiTooltips
{
UiTooltipMetrics metrics(float scale, const UiThemeResolved& theme);
} // namespace UiTooltips

namespace UiTabsTokens
{
UiTabMetrics metrics(UiTabRole role, float scale, const UiThemeResolved& theme);
} // namespace UiTabsTokens

namespace UiReadouts
{
UiReadoutMetrics metrics(UiReadoutRole role, float scale, const UiThemeResolved& theme);
} // namespace UiReadouts

} // namespace bws::ui

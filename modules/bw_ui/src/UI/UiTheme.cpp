// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/foundation/UiTheme.h"
#include "bw_ui/internal/UiBootstrap.h"
#include "bw_ui/foundation/Fonts.h"
#include "bw_ui/generated/BwTokens.h"

namespace bws::ui
{
namespace
{
UiTypographyTokenSet defaultTypography()
{
    // All values sourced from bws::tokens::shared::typography::*
    // Generated from bds/tokens/products/shared.tokens.json.
    // To change any size or tracking value: edit the JSON, regenerate BwTokens.h.
    namespace sizes = bws::tokens::shared::typography::sizes;
    namespace tracking = bws::tokens::shared::typography::tracking;

    UiTypographyTokenSet t;
    t.brandSmall = {sizes::LABEL, FontWeight::semiBold, tracking::WIDE, true};
    t.title = {sizes::HERO, FontWeight::semiBold, tracking::NORMAL, true};
    t.sectionLabel = {sizes::DISPLAY, FontWeight::semiBold, tracking::WIDE, true};
    t.controlText = {sizes::HEADING, FontWeight::medium, tracking::TIGHT, false};
    t.readout = {sizes::TITLE, FontWeight::semiBold, tracking::TIGHT, false};
    t.tab = {sizes::DISPLAY, FontWeight::medium, tracking::TIGHT, true};
    t.overlay = {sizes::DISPLAY, FontWeight::bold, tracking::NORMAL, true};
    t.headerBrand = {sizes::HEADING, FontWeight::semiBold, tracking::BRAND, true};
    t.headerPreset = {sizes::TITLE, FontWeight::semiBold, tracking::TIGHT, false};
    // Spatial reference text - dB ticks, meter scales, axis labels, status badges
    // 9pt, medium weight, tight tracking, no uppercase
    t.annotation = {sizes::SMALL, FontWeight::medium, tracking::TIGHT, false};
    return t;
}

UiHeaderTokenSet defaultHeader()
{
    namespace h = bws::tokens::shared::header;

    UiHeaderTokenSet s;
    s.height = h::HEIGHT;
    s.brandZoneWidth = static_cast<float>(h::BRAND_ZONE_WIDTH);
    s.iconSize = static_cast<float>(h::ICON_SIZE);
    s.iconPadding = static_cast<float>(h::ICON_PADDING);
    s.nameIconGap = static_cast<float>(h::NAME_ICON_GAP);
    s.nameRightPad = static_cast<float>(h::NAME_RIGHT_PAD);
    s.nameCategoryGap = static_cast<float>(h::NAME_CATEGORY_GAP);
    s.funcLeftOffset = static_cast<float>(h::FUNC_LEFT_OFFSET);
    s.navWidth = static_cast<float>(h::NAV_WIDTH);
    s.saveWidth = static_cast<float>(h::SAVE_WIDTH);
    s.abWidth = static_cast<float>(h::AB_WIDTH);
    s.settingsWidth = static_cast<float>(h::SETTINGS_WIDTH);
    s.rightInset = static_cast<float>(h::RIGHT_INSET);
    s.gap = static_cast<float>(h::GAP);
    s.renamePadV = static_cast<float>(h::RENAME_PAD_V);
    return s;
}

UiSpacing defaultSpacing()
{
    namespace sp = bws::tokens::shared::spacing;

    UiSpacing s;
    s.xxs = static_cast<float>(sp::XXS);
    s.xs = static_cast<float>(sp::XS);
    s.sm = static_cast<float>(sp::SM);
    s.md = static_cast<float>(sp::MD);
    s.lg = static_cast<float>(sp::LG);
    s.xl = static_cast<float>(sp::XL);
    s.xxl = static_cast<float>(sp::XXL);
    return s;
}

UiWeatherColorSet defaultWeatherColors()
{
    namespace b = bws::tokens::shared::brass;

    UiWeatherColorSet w;
    w.accent = juce::Colour(b::colors::ACCENT);
    w.glow = juce::Colour(b::colors::GLOW);
    w.dim = juce::Colour(b::colors::DIM);
    w.bgMain = juce::Colour(b::colors::BG_MAIN);
    w.bgRaised = juce::Colour(b::colors::BG_RAISED);
    w.bgElevated = juce::Colour(b::colors::BG_ELEVATED);
    w.bgInput = juce::Colour(b::colors::BG_INPUT);
    w.borderLight = juce::Colour(b::colors::BORDER_LIGHT);
    w.borderDark = juce::Colour(b::colors::BORDER_DARK);
    w.textBright = juce::Colour(bws::tokens::shared::ui::TEXT_BRIGHT);
    w.textPrimary = juce::Colour(b::colors::TEXT_PRIMARY);
    w.textSecondary = juce::Colour(b::colors::TEXT_SECONDARY);
    w.textTertiary = juce::Colour(b::colors::TEXT_TERTIARY);
    w.textDisabled = juce::Colour(b::colors::TEXT_DISABLED);
    w.surfacePrimary = juce::Colour(b::surface::PRIMARY);
    w.surfaceHighlight = juce::Colour(b::surface::HIGHLIGHT);
    w.surfaceDark = juce::Colour(b::surface::DARK);
    w.ledBorder = juce::Colour(b::panel::LED_BORDER);
    w.frameDark = juce::Colour(b::surface::FRAME_DARK);
    w.panelDeeper = juce::Colour(b::panel::DEEPER);
    w.panelNearBlack = juce::Colour(b::panel::NEAR_BLACK);
    w.panelDark = juce::Colour(b::panel::DARK_PANEL);
    w.linkBlue = juce::Colour(b::colors::LINK_BLUE);
    namespace u = bws::tokens::shared::ui;
    w.precisionBlue = juce::Colour(u::PRECISION_BLUE);
    w.trimYellow = juce::Colour(u::TRIM_YELLOW);
    w.tickMark = juce::Colour(u::TICK_MARK);
    w.radiusSm = b::radii::SMALL;
    w.radiusMd = b::radii::MEDIUM;
    w.radiusLg = b::radii::LARGE;
    return w;
}

UiKnobTokenSet defaultKnobs()
{
    // Knob role metrics resolve from the generated token struct instances.
    namespace k = bws::tokens::shared::knobs;
    const auto toMetrics = [](const k::UiKnobMetricsTokens& t) -> UiKnobMetrics {
        return {t.diameter,         t.ringThickness, t.arcThickness,   t.pointerLength,
                t.pointerThickness, t.labelPadding,  t.readoutPadding, t.hitPadding};
    };
    UiKnobTokenSet set;
    set.hero = toMetrics(k::HERO);
    set.primary = toMetrics(k::PRIMARY);
    set.secondary = toMetrics(k::SECONDARY);
    return set;
}

UiDropdownTokenSet defaultDropdowns()
{
    // Dropdown variants resolve from the generated token struct instances.
    namespace d = bws::tokens::shared::dropdowns;
    const auto m = [](const d::UiDropdownMetricsTokens& t) -> UiDropdownMetrics {
        return {t.height,          t.padding,       t.arrowBoxWidth,   t.cornerRadius,    t.outlineThickness,
                t.popupFontScale,  t.minPopupWidth, t.rowHeightFactor, t.tickGlyphFactor, t.minTickGlyphRadius,
                t.separatorInsetX, t.shadowAlpha,   t.shadowBlur,      t.shadowOffsetY};
    };
    UiDropdownTokenSet set;
    set.standard = m(d::STANDARD);
    set.utilityMenu = m(d::UTILITY_MENU);
    set.compact = m(d::COMPACT);
    set.overlay = m(d::OVERLAY);
    return set;
}

UiReadoutTokenSet defaultReadouts()
{
    // Readout variants resolve from the generated token struct; the decimal
    // policy is an enum (not a numeric token) supplied per variant.
    namespace r = bws::tokens::shared::readouts;
    const auto m = [](const r::UiReadoutMetricsTokens& t, ReadoutDecimalPolicy decimals) -> UiReadoutMetrics {
        return {t.minWidth, t.height, t.paddingX, t.paddingY, t.cornerRadius, t.outlineThickness, decimals};
    };
    UiReadoutTokenSet set;
    set.standard = m(r::STANDARD, ReadoutDecimalPolicy::Auto2);
    set.compact = m(r::COMPACT, ReadoutDecimalPolicy::Auto1);
    return set;
}

UiTooltipMetrics defaultTooltips()
{
    const auto& t = bws::tokens::shared::tooltips::STANDARD;
    return {t.minWidth,          t.maxWidth,    t.maxWidthRatio, t.fontScale,    t.paddingX,
            t.paddingY,          t.offsetX,     t.offsetY,       t.minTextWidth, t.parentInset,
            t.cornerRadiusScale, t.shadowAlpha, t.shadowBlur,    t.shadowOffsetY};
}

UiSecondaryKnobStyle defaultSecondaryKnobStyle()
{
    UiSecondaryKnobStyle style;
    style.warmth = 1.0f;
    style.depth = 1.0f;
    style.arcEnergy = 1.12f;
    style.readoutEmphasis = 1.0f;
    return style;
}

UiTabTokenSet defaultTabs()
{
    const auto& t = bws::tokens::shared::tabs::STANDARD;
    UiTabTokenSet set;
    set.standard = {t.height, t.paddingX, t.cornerRadius, t.outlineThickness};
    return set;
}

UiShellLayoutTokens defaultShellLayout()
{
    UiShellLayoutTokens layout;
    layout.tabPadWidth = defaultSpacing().xl + defaultSpacing().sm;
    return layout;
}

UiEnvelopeTokens defaultEnvelope()
{
    namespace t = bws::tokens::shared::envelope::trace;

    UiEnvelopeTokens env;
    env.trace.cold = juce::Colour(t::COLD);
    env.trace.hot = juce::Colour(t::HOT);
    return env;
}

UiDynamicsWashTokens defaultDynamicsWash()
{
    namespace t = bws::tokens::shared::dynamics_wash;

    UiDynamicsWashTokens w;
    w.stableMid = juce::Colour(t::STABLE_MID);
    w.working = juce::Colour(t::WORKING);
    w.heavy = juce::Colour(t::HEAVY);
    return w;
}

UiEnvelopeChromeTokens defaultEnvelopeChrome()
{
    namespace t = bws::tokens::shared::envelope_chrome;

    UiEnvelopeChromeTokens c;
    c.panel = juce::Colour(t::PANEL);
    c.outline = juce::Colour(t::OUTLINE);
    c.grid = juce::Colour(t::GRID);
    c.gridFill = juce::Colour(t::GRID_FILL);
    c.rangeFloor = juce::Colour(t::RANGE_FLOOR);
    c.autoReleaseBand = juce::Colour(t::AUTO_RELEASE_BAND);
    c.curve = juce::Colour(t::CURVE);
    c.curveGlow = juce::Colour(t::CURVE_GLOW);
    c.markerBase = juce::Colour(t::MARKER_BASE);
    c.threshold = juce::Colour(t::THRESHOLD);
    return c;
}

UiColorTokens defaultColors()
{
    namespace sc_bg = bws::tokens::shared::bg;
    namespace sc_txt = bws::tokens::shared::text;
    namespace sc_bdr = bws::tokens::shared::border;
    namespace sc_acc = bws::tokens::shared::accent;
    namespace sc_sem = bws::tokens::shared::semantic;

    UiColorTokens c;
    c.bg0 = juce::Colour(sc_bg::BASE);
    c.bg1 = juce::Colour(sc_bg::SURFACE);
    c.bg2 = juce::Colour(sc_bg::ELEVATED);
    c.text0 = juce::Colour(sc_txt::PRIMARY);
    c.text1 = juce::Colour(sc_txt::SECONDARY);
    c.textDisabled = juce::Colour(sc_txt::DISABLED).withAlpha(bws::tokens::shared::opacity::ui_theme::TEXT_DISABLED);
    c.outline0 = juce::Colour(sc_bdr::OUTLINE);
    c.accent0 = juce::Colour(sc_acc::PRIMARY);
    c.accent1 = juce::Colour(sc_acc::SECONDARY).brighter(0.12f);
    c.ok = juce::Colour(sc_sem::OK);
    c.warn = juce::Colour(sc_sem::WARN);
    c.danger = juce::Colour(sc_sem::DANGER);
    c.glowAccent = c.accent0.withAlpha(bws::tokens::shared::opacity::ui_theme::GLOW_ACCENT);
    return c;
}

UiSurfaceTokens defaultSurfaces()
{
    namespace sv = bws::tokens::shared::surfaces;
    UiSurfaceTokens s;
    s.radiusSm = sv::RADIUS_SM;
    s.radiusMd = sv::RADIUS_MD;
    s.radiusLg = sv::RADIUS_LG;
    s.strokeHairline = sv::STROKE_HAIRLINE;
    s.strokeThin = sv::STROKE_THIN;
    s.strokeMed = sv::STROKE_MED;
    s.elevationFlat = sv::ELEVATION_FLAT;
    s.elevationRaised = sv::ELEVATION_RAISED;
    s.elevationHero = sv::ELEVATION_HERO;
    s.finish = UiPanelFinish::BrushedMetal;
    return s;
}

float densityScale(UiDensity d) noexcept
{
    return d == UiDensity::Compact ? 0.92f : 1.0f;
}

juce::Font makeFont(const UiTypographySpec& spec, float scale, float density)
{
    const float h = spec.height * juce::jmax(0.5f, density * scale);
    const bool bold = spec.weight == FontWeight::semiBold || spec.weight == FontWeight::bold;

    // Use the embedded typeface from bw::Fonts SSoT - never look up by name string.
    // bold → title (Inter SemiBold), plain → body (Inter Regular).
    juce::Font f = bold ? bw::Fonts::title(h) : bw::Fonts::body(h);
    f.setExtraKerningFactor(spec.tracking);
    return f;
}

template <typename T>
T scaled(const T& src, float scale)
{
    T out = src;
    out.xxs *= scale;
    out.xs *= scale;
    out.sm *= scale;
    out.md *= scale;
    out.lg *= scale;
    out.xl *= scale;
    out.xxl *= scale;
    return out;
}

UiSurfaceTokens scaled(const UiSurfaceTokens& src, float scale)
{
    UiSurfaceTokens out = src;
    out.radiusSm *= scale;
    out.radiusMd *= scale;
    out.radiusLg *= scale;
    out.strokeHairline *= scale;
    out.strokeThin *= scale;
    out.strokeMed *= scale;
    out.elevationFlat *= scale;
    out.elevationRaised *= scale;
    out.elevationHero *= scale;
    return out;
}
} // namespace

UiThemeBase defaultUiThemeBase()
{
    UiThemeBase base;
    base.colors = defaultColors();
    base.spacing = defaultSpacing();
    base.surfaces = defaultSurfaces();
    base.typography = defaultTypography();
    base.header = defaultHeader();
    base.weatherColors = defaultWeatherColors();
    base.secondaryKnobStyle = defaultSecondaryKnobStyle();
    base.knobs = defaultKnobs();
    base.dropdowns = defaultDropdowns();
    base.tooltips = defaultTooltips();
    base.readouts = defaultReadouts();
    base.tabs = defaultTabs();
    base.shellLayout = defaultShellLayout();
    base.envelope = defaultEnvelope();
    base.dynamicsWash = defaultDynamicsWash();
    base.envelopeChrome = defaultEnvelopeChrome();
    base.density = UiDensity::Standard;
    base.heroEmphasis = 1.05f;
    return base;
}

UiThemeResolved resolveTheme(const UiThemeBase& base, const UiThemeOverride& patch)
{
    UiThemeResolved out;
    out.colors = base.colors;
    if (patch.colors)
        out.colors = *patch.colors;
    if (patch.accent0)
        out.colors.accent0 = *patch.accent0;
    if (patch.accent1)
        out.colors.accent1 = *patch.accent1;
    else
        out.colors.accent1 = out.colors.accent0.brighter(0.14f);
    out.colors.glowAccent = out.colors.accent0.withAlpha(bws::tokens::shared::opacity::ui_theme::GLOW_ACCENT);

    out.surfaces = base.surfaces;
    if (patch.panelFinish)
        out.surfaces.finish = *patch.panelFinish;

    out.knobs = base.knobs;
    out.dropdowns = base.dropdowns;
    out.tooltips = base.tooltips;
    out.readouts = base.readouts;
    out.tabs = base.tabs;
    out.shellLayout = base.shellLayout;
    out.typography = base.typography;
    if (patch.typography)
        out.typography = *patch.typography;

    out.header = base.header;
    if (patch.header)
        out.header = *patch.header;

    out.weatherColors = base.weatherColors;
    if (patch.weatherColors)
        out.weatherColors = *patch.weatherColors;

    out.secondaryKnobStyle = base.secondaryKnobStyle;
    if (patch.secondaryKnobStyle)
        out.secondaryKnobStyle = *patch.secondaryKnobStyle;

    if (patch.shellLayout)
        out.shellLayout = *patch.shellLayout;

    out.envelope = base.envelope;
    if (patch.envelope)
        out.envelope = *patch.envelope;

    out.dynamicsWash = base.dynamicsWash;
    if (patch.dynamicsWash)
        out.dynamicsWash = *patch.dynamicsWash;

    out.envelopeChrome = base.envelopeChrome;
    if (patch.envelopeChrome)
        out.envelopeChrome = *patch.envelopeChrome;

    out.density = patch.density.value_or(base.density);
    out.densityScale = densityScale(out.density);
    out.heroEmphasis = patch.heroEmphasis.value_or(base.heroEmphasis);
    out.spacing = scaled(base.spacing, out.densityScale);
    out.surfaces = scaled(out.surfaces, out.densityScale);
    return out;
}

UiKnobMetrics UiKnobs::metrics(UiKnobRole role, float scale, const UiThemeResolved& theme)
{
    UiKnobMetrics base {};
    switch (role)
    {
    case UiKnobRole::Hero:
        base = theme.knobs.hero;
        break;
    case UiKnobRole::Primary:
        base = theme.knobs.primary;
        break;
    case UiKnobRole::Secondary:
        base = theme.knobs.secondary;
        break;
    }

    const float s =
        juce::jmax(0.5f, theme.densityScale * scale) * (role == UiKnobRole::Hero ? theme.heroEmphasis : 1.0f);
    base.diameter *= s;
    base.ringThickness *= s;
    base.arcThickness *= s;
    base.pointerLength *= s;
    base.pointerThickness *= s;
    base.labelPadding *= s;
    base.readoutPadding *= s;
    base.hitPadding *= s;

    if (role == UiKnobRole::Secondary)
    {
        // Supporting knobs need more aggressive optical compression than simple geometric scaling.
        const float opticalCompression = std::pow(juce::jlimit(0.5f, 1.0f, s), 0.85f);
        base.ringThickness *= opticalCompression * 0.86f;
        base.arcThickness *= opticalCompression * 0.72f;
        base.pointerLength *= opticalCompression * 0.5f;
        base.pointerThickness *= opticalCompression * 0.58f;
        base.labelPadding *= juce::jmin(1.0f, opticalCompression * 0.9f);
        base.readoutPadding *= juce::jmin(1.0f, opticalCompression * 0.82f);
    }

    return base;
}

UiDropdownMetrics UiDropdowns::metrics(UiDropdownRole role, float scale, const UiThemeResolved& theme)
{
    UiDropdownMetrics m;
    switch (role)
    {
    case UiDropdownRole::Overlay:
        m = theme.dropdowns.overlay;
        break;
    case UiDropdownRole::UtilityMenu:
        m = theme.dropdowns.utilityMenu;
        break;
    case UiDropdownRole::Compact:
        m = theme.dropdowns.compact;
        break;
    case UiDropdownRole::Standard:
        m = theme.dropdowns.standard;
        break;
    }
    const float s = juce::jmax(0.5f, theme.densityScale * scale);
    m.height *= s;
    m.padding *= s;
    m.arrowBoxWidth *= s;
    m.cornerRadius *= s;
    m.outlineThickness *= s;
    m.minPopupWidth *= s;
    m.minTickGlyphRadius *= s;
    m.separatorInsetX *= s;
    m.shadowBlur *= s;
    m.shadowOffsetY *= s;
    return m;
}

UiTooltipMetrics UiTooltips::metrics(float scale, const UiThemeResolved& theme)
{
    UiTooltipMetrics m = theme.tooltips;
    const float s = juce::jmax(0.5f, theme.densityScale * scale);
    m.minWidth *= s;
    m.maxWidth *= s;
    m.paddingX *= s;
    m.paddingY *= s;
    m.offsetX *= s;
    m.offsetY *= s;
    m.minTextWidth *= s;
    m.parentInset *= s;
    m.shadowBlur *= s;
    m.shadowOffsetY *= s;
    return m;
}

UiTabMetrics UiTabsTokens::metrics(UiTabRole role, float scale, const UiThemeResolved& theme)
{
    UiTabMetrics m = theme.tabs.standard;
    juce::ignoreUnused(role);
    const float s = juce::jmax(0.5f, theme.densityScale * scale);
    m.height *= s;
    m.paddingX *= s;
    m.cornerRadius *= s;
    m.outlineThickness *= s;
    return m;
}

UiReadoutMetrics UiReadouts::metrics(UiReadoutRole role, float scale, const UiThemeResolved& theme)
{
    UiReadoutMetrics m = (role == UiReadoutRole::Compact) ? theme.readouts.compact : theme.readouts.standard;
    const float s = juce::jmax(0.5f, theme.densityScale * scale);
    m.minWidth *= s;
    m.height *= s;
    m.paddingX *= s;
    m.paddingY *= s;
    m.cornerRadius *= s;
    m.outlineThickness *= s;
    return m;
}

} // namespace bws::ui

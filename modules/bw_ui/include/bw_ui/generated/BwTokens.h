// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
//
// BwTokens.h - Auto-generated from bds/tokens/products/*.tokens.json
// DO NOT EDIT - regenerate with: python3 bds/tools/generate_tokens.py

#pragma once

#include <cstdint>

namespace bws::tokens {

namespace barometer {
    namespace bg {
        constexpr uint32_t BASE = 0xff1a1a1a;
        constexpr uint32_t READOUT = 0xff252525;
        constexpr uint32_t SLIDER_TRACK = 0xff2a2a2a;
        constexpr uint32_t PHASE_INACTIVE = 0xff3a3a3a;
    }
    namespace text {
        constexpr uint32_t TITLE = 0xfffffef0;
        constexpr uint32_t READOUT = 0xfffffef0;
        constexpr uint32_t BRANDING = 0xff888888;
        constexpr uint32_t METER_LABEL = 0xffaaaaaa;
        constexpr uint32_t IN_OUT_LABEL = 0xff999999;
        constexpr uint32_t TICK_LABEL = 0xff777777;
        constexpr uint32_t PHASE_BORDER = 0xff555555;
        constexpr uint32_t OUTLINE_BORDER = 0xff444444;
    }
    namespace accent {
        constexpr uint32_t BRASS = 0xffd4af37;
        constexpr uint32_t BRASS_ACTIVE = 0xffd4a84b;
        constexpr uint32_t LINK_BLUE = 0xff4a90d9;
    }
    namespace meter {
        constexpr uint32_t GREEN = 0xff00aa00;
        constexpr uint32_t YELLOW = 0xffffaa00;
        constexpr uint32_t RED = 0xffff3333;
        constexpr uint32_t CLIP = 0xffff0000;
    }
    namespace slider {
        constexpr uint32_t THUMB_BRIGHT = 0xffe5c47a;
        constexpr uint32_t THUMB_DARK = 0xffa87f3f;
        constexpr uint32_t THUMB_RING = 0xff7a6545;
        constexpr uint32_t TRACK_SHADOW = 0x40000000;
    }
    namespace indicator {
        constexpr uint32_t SOLO_YELLOW = 0xffffcc00;
        constexpr uint32_t PHASE_ACTIVE = 0xffd4a84b;
        constexpr uint32_t PHASE_TEXT = 0xff888888;
        constexpr uint32_t DC_FILTER_ACTIVE = 0xff4a90d9;
        constexpr uint32_t LUFS_CLIP_INDICATOR = 0xffe05050;
    }
    struct MeterGradientStop { float position; uint32_t color; };
    constexpr MeterGradientStop METER_GRADIENT[] = {
        { 0.0f, 0xff00aa00 },
        { 0.5f, 0xffffaa00 },
        { 0.91f, 0xffffaa00 },
        { 1.0f, 0xffff3333 },
    };
    namespace spacing {
        constexpr int UNIT = 24;
        constexpr int TOP_MARGIN = 24;
        constexpr int TITLE_HEIGHT = 20;
        constexpr int TITLE_TO_PRESET_GAP = 15;
        constexpr int PRESET_TO_DIAL_GAP = 24;
        constexpr int DIAL_TO_CONTROLS_GAP = 24;
        constexpr int CONTROLS_TO_BRAND_GAP = 15;
        constexpr int BOTTOM_MARGIN = 15;
        constexpr int PHASE_BUTTON_GAP = 8;
        constexpr int CONTROLS_GAP = 24;
        constexpr int LABEL_TO_CONTROL_GAP = 8;
        constexpr int METER_DIAL_GAP = 14;
    }
    namespace radii {
        constexpr float SM = 1.5f;
        constexpr int MD = 3;
        constexpr int LG = 4;
    }
    namespace typography {
        namespace sizes {
            constexpr int TINY = 8;
            constexpr int SMALL = 9;
            constexpr int BODY = 10;
            constexpr int DISPLAY = 12;
            constexpr int TITLE = 16;
        }
    }
    namespace layout {
        constexpr int PLUGIN_WIDTH = 408;
        constexpr int HERO_DIAL_SIZE = 200;
        constexpr int PHASE_BUTTON_SIZE = 24;
        constexpr int PHASE_GROUP_WIDTH = 120;
        constexpr int BALANCE_SLIDER_WIDTH = 120;
        constexpr int BALANCE_SLIDER_HEIGHT = 24;
        constexpr int CONTROL_CONTAINER_WIDTH = 120;
        constexpr int CONTROLS_GROUP_WIDTH = 264;
        constexpr int METER_BAR_WIDTH = 8;
        constexpr int METER_GAP = 3;
        constexpr int METER_WIDTH = 19;
        constexpr int METER_HEIGHT = 160;
        constexpr int PRESET_DROPDOWN_WIDTH = 140;
        constexpr int PRESET_DROPDOWN_HEIGHT = 22;
        constexpr int AB_BUTTON_WIDTH = 52;
        constexpr int AB_BUTTON_HEIGHT = 22;
        constexpr int DC_FILTER_BUTTON_WIDTH = 36;
        constexpr int DC_FILTER_BUTTON_HEIGHT = 22;
        constexpr int SETTINGS_BUTTON_SIZE = 20;
        constexpr int TAB_BUTTON_WIDTH = 54;
        constexpr int TAB_BUTTON_HEIGHT = 18;
        constexpr int TAB_BUTTON_GAP = 4;
        constexpr int BRAND_HEIGHT = 12;
        constexpr int LABEL_HEIGHT = 14;
        constexpr int SETTINGS_BUTTON_RIGHT_MARGIN = 14;
        constexpr int SETTINGS_BUTTON_TOP_MARGIN = 14;
        constexpr int PRESET_DROPDOWN_TOP_GAP = 6;
        constexpr int AB_BUTTON_ABOVE_METER_GAP = 8;
        constexpr int VALUE_LABEL_HEIGHT = 14;
        constexpr int VALUE_TO_SLIDER_GAP = 8;
        constexpr int SLIDER_LABEL_SPACE = 14;
        constexpr int LUFS_READOUT_HEIGHT = 58;
        constexpr int CORRELATION_METER_HEIGHT = 20;
        constexpr int CONTROLS_ROW_HEIGHT = 24;
        constexpr float MIN_SCALE = 0.75f;
        constexpr float MAX_SCALE = 1.5f;
    }
    namespace meter {
        constexpr float DB_MIN = -60.0f;
        constexpr float DB_MAX = 6.0f;
        constexpr float CORNER_RADIUS_LG = 3.0f;
        constexpr float CORNER_RADIUS_MD = 2.0f;
        constexpr float CORNER_RADIUS_SM = 1.5f;
        constexpr float STROKE_WIDTH_THIN = 1.0f;
        constexpr float STROKE_WIDTH_MED = 1.5f;
        constexpr float TICK_STROKE_WIDTH = 1.0f;
        constexpr float SOLO_DOT_SM = 2.0f;
        constexpr float SOLO_DOT_MD = 4.0f;
        constexpr float SOLO_DOT_LG = 8.0f;
    }
    namespace opacity {
        constexpr float METER_BAR_BG = 0.3f;
        constexpr float METER_HOVER = 0.8f;
        constexpr float METER_NORMAL = 0.5f;
        constexpr float METER_TEXT_PRIMARY = 0.95f;
        constexpr float METER_TEXT_SECONDARY = 0.85f;
        constexpr float SLIDER_HIGHLIGHT = 0.25f;
        constexpr float SLIDER_SHADOW = 0.15f;
        constexpr float SLIDER_RING = 0.3f;
        constexpr float SLIDER_READOUT = 0.9f;
        constexpr float METER_HOVER_GLOW = 0.3f;
        constexpr float SEPARATOR_LINE = 0.3f;
        constexpr float READOUT_DIM = 0.55f;
        constexpr float STATUS_BADGE_FILL = 0.2f;
        constexpr float SPARKLINE_STROKE = 0.4f;
        constexpr float SIGNAL_LOCK_DOT_EMPTY = 0.6f;
    }
    namespace animation {
        constexpr int FADE_IN_DURATION_MS = 150;
        constexpr int FADE_OUT_DURATION_MS = 300;
        constexpr float FADE_IN_MULTIPLIER = 5.0f;
        constexpr float FADE_OUT_MULTIPLIER = 3.0f;
        constexpr int TIMER_INTERVAL_MS = 16;
        constexpr int PEAK_HOLD_TICKS = 45;
        constexpr float METER_FALL_RATE_DB = 0.7f;
        constexpr float GAIN_RAMP_SEC = 0.02f;
        constexpr float RMS_WINDOW_SEC = 0.3f;
    }
    namespace paint {
        constexpr float SLIDER_CORNER_RADIUS = 4.0f;
        constexpr float SLIDER_STROKE_WIDTH = 1.0f;
        constexpr float SLIDER_RING_BORDER = 1.5f;
        constexpr float SLIDER_RING_STROKE = 0.5f;
    }
} // namespace barometer

namespace rendering {
    namespace shadows {
        struct ShadowToken { int offsetX; int offsetY; float blur; uint32_t color; };
        constexpr ShadowToken ELEVATION_NONE = { 0, 0, 0.0f, 0x00000000 };
        constexpr ShadowToken ELEVATION_LOW = { 0, 2, 4.0f, 0x1a000000 };
        constexpr ShadowToken ELEVATION_MEDIUM = { 0, 4, 8.0f, 0x26000000 };
        constexpr ShadowToken ELEVATION_HIGH = { 0, 8, 16.0f, 0x33000000 };
        constexpr ShadowToken ELEVATION_EXTREME = { 0, 16, 32.0f, 0x40000000 };
        constexpr ShadowToken INNER_SUBTLE = { 0, 1, 2.0f, 0x0d000000 };
        constexpr ShadowToken INNER_MEDIUM = { 0, 2, 4.0f, 0x1a000000 };
        constexpr ShadowToken INNER_STRONG = { 0, 4, 8.0f, 0x26000000 };
        constexpr ShadowToken GLOW_SUBTLE = { 0, 0, 4.0f, 0x33ffffff };
        constexpr ShadowToken GLOW_MEDIUM = { 0, 0, 8.0f, 0x4dffffff };
        constexpr ShadowToken GLOW_STRONG = { 0, 0, 16.0f, 0x66ffffff };
        constexpr ShadowToken COLORED_GLOW_ACCENT = { 0, 0, 12.0f, 0x662563eb };
        constexpr ShadowToken COLORED_GLOW_SUCCESS = { 0, 0, 12.0f, 0x6610b981 };
        constexpr ShadowToken COLORED_GLOW_WARNING = { 0, 0, 12.0f, 0x66f59e0b };
        constexpr ShadowToken COLORED_GLOW_DANGER = { 0, 0, 12.0f, 0x66ef4444 };
        constexpr ShadowToken COLORED_GLOW_SUBTLE = { 0, 0, 8.0f, 0x4d1a1a1a };
        namespace elevation_none {
            constexpr int OFFSET_X = 0;
            constexpr int OFFSET_Y = 0;
            constexpr float BLUR = 0.0f;
            constexpr uint32_t COLOR = 0x00000000;
        }
        namespace elevation_low {
            constexpr int OFFSET_X = 0;
            constexpr int OFFSET_Y = 2;
            constexpr float BLUR = 4.0f;
            constexpr uint32_t COLOR = 0x1a000000;
        }
        namespace elevation_medium {
            constexpr int OFFSET_X = 0;
            constexpr int OFFSET_Y = 4;
            constexpr float BLUR = 8.0f;
            constexpr uint32_t COLOR = 0x26000000;
        }
        namespace elevation_high {
            constexpr int OFFSET_X = 0;
            constexpr int OFFSET_Y = 8;
            constexpr float BLUR = 16.0f;
            constexpr uint32_t COLOR = 0x33000000;
        }
        namespace elevation_extreme {
            constexpr int OFFSET_X = 0;
            constexpr int OFFSET_Y = 16;
            constexpr float BLUR = 32.0f;
            constexpr uint32_t COLOR = 0x40000000;
        }
        namespace inner_subtle {
            constexpr int OFFSET_X = 0;
            constexpr int OFFSET_Y = 1;
            constexpr float BLUR = 2.0f;
            constexpr uint32_t COLOR = 0x0d000000;
        }
        namespace inner_medium {
            constexpr int OFFSET_X = 0;
            constexpr int OFFSET_Y = 2;
            constexpr float BLUR = 4.0f;
            constexpr uint32_t COLOR = 0x1a000000;
        }
        namespace inner_strong {
            constexpr int OFFSET_X = 0;
            constexpr int OFFSET_Y = 4;
            constexpr float BLUR = 8.0f;
            constexpr uint32_t COLOR = 0x26000000;
        }
        namespace glow_subtle {
            constexpr int OFFSET_X = 0;
            constexpr int OFFSET_Y = 0;
            constexpr float BLUR = 4.0f;
            constexpr uint32_t COLOR = 0x33ffffff;
        }
        namespace glow_medium {
            constexpr int OFFSET_X = 0;
            constexpr int OFFSET_Y = 0;
            constexpr float BLUR = 8.0f;
            constexpr uint32_t COLOR = 0x4dffffff;
        }
        namespace glow_strong {
            constexpr int OFFSET_X = 0;
            constexpr int OFFSET_Y = 0;
            constexpr float BLUR = 16.0f;
            constexpr uint32_t COLOR = 0x66ffffff;
        }
        namespace colored_glow_accent {
            constexpr int OFFSET_X = 0;
            constexpr int OFFSET_Y = 0;
            constexpr float BLUR = 12.0f;
            constexpr uint32_t COLOR = 0x662563eb;
        }
        namespace colored_glow_success {
            constexpr int OFFSET_X = 0;
            constexpr int OFFSET_Y = 0;
            constexpr float BLUR = 12.0f;
            constexpr uint32_t COLOR = 0x6610b981;
        }
        namespace colored_glow_warning {
            constexpr int OFFSET_X = 0;
            constexpr int OFFSET_Y = 0;
            constexpr float BLUR = 12.0f;
            constexpr uint32_t COLOR = 0x66f59e0b;
        }
        namespace colored_glow_danger {
            constexpr int OFFSET_X = 0;
            constexpr int OFFSET_Y = 0;
            constexpr float BLUR = 12.0f;
            constexpr uint32_t COLOR = 0x66ef4444;
        }
        namespace colored_glow_subtle {
            constexpr int OFFSET_X = 0;
            constexpr int OFFSET_Y = 0;
            constexpr float BLUR = 8.0f;
            constexpr uint32_t COLOR = 0x4d1a1a1a;
        }
    }
} // namespace rendering

namespace shared {
    namespace bg {
        constexpr uint32_t BASE = 0xff0f172a;
        constexpr uint32_t SURFACE = 0xff1e293b;
        constexpr uint32_t ELEVATED = 0xff0f172a;
    }
    namespace text {
        constexpr uint32_t PRIMARY = 0xff94a3b8;
        constexpr uint32_t SECONDARY = 0xff64748b;
        constexpr uint32_t DISABLED = 0xff64748b;
    }
    namespace border {
        constexpr uint32_t OUTLINE = 0xff334155;
    }
    namespace accent {
        constexpr uint32_t PRIMARY = 0xfff6c344;
        constexpr uint32_t SECONDARY = 0xfff6c344;
    }
    namespace envelope {
        namespace trace {
            constexpr uint32_t COLD = 0xfff6c344;
            constexpr uint32_t HOT = 0xfff6c344;
        }
    }
    namespace dynamics_wash {
        constexpr uint32_t STABLE_MID = 0xff5a9880;
        constexpr uint32_t WORKING = 0xffb89060;
        constexpr uint32_t HEAVY = 0xffc07040;
    }
    namespace envelope_chrome {
        constexpr uint32_t PANEL = 0xff1a1a20;
        constexpr uint32_t OUTLINE = 0xff666666;
        constexpr uint32_t GRID = 0xff666666;
        constexpr uint32_t GRID_FILL = 0xff505868;
        constexpr uint32_t RANGE_FLOOR = 0xffb89060;
        constexpr uint32_t AUTO_RELEASE_BAND = 0xffd4a574;
        constexpr uint32_t CURVE = 0xffd0d4dc;
        constexpr uint32_t CURVE_GLOW = 0xffd4a574;
        constexpr uint32_t MARKER_BASE = 0xffa89968;
        constexpr uint32_t THRESHOLD = 0xffb89060;
    }
    namespace primitive {
        constexpr uint32_t AMBER_600 = 0xffb89060;
    }
    namespace semantic {
        constexpr uint32_t OK = 0xff10b981;
        constexpr uint32_t WARN = 0xfff59e0b;
        constexpr uint32_t DANGER = 0xffef4444;
        constexpr uint32_t WARM_AMBER = 0xffb89060;
    }
    namespace component {
        namespace inline_slider {
            constexpr uint32_t WARM_AMBER = 0xffb89060;
        }
    }
    namespace brass {
        namespace colors {
            constexpr uint32_t ACCENT = 0xffd4af37;
            constexpr uint32_t GLOW = 0x59d4af37;
            constexpr uint32_t DIM = 0x99d4af37;
            constexpr uint32_t BG_MAIN = 0xff1a1a1a;
            constexpr uint32_t BG_RAISED = 0xff2a2a2a;
            constexpr uint32_t BG_ELEVATED = 0xff3a3a3a;
            constexpr uint32_t BG_INPUT = 0xff252525;
            constexpr uint32_t BORDER_LIGHT = 0xff4a4a4a;
            constexpr uint32_t BORDER_DARK = 0xff1a1a1a;
            constexpr uint32_t TEXT_PRIMARY = 0xfffefef0;
            constexpr uint32_t TEXT_SECONDARY = 0xffaaaaaa;
            constexpr uint32_t TEXT_TERTIARY = 0xff888888;
            constexpr uint32_t TEXT_DISABLED = 0xff666666;
            constexpr uint32_t LINK_BLUE = 0xff4a90d9;
        }
        namespace surface {
            constexpr uint32_t PRIMARY = 0xffa89968;
            constexpr uint32_t HIGHLIGHT = 0xffc0a575;
            constexpr uint32_t DARK = 0xff8b7f6a;
            constexpr uint32_t FRAME_DARK = 0xff5a4a2f;
        }
        namespace panel {
            constexpr uint32_t DEEPER = 0xff0f0f0f;
            constexpr uint32_t NEAR_BLACK = 0xff0a0a0a;
            constexpr uint32_t DARK_PANEL = 0xff151515;
            constexpr uint32_t LED_BORDER = 0xff1f1f1f;
        }
        namespace radii {
            constexpr float SMALL = 2.0f;
            constexpr float MEDIUM = 4.0f;
            constexpr float LARGE = 6.0f;
        }
        namespace component_sizes {
            constexpr int AB_TOGGLE_WIDTH = 52;
            constexpr int AB_TOGGLE_HEIGHT = 22;
            constexpr int TOGGLE_SIZE = 24;
            constexpr int TOGGLE_GAP = 8;
            constexpr int SLIDER_WIDTH = 120;
            constexpr int SLIDER_HEIGHT = 24;
            constexpr int PRESET_WIDTH = 140;
            constexpr int PRESET_HEIGHT = 22;
        }
    }
    namespace stability {
        constexpr uint32_t STABLE_GREEN = 0xff7ab892;
        constexpr uint32_t WORKING_BRASS = 0xffd4a574;
        constexpr uint32_t HEAVY_ORANGE = 0xffd97545;
    }
    namespace atmospheric {
        constexpr uint32_t DEEP = 0xff0d0d0d;
        constexpr uint32_t MID = 0xff1a1a1a;
        constexpr uint32_t LIGHT = 0xff2a2a2a;
    }
    namespace equilibrium {
        constexpr uint32_t SIGNAL_RED = 0xffc62828;
        constexpr uint32_t CHROME_SCRIM = 0xcc0d0d0d;
        constexpr uint32_t INDICATOR_HOVER = 0xffc0c0c0;
    }
    namespace brand_text {
        constexpr uint32_t PRIMARY = 0xfff5f0e8;
        constexpr uint32_t SECONDARY = 0xffcccccc;
        constexpr uint32_t TERTIARY = 0xff999999;
        constexpr uint32_t QUATERNARY = 0xff666666;
        constexpr uint32_t DISABLED = 0xff444444;
    }
    namespace brand_border {
        constexpr uint32_t SUBTLE = 0xff333333;
    }
    namespace ui {
        constexpr uint32_t TEXT_BRIGHT = 0xffe0e0e0;
        constexpr uint32_t PRECISION_BLUE = 0xff60c0e0;
        constexpr uint32_t TRIM_YELLOW = 0xffffcc00;
        constexpr uint32_t TICK_MARK = 0xff6a6a6a;
        constexpr uint32_t GAIN_GOLD = 0xffd4a843;
        constexpr uint32_t GAIN_LABEL_CREAM = 0xffeeddcc;
    }
    namespace typography {
        namespace sizes {
            constexpr float MICRO = 7.5f;
            constexpr int TINY = 8;
            constexpr int SMALL = 9;
            constexpr int BODY = 10;
            constexpr int LABEL = 11;
            constexpr int DISPLAY = 12;
            constexpr int HEADING = 13;
            constexpr int TITLE = 14;
            constexpr int HERO = 19;
        }
        namespace tracking {
            constexpr float TIGHT = 0.02f;
            constexpr float NORMAL = 0.04f;
            constexpr float WIDE = 0.08f;
            constexpr float BRAND = 0.12f;
        }
    }
    namespace header {
        constexpr int HEIGHT = 32;
        constexpr int BRAND_ZONE_WIDTH = 120;
        constexpr int ICON_SIZE = 16;
        constexpr int ICON_PADDING = 8;
        constexpr int NAME_ICON_GAP = 4;
        constexpr int NAME_RIGHT_PAD = 8;
        constexpr int NAME_CATEGORY_GAP = 2;
        constexpr int FUNC_LEFT_OFFSET = 10;
        constexpr int NAV_WIDTH = 22;
        constexpr int SAVE_WIDTH = 28;
        constexpr int AB_WIDTH = 36;
        constexpr int SETTINGS_WIDTH = 22;
        constexpr int RIGHT_INSET = 8;
        constexpr int GAP = 6;
        constexpr int RENAME_PAD_V = 4;
    }
    namespace spacing {
        constexpr int XXS = 2;
        constexpr int XS = 4;
        constexpr int SM = 8;
        constexpr int MD = 12;
        constexpr int LG = 16;
        constexpr int XL = 24;
        constexpr int XXL = 32;
    }
    namespace grid_cell {
        constexpr int MIN_SIZE = 48;
        constexpr int MAX_SIZE = 160;
    }
    namespace surfaces {
        constexpr float RADIUS_SM = 4.0f;
        constexpr float RADIUS_MD = 9.0f;
        constexpr float RADIUS_LG = 14.0f;
        constexpr float STROKE_HAIRLINE = 0.9f;
        constexpr float STROKE_THIN = 1.1f;
        constexpr float STROKE_MED = 1.8f;
        constexpr float ELEVATION_FLAT = 0.0f;
        constexpr float ELEVATION_RAISED = 5.0f;
        constexpr float ELEVATION_HERO = 9.0f;
    }
    namespace knobs {
        struct UiKnobMetricsTokens { float diameter; float ringThickness; float arcThickness; float pointerLength; float pointerThickness; float labelPadding; float readoutPadding; float hitPadding; };
        constexpr UiKnobMetricsTokens HERO = { 190.0f, 10.0f, 5.0f, 20.0f, 3.0f, 10.0f, 12.0f, 6.0f };
        constexpr UiKnobMetricsTokens PRIMARY = { 142.0f, 8.0f, 4.0f, 16.0f, 2.5f, 8.0f, 10.0f, 5.0f };
        constexpr UiKnobMetricsTokens SECONDARY = { 118.0f, 7.0f, 3.2f, 14.0f, 2.2f, 7.0f, 9.0f, 4.0f };
        namespace hero {
            constexpr float DIAMETER = 190.0f;
            constexpr float RING_THICKNESS = 10.0f;
            constexpr float ARC_THICKNESS = 5.0f;
            constexpr float POINTER_LENGTH = 20.0f;
            constexpr float POINTER_THICKNESS = 3.0f;
            constexpr float LABEL_PADDING = 10.0f;
            constexpr float READOUT_PADDING = 12.0f;
            constexpr float HIT_PADDING = 6.0f;
        }
        namespace primary {
            constexpr float DIAMETER = 142.0f;
            constexpr float RING_THICKNESS = 8.0f;
            constexpr float ARC_THICKNESS = 4.0f;
            constexpr float POINTER_LENGTH = 16.0f;
            constexpr float POINTER_THICKNESS = 2.5f;
            constexpr float LABEL_PADDING = 8.0f;
            constexpr float READOUT_PADDING = 10.0f;
            constexpr float HIT_PADDING = 5.0f;
        }
        namespace secondary {
            constexpr float DIAMETER = 118.0f;
            constexpr float RING_THICKNESS = 7.0f;
            constexpr float ARC_THICKNESS = 3.2f;
            constexpr float POINTER_LENGTH = 14.0f;
            constexpr float POINTER_THICKNESS = 2.2f;
            constexpr float LABEL_PADDING = 7.0f;
            constexpr float READOUT_PADDING = 9.0f;
            constexpr float HIT_PADDING = 4.0f;
        }
    }
    namespace dropdowns {
        struct UiDropdownMetricsTokens { float height; float padding; float arrowBoxWidth; float cornerRadius; float outlineThickness; float popupFontScale; float minPopupWidth; float rowHeightFactor; float tickGlyphFactor; float minTickGlyphRadius; float separatorInsetX; float shadowAlpha; float shadowBlur; float shadowOffsetY; };
        constexpr UiDropdownMetricsTokens STANDARD = { 36.0f, 12.0f, 28.0f, 8.0f, 1.2f, 0.98f, 128.0f, 1.8f, 0.22f, 1.5f, 8.0f, 0.36f, 10.0f, 2.0f };
        constexpr UiDropdownMetricsTokens UTILITY_MENU = { 30.0f, 8.0f, 22.0f, 7.0f, 1.0f, 0.94f, 112.0f, 1.55f, 0.22f, 1.5f, 8.0f, 0.36f, 10.0f, 2.0f };
        constexpr UiDropdownMetricsTokens COMPACT = { 32.0f, 10.0f, 24.0f, 7.0f, 1.1f, 0.94f, 128.0f, 1.8f, 0.22f, 1.5f, 8.0f, 0.36f, 10.0f, 2.0f };
        constexpr UiDropdownMetricsTokens OVERLAY = { 22.0f, 6.0f, 14.0f, 4.0f, 0.6f, 1.0f, 128.0f, 1.8f, 0.22f, 1.5f, 8.0f, 0.36f, 10.0f, 2.0f };
        namespace standard {
            constexpr float HEIGHT = 36.0f;
            constexpr float PADDING = 12.0f;
            constexpr float ARROW_BOX_WIDTH = 28.0f;
            constexpr float CORNER_RADIUS = 8.0f;
            constexpr float OUTLINE_THICKNESS = 1.2f;
            constexpr float POPUP_FONT_SCALE = 0.98f;
            constexpr float MIN_POPUP_WIDTH = 128.0f;
            constexpr float ROW_HEIGHT_FACTOR = 1.8f;
            constexpr float TICK_GLYPH_FACTOR = 0.22f;
            constexpr float MIN_TICK_GLYPH_RADIUS = 1.5f;
            constexpr float SEPARATOR_INSET_X = 8.0f;
            constexpr float SHADOW_ALPHA = 0.36f;
            constexpr float SHADOW_BLUR = 10.0f;
            constexpr float SHADOW_OFFSET_Y = 2.0f;
        }
        namespace utility_menu {
            constexpr float HEIGHT = 30.0f;
            constexpr float PADDING = 8.0f;
            constexpr float ARROW_BOX_WIDTH = 22.0f;
            constexpr float CORNER_RADIUS = 7.0f;
            constexpr float OUTLINE_THICKNESS = 1.0f;
            constexpr float POPUP_FONT_SCALE = 0.94f;
            constexpr float MIN_POPUP_WIDTH = 112.0f;
            constexpr float ROW_HEIGHT_FACTOR = 1.55f;
            constexpr float TICK_GLYPH_FACTOR = 0.22f;
            constexpr float MIN_TICK_GLYPH_RADIUS = 1.5f;
            constexpr float SEPARATOR_INSET_X = 8.0f;
            constexpr float SHADOW_ALPHA = 0.36f;
            constexpr float SHADOW_BLUR = 10.0f;
            constexpr float SHADOW_OFFSET_Y = 2.0f;
        }
        namespace compact {
            constexpr float HEIGHT = 32.0f;
            constexpr float PADDING = 10.0f;
            constexpr float ARROW_BOX_WIDTH = 24.0f;
            constexpr float CORNER_RADIUS = 7.0f;
            constexpr float OUTLINE_THICKNESS = 1.1f;
            constexpr float POPUP_FONT_SCALE = 0.94f;
            constexpr float MIN_POPUP_WIDTH = 128.0f;
            constexpr float ROW_HEIGHT_FACTOR = 1.8f;
            constexpr float TICK_GLYPH_FACTOR = 0.22f;
            constexpr float MIN_TICK_GLYPH_RADIUS = 1.5f;
            constexpr float SEPARATOR_INSET_X = 8.0f;
            constexpr float SHADOW_ALPHA = 0.36f;
            constexpr float SHADOW_BLUR = 10.0f;
            constexpr float SHADOW_OFFSET_Y = 2.0f;
        }
        namespace overlay {
            constexpr float HEIGHT = 22.0f;
            constexpr float PADDING = 6.0f;
            constexpr float ARROW_BOX_WIDTH = 14.0f;
            constexpr float CORNER_RADIUS = 4.0f;
            constexpr float OUTLINE_THICKNESS = 0.6f;
            constexpr float POPUP_FONT_SCALE = 1.0f;
            constexpr float MIN_POPUP_WIDTH = 128.0f;
            constexpr float ROW_HEIGHT_FACTOR = 1.8f;
            constexpr float TICK_GLYPH_FACTOR = 0.22f;
            constexpr float MIN_TICK_GLYPH_RADIUS = 1.5f;
            constexpr float SEPARATOR_INSET_X = 8.0f;
            constexpr float SHADOW_ALPHA = 0.36f;
            constexpr float SHADOW_BLUR = 10.0f;
            constexpr float SHADOW_OFFSET_Y = 2.0f;
        }
    }
    namespace tooltips {
        struct UiTooltipMetricsTokens { float minWidth; float maxWidth; float maxWidthRatio; float fontScale; float paddingX; float paddingY; float offsetX; float offsetY; float minTextWidth; float parentInset; float cornerRadiusScale; float shadowAlpha; float shadowBlur; float shadowOffsetY; };
        constexpr UiTooltipMetricsTokens STANDARD = { 150.0f, 240.0f, 0.34f, 0.94f, 6.0f, 4.0f, 14.0f, 4.0f, 40.0f, 2.0f, 1.0f, 0.22f, 6.0f, 1.0f };
        namespace standard {
            constexpr float MIN_WIDTH = 150.0f;
            constexpr float MAX_WIDTH = 240.0f;
            constexpr float MAX_WIDTH_RATIO = 0.34f;
            constexpr float FONT_SCALE = 0.94f;
            constexpr float PADDING_X = 6.0f;
            constexpr float PADDING_Y = 4.0f;
            constexpr float OFFSET_X = 14.0f;
            constexpr float OFFSET_Y = 4.0f;
            constexpr float MIN_TEXT_WIDTH = 40.0f;
            constexpr float PARENT_INSET = 2.0f;
            constexpr float CORNER_RADIUS_SCALE = 1.0f;
            constexpr float SHADOW_ALPHA = 0.22f;
            constexpr float SHADOW_BLUR = 6.0f;
            constexpr float SHADOW_OFFSET_Y = 1.0f;
        }
    }
    namespace readouts {
        struct UiReadoutMetricsTokens { float minWidth; float height; float paddingX; float paddingY; float cornerRadius; float outlineThickness; };
        constexpr UiReadoutMetricsTokens STANDARD = { 88.0f, 32.0f, 11.0f, 4.0f, 8.0f, 1.15f };
        constexpr UiReadoutMetricsTokens COMPACT = { 72.0f, 22.0f, 8.0f, 2.0f, 0.0f, 1.0f };
        namespace standard {
            constexpr float MIN_WIDTH = 88.0f;
            constexpr float HEIGHT = 32.0f;
            constexpr float PADDING_X = 11.0f;
            constexpr float PADDING_Y = 4.0f;
            constexpr float CORNER_RADIUS = 8.0f;
            constexpr float OUTLINE_THICKNESS = 1.15f;
        }
        namespace compact {
            constexpr float MIN_WIDTH = 72.0f;
            constexpr float HEIGHT = 22.0f;
            constexpr float PADDING_X = 8.0f;
            constexpr float PADDING_Y = 2.0f;
            constexpr float CORNER_RADIUS = 0.0f;
            constexpr float OUTLINE_THICKNESS = 1.0f;
        }
    }
    namespace readout_derivation {
        // Geometry coefficients applied to readout metrics at paint time.
        constexpr float PILL_INSET = 0.5f;
        constexpr float COMPACT_CORNER_FACTOR = 0.28f;
        constexpr float VERTICAL_SAFETY_COMPACT_FACTOR = 1.25f;
        constexpr float VERTICAL_SAFETY_STANDARD_FACTOR = 2.0f;
        constexpr float SHELL_PADDING_XADD = 5.0f;
        constexpr float MIN_SHELL_WIDTH_FACTOR = 1.15f;
        constexpr float COMPACT_VERTICAL_PAD_FACTOR = 0.5f;
        constexpr float UNDERLINE_WIDTH_FACTOR = 0.52f;
        constexpr float UNDERLINE_YOFFSET_FACTOR = 0.4f;
        constexpr float UNDERLINE_THICKNESS_FACTOR = 0.72f;
        constexpr float SELECTION_CORNER_FACTOR = 0.22f;
        constexpr float CARET_HEIGHT_FACTOR = 0.9f;
    }
    namespace tabs {
        struct UiTabMetricsTokens { float height; float paddingX; float cornerRadius; float outlineThickness; };
        constexpr UiTabMetricsTokens STANDARD = { 34.0f, 12.0f, 8.0f, 1.15f };
        namespace standard {
            constexpr float HEIGHT = 34.0f;
            constexpr float PADDING_X = 12.0f;
            constexpr float CORNER_RADIUS = 8.0f;
            constexpr float OUTLINE_THICKNESS = 1.15f;
        }
    }
    namespace opacity {
        namespace outline {
            constexpr float HAIRLINE = 0.25f;
            constexpr float DIVIDER = 0.35f;
            constexpr float STANDARD = 0.6f;
            constexpr float EMPHASIZED = 0.7f;
            constexpr float BOLD = 0.8f;
        }
        namespace text {
            constexpr float SECTION_HEADER = 0.85f;
        }
        namespace scrollbar {
            constexpr float TRACK = 0.15f;
            constexpr float THUMB = 0.55f;
        }
        namespace dropdown {
            constexpr float SELECTED_ITEM_HIGHLIGHT = 0.15f;
        }
        namespace badge {
            constexpr float NEUTRAL_FILL = 0.88f;
            constexpr float NEUTRAL_BORDER = 0.3f;
            constexpr float NEUTRAL_TEXT = 0.66f;
            constexpr float SUBTLE_FILL = 0.68f;
            constexpr float SUBTLE_BORDER = 0.18f;
            constexpr float SUBTLE_TEXT = 0.48f;
            constexpr float ACCENT_FILL = 0.18f;
            constexpr float ACCENT_BORDER = 0.3f;
            constexpr float ACCENT_TEXT = 0.82f;
            constexpr float STATUS_FILL = 0.28f;
            constexpr float STATUS_BORDER = 0.42f;
            constexpr float STATUS_TEXT = 0.92f;
        }
        namespace chip {
            constexpr float FILL_SELECTED = 0.22f;
            constexpr float FILL_UNSELECTED = 0.88f;
            constexpr float BORDER_SELECTED_FOCUSED = 0.58f;
            constexpr float BORDER_SELECTED_DEFAULT = 0.34f;
            constexpr float BORDER_UNSELECTED_HOVER = 0.3f;
            constexpr float BORDER_UNSELECTED_DEFAULT = 0.18f;
            constexpr float TEXT_BRIGHT = 0.96f;
            constexpr float TEXT_UNSELECTED_HOVER = 0.86f;
            constexpr float TEXT_UNSELECTED_DEFAULT = 0.72f;
            constexpr float ACCENT_STROKE = 0.46f;
            constexpr float DISABLED_FILL_MULT = 0.55f;
            constexpr float DISABLED_BORDER_MULT = 0.5f;
            constexpr float DISABLED_TEXT_MULT = 0.55f;
        }
        namespace card {
            constexpr float SHADOW = 0.22f;
            constexpr float HAIRLINE = 0.012f;
            constexpr float SURFACE = 0.88f;
            constexpr float BORDER = 0.2f;
            constexpr float TEXTURE_STRIPE = 0.05f;
        }
        namespace row {
            constexpr float HOVER_BG = 0.12f;
            constexpr float PRESSED_BG = 0.26f;
            constexpr float HAIRLINE = 0.035f;
            constexpr float SELECTED_INDICATOR = 0.92f;
            constexpr float SECONDARY_TEXT = 0.38f;
            constexpr float FAVOURITE_ENABLED = 0.22f;
            constexpr float FAVOURITE_DISABLED = 0.12f;
            constexpr float TITLE_HIGHLIGHTED = 0.96f;
            constexpr float TITLE_DEFAULT = 0.82f;
        }
        namespace dialog {
            constexpr float BACKDROP = 0.8f;
            constexpr float OUTLINE_SUBTLE = 0.2f;
            constexpr float OUTLINE_ACCENT = 0.46f;
            constexpr float SECONDARY_TEXT = 0.72f;
        }
        namespace search {
            constexpr float HIGHLIGHT = 0.18f;
            constexpr float CARET = 0.95f;
            constexpr float CHROME_FILL = 0.96f;
            constexpr float CHROME_BORDER = 0.22f;
            constexpr float CHROME_FOCUS = 0.56f;
            constexpr float EMPTY_TEXT = 0.28f;
        }
        namespace header {
            constexpr float CHIP_SELECTED = 0.18f;
            constexpr float CHIP_UNSELECTED = 0.78f;
            constexpr float ACCENT_WASH_DIM = 0.12f;
            constexpr float ACCENT_WASH_BRIGHT = 0.45f;
            constexpr float ACCENT_WASH_GLOW = 0.76f;
            constexpr float ACCENT_EDGE = 0.95f;
            constexpr float HAIRLINE = 0.05f;
            constexpr float ROW_HOVER = 0.08f;
            constexpr float CAPTION = 0.24f;
            constexpr float BORDER_OPEN = 0.42f;
            constexpr float BORDER_CLOSED = 0.22f;
            constexpr float AB_DIVIDER_SUSPENDED = 0.18f;
            constexpr float AB_DIVIDER_ACTIVE = 0.35f;
            constexpr float AB_INACTIVE = 0.3f;
            constexpr float AB_SEPARATOR_SUSPENDED = 0.18f;
            constexpr float AB_SEPARATOR_ACTIVE = 0.3f;
            constexpr float NAV_HOVERED = 0.9f;
            constexpr float NAV_IDLE = 0.4f;
            constexpr float SAVE_SUSPENDED = 0.22f;
            constexpr float SAVE_IDLE = 0.84f;
            constexpr float SETTINGS_SUSPENDED = 0.18f;
            constexpr float SETTINGS_HOVERED = 0.72f;
            constexpr float SETTINGS_IDLE = 0.42f;
        }
        namespace gr_meter {
            constexpr float BG_IDLE = 0.85f;
            constexpr float BG_ACTIVE = 0.7f;
            constexpr float ACCENT_GRID = 0.4f;
            constexpr float ACCENT_WASH_SUBTLE = 0.06f;
            constexpr float THRESHOLD = 0.5f;
            constexpr float THRESHOLD_DRAG = 0.8f;
            constexpr float THRESHOLD_HOVER = 0.4f;
            constexpr float TEXT_PRIMARY = 0.9f;
            constexpr float LED_UNLIT = 0.3f;
            constexpr float TRACE_GRID = 0.2f;
            constexpr float TRACE_FILL_TOP = 0.1f;
            constexpr float THRESHOLD_LINE = 0.7f;
            constexpr float PEAK_TEXT = 0.7f;
        }
        namespace knob {
            constexpr float ACCENT_WASH = 0.3f;
        }
        namespace strip {
            constexpr float ACCENT_GLOW_DIM = 0.3f;
            constexpr float ACCENT_GLOW_BRIGHT = 0.5f;
            constexpr float GLOW_DIM = 0.1f;
            constexpr float SELECTED_FILL = 0.15f;
            constexpr float NORMAL_FILL = 0.08f;
            constexpr float SELECTED_TEXT = 0.7f;
            constexpr float BORDER_DEFAULT = 0.4f;
            constexpr float SECONDARY_TEXT = 0.6f;
            constexpr float ACCENT_EDGE = 0.6f;
            constexpr float SEPARATOR = 0.2f;
            constexpr float BG_EXPANSION_BASE = 0.95f;
            constexpr float BORDER_EXPANSION_BASE = 0.4f;
            constexpr float BORDER_EXPANSION_FADED = 0.3f;
        }
        namespace panel {
            constexpr float GLOW_WASH = 0.15f;
            constexpr float ACCENT_EDGE = 0.6f;
            constexpr float BORDER = 0.4f;
            constexpr float BG_EXPANSION_BASE = 0.95f;
            constexpr float BORDER_EXPANSION_BASE = 0.4f;
        }
        namespace lamp {
            constexpr float GLOW_CENTER = 0.3f;
            constexpr float HIGHLIGHT_CAP = 0.3f;
        }
        namespace iris {
            constexpr float BORDER_MULT = 0.5f;
            constexpr float GLOW_PULSE_SUBTLE = 0.35f;
            constexpr float GLOW_PULSE_MEDIUM = 0.7f;
            constexpr float CLOSING_MULT = 0.3f;
            constexpr float LABEL_MULT = 0.9f;
            constexpr float HOVER_GLOW = 0.2f;
            constexpr float ACCENT_FULL = 0.6f;
            constexpr float BORDER_DARK = 0.5f;
            constexpr float RING_MULT = 0.3f;
            constexpr float TICK_MULT = 0.4f;
        }
        namespace curve {
            constexpr float GRADIENT_CENTER = 0.15f;
            constexpr float GRADIENT_EDGE = 0.05f;
            constexpr float STROKE = 0.3f;
            constexpr float STROKE_BRIGHT = 0.6f;
            constexpr float STROKE_MID = 0.5f;
        }
        namespace window {
            constexpr float DROP_INDICATOR = 0.3f;
            constexpr float ACCENT_BORDER = 0.7f;
            constexpr float BACKDROP_FLASH = 0.7f;
            constexpr float BACKDROP_IDLE = 0.6f;
            constexpr float ACCENT_PULSE_MULT = 0.9f;
            constexpr float WHITE_BOOST = 0.2f;
            constexpr float TEXT_SECONDARY = 0.7f;
        }
        namespace display {
            constexpr float BORDER_STANDARD = 0.3f;
            constexpr float BORDER_FADED = 0.12f;
            constexpr float ACCENT_WASH = 0.15f;
            constexpr float BORDER_SUBTLE = 0.15f;
            constexpr float ACCENT_DIM = 0.1f;
            constexpr float ACCENT_BRIGHT = 0.5f;
        }
        namespace tab {
            constexpr float FOCUS_RING = 0.74f;
            constexpr float FOCUS_RING_IDLE = 0.62f;
            constexpr float BG_RAISED_MULT = 0.9f;
            constexpr float BORDER_MULT_FOCUSED = 0.74f;
            constexpr float BORDER_MULT_IDLE = 0.42f;
            constexpr float TEXT_MULT_ENABLED = 0.92f;
            constexpr float TEXT_MULT_DISABLED = 0.55f;
        }
        namespace shadow {
            constexpr float SOFT = 0.05f;
            constexpr float DROP = 0.5f;
            constexpr float HIGHLIGHT = 0.15f;
            constexpr float SUBTLE = 0.08f;
            constexpr float SURFACE_HIGH = 0.6f;
            constexpr float SURFACE_SUBTLE = 0.15f;
            constexpr float BG_MULT = 0.95f;
            constexpr float BORDER_MULT = 0.8f;
            constexpr float WHITE_MUTED = 0.6f;
            constexpr float WHITE_STRONG = 0.95f;
        }
        namespace ui_panel {
            constexpr float MATTE_MID = 0.95f;
            constexpr float MATTE_OUTLINE = 0.55f;
            constexpr float STRIPE_VERTICAL = 0.05f;
            constexpr float STRIPE_HORIZONTAL = 0.08f;
            constexpr float BED_GRADIENT_START = 0.06f;
            constexpr float BED_GRADIENT_END = 0.1f;
            constexpr float BED_GRADIENT_MID = 0.02f;
            constexpr float BED_TOP_EDGE = 0.04f;
            constexpr float BED_BOTTOM_SHADE = 0.05f;
        }
        namespace ui_tabs {
            constexpr float FILL_ON = 0.9f;
            constexpr float FILL_OFF = 0.95f;
            constexpr float OUTLINE = 0.7f;
            constexpr float BG = 0.92f;
            constexpr float AMBIENT_OUTLINE = 0.6f;
        }
        namespace ui_button {
            constexpr float GHOST_BASE = 0.9f;
            constexpr float GHOST_HOVER = 0.12f;
            constexpr float GHOST_ACTIVE = 0.2f;
            constexpr float GHOST_OUTLINE = 0.8f;
            constexpr float TEXT_HOVER = 0.1f;
            constexpr float TEXT_PRESSED = 0.16f;
            constexpr float DISABLED_BG = 0.15f;
            constexpr float DISABLED_OUTLINE = 0.35f;
            constexpr float FOCUS_RING = 0.7f;
        }
        namespace ui_toggle {
            constexpr float TRACK_OFF_IDLE = 0.18f;
            constexpr float TRACK_OFF_HOVER = 0.25f;
            constexpr float TRACK_OFF_PRESSED = 0.35f;
            constexpr float OUTLINE_ON = 0.45f;
            constexpr float DISABLED_TRACK = 0.16f;
            constexpr float DISABLED_OUTLINE = 0.18f;
            constexpr float FOCUS_RING = 0.65f;
        }
        namespace ui_readout {
            constexpr float FILL_NORMAL = 0.9f;
            constexpr float FILL_HOVER = 0.94f;
            constexpr float FILL_ACTIVE = 0.97f;
            constexpr float OUTLINE_HOVER = 0.18f;
            constexpr float OUTLINE_ACTIVE = 0.3f;
            constexpr float TEXT_COMPACT_BASE = 0.96f;
            constexpr float UNDERLINE_COMPACT_BASE = 0.18f;
            constexpr float UNDERLINE_HOVER_COEFF = 0.24f;
            constexpr float UNDERLINE_ACTIVE_COEFF = 0.32f;
            constexpr float SELECTION_HIGHLIGHT = 0.24f;
            constexpr float CARET = 0.9f;
            constexpr float PLACEHOLDER = 0.6f;
        }
        namespace tab_bar {
            constexpr float SELECTED_FILL = 0.9f;
            constexpr float PRESSED_FILL_BASE = 0.1f;
            constexpr float PRESSED_FILL_OVERLAY = 0.18f;
            constexpr float OUTLINE = 0.6f;
            constexpr float FOCUS_RING = 0.65f;
        }
        namespace ui_meter {
            constexpr float TRACK_DISABLED = 0.2f;
            constexpr float TRACK_OUTLINE_DEFAULT = 0.55f;
            constexpr float TRACK_OUTLINE_DISABLED = 0.25f;
            constexpr float ACCENT_DISABLED = 0.6f;
            constexpr float ACCENT_SECONDARY = 0.7f;
        }
        namespace ui_slider {
            constexpr float TRACK_BG = 0.85f;
            constexpr float THUMB_OUTLINE = 0.8f;
            constexpr float FILL = 0.95f;
        }
        namespace graph_panel {
            constexpr float OUTLINE = 0.6f;
            constexpr float DIVIDER = 0.35f;
        }
        namespace knob_chrome {
            constexpr float FACE_BG_IDLE = 0.9f;
            constexpr float FACE_BG_LOCKED = 0.82f;
            constexpr float RING_IDLE = 0.8f;
            constexpr float RING_LOCKED = 0.42f;
            constexpr float GUIDE_ARC_IDLE = 0.24f;
            constexpr float GUIDE_ARC_LOCKED = 0.12f;
            constexpr float ACTIVE_ARC_IDLE = 0.9f;
            constexpr float ACTIVE_ARC_LOCKED = 0.26f;
            constexpr float POINTER_IDLE = 1.0f;
            constexpr float POINTER_LOCKED = 0.34f;
        }
        namespace knob_lighting {
            constexpr float HIGHLIGHT_TOP_COEFF = 0.22f;
            constexpr float HIGHLIGHT_MID_COEFF = 0.05f;
            constexpr float SHADOW_TOP_COEFF = 0.035f;
            constexpr float SHADOW_MID_COEFF = 0.14f;
            constexpr float DROP_SHADOW = 0.18f;
        }
        namespace inline_slider {
            constexpr float LABEL_HERO_DISABLED = 0.7f;
            constexpr float LABEL_SUPPORTING_DISABLED = 0.54f;
            constexpr float LABEL_HERO_IDLE = 0.92f;
            constexpr float LABEL_SUPPORTING_IDLE = 0.72f;
            constexpr float LANE_TOP_HERO_DISABLED = 0.74f;
            constexpr float LANE_TOP_SUPPORTING_DISABLED = 0.58f;
            constexpr float LANE_TOP_HERO_IDLE = 0.94f;
            constexpr float LANE_TOP_SUPPORTING_IDLE = 0.76f;
            constexpr float COOL_BASE_DISABLED = 0.08f;
            constexpr float COOL_BASE_HERO = 0.22f;
            constexpr float COOL_BASE_DEFAULT = 0.12f;
            constexpr float COOL_BRIGHT_DISABLED = 0.14f;
            constexpr float COOL_BRIGHT_HERO = 0.34f;
            constexpr float COOL_BRIGHT_DEFAULT = 0.16f;
            constexpr float WARM_ACCENT_DRAGGING = 0.045f;
            constexpr float WARM_ACCENT_IDLE = 0.022f;
            constexpr float TOP_HAIRLINE_DISABLED = 0.05f;
            constexpr float TOP_HAIRLINE_HERO = 0.1f;
            constexpr float TOP_HAIRLINE_DEFAULT = 0.03f;
            constexpr float LOWER_SHADE_DISABLED = 0.08f;
            constexpr float LOWER_SHADE_HERO = 0.18f;
            constexpr float LOWER_SHADE_DEFAULT = 0.05f;
            constexpr float FILL_BASE_COOL_HERO = 0.12f;
            constexpr float FILL_BASE_COOL_DEFAULT = 0.08f;
            constexpr float FILL_BASE_WARM_DRAGGING = 0.34f;
            constexpr float FILL_BASE_WARM_DEFAULT = 0.18f;
            constexpr float FILL_BASE_COOL_2_HERO = 0.28f;
            constexpr float FILL_BASE_COOL_2_DEFAULT = 0.15f;
            constexpr float FILL_BRIGHT_COOL_HERO = 0.16f;
            constexpr float FILL_BRIGHT_COOL_DEFAULT = 0.1f;
            constexpr float FILL_BRIGHT_WARM_DRAGGING = 0.5f;
            constexpr float FILL_BRIGHT_WARM_DEFAULT = 0.28f;
            constexpr float FILL_BRIGHT_COOL_2_HERO = 0.36f;
            constexpr float FILL_BRIGHT_COOL_2_DEFAULT = 0.18f;
            constexpr float HANDLE_BG_COOL_HERO = 0.16f;
            constexpr float HANDLE_BG_COOL_DEFAULT = 0.1f;
            constexpr float HANDLE_ACCENT_WARM_FOCUSED = 0.64f;
            constexpr float HANDLE_ACCENT_WARM_DEFAULT = 0.52f;
            constexpr float HANDLE_ACCENT_COOL_HERO = 0.34f;
            constexpr float HANDLE_ACCENT_COOL_DEFAULT = 0.18f;
        }
        namespace texture {
            constexpr float SCRATCH = 0.3f;
        }
        namespace equilibrium {
            constexpr float LOW_END_BAND = 0.4f;
            constexpr float CALIB_LABEL = 0.6f;
            constexpr float LOCKED_PULSE_HALO = 0.3f;
        }
        namespace editor_recipe {
            constexpr float PEAK_STROKE = 0.7f;
            constexpr float CURVE_STROKE = 0.8f;
        }
        namespace resize_corner {
            constexpr float HOVER = 0.92f;
            constexpr float IDLE = 0.62f;
        }
        namespace about_overlay {
            constexpr float ACCENT_BORDER = 0.6f;
        }
        namespace correlation_meter {
            constexpr float CENTER_LINE = 0.5f;
            constexpr float PEAK_HOLD = 0.8f;
            constexpr float NEEDLE_SHADOW = 0.3f;
            constexpr float PEAK_NEEDLE = 0.5f;
            constexpr float HIST_BAR = 0.8f;
            constexpr float HIST_CENTER_LINE = 0.3f;
            constexpr float SCALE_LABEL = 0.6f;
            constexpr float AXIS_LABEL_DIM = 0.4f;
            constexpr float SCALE_TICK = 0.7f;
            constexpr float ARC_SEGMENT = 0.4f;
            constexpr float BADGE_BG_ACTIVE = 0.2f;
            constexpr float BADGE_BG_IDLE = 0.3f;
            constexpr float BADGE_TEXT_IDLE = 0.4f;
        }
        namespace goniometer {
            constexpr float GRID_AXIS = 0.3f;
            constexpr float GRID_DIAGONAL = 0.2f;
            constexpr float GRID_CIRCLE = 0.15f;
            constexpr float BOUNDARY = 0.4f;
            constexpr float DIAGONAL_LABEL = 0.6f;
            constexpr float DOT_BASE = 0.8f;
            constexpr float LINE_TRACE = 0.7f;
            constexpr float PERSIST_CORE = 0.9f;
            constexpr float PERSIST_GLOW = 0.3f;
            constexpr float BALANCE_BORDER = 0.5f;
            constexpr float BALANCE_LABEL = 0.6f;
        }
        namespace level_meter {
            constexpr float GAIN_OVERLAY_WASH = 0.06f;
            constexpr float TRACK_BORDER = 0.5f;
            constexpr float GRID_LINE = 0.2f;
            constexpr float SCALE_MARKER = 0.5f;
            constexpr float READOUT_BG = 0.5f;
            constexpr float PEAK_LABEL_TEXT = 0.7f;
            constexpr float PEAK_HOLD_TEXT = 0.7f;
            constexpr float GAIN_LINE_DRAG = 0.8f;
            constexpr float GAIN_LINE_HOVER = 0.4f;
            constexpr float GAIN_LINE_IDLE = 0.25f;
            constexpr float GAIN_LABEL_TEXT = 0.9f;
        }
        namespace loudness_meter {
            constexpr float READOUT_SUBLABEL = 0.6f;
            constexpr float METER_LABEL = 0.7f;
            constexpr float SCALE_GRID_LINE = 0.3f;
            constexpr float TARGET_LINE = 0.6f;
            constexpr float HISTORY_TRACE = 0.5f;
            constexpr float SILENCE_DIM = 0.5f;
        }
        namespace spectrum_analyzer {
            constexpr float INPUT_DEFAULT = 0.15f;
            constexpr float OUTPUT_DEFAULT = 0.4f;
            constexpr float TRACE_PATH = 0.5f;
            constexpr float TRACE_FILL = 0.3f;
        }
        namespace waveform {
            constexpr float PANEL_BORDER = 0.5f;
            constexpr float GRID_LINE = 0.15f;
            constexpr float CENTER_LINE = 0.3f;
            constexpr float RIGHT_CHANNEL_TRACE = 0.7f;
            constexpr float TRACE_FILL = 0.2f;
            constexpr float TRACE_STROKE = 0.9f;
            constexpr float TRIGGER_LINE = 0.5f;
            constexpr float ENVELOPE_FILL = 0.4f;
            constexpr float ENVELOPE_STROKE = 0.8f;
            constexpr float LISSAJOUS_CROSSHAIR = 0.2f;
            constexpr float LISSAJOUS_TRACE = 0.7f;
            constexpr float NO_SIGNAL_TEXT = 0.35f;
        }
        namespace arc_knob {
            constexpr float LOCKED_DIM = 0.55f;
            constexpr float HOVER_GLOW = 0.15f;
            constexpr float TRIM_TRACK_DIM = 0.5f;
            constexpr float READOUT_TEXT = 0.95f;
            constexpr float TRIM_PREVIEW_TEXT = 0.7f;
            constexpr float FINE_BADGE_TEXT = 0.85f;
            constexpr float INLINE_EDITOR_BG = 0.95f;
            constexpr float INLINE_EDITOR_BORDER = 0.6f;
            constexpr float INLINE_EDITOR_HIGHLIGHT = 0.4f;
        }
        namespace ui_theme {
            constexpr float TEXT_DISABLED = 0.45f;
            constexpr float GLOW_ACCENT = 0.32f;
        }
        namespace weather_envelope {
            constexpr float PANEL_FILL = 0.5f;
            constexpr float PANEL_OUTLINE = 0.38f;
            constexpr float GRID = 0.08f;
            constexpr float GRID_FILL = 0.1f;
            constexpr float AXIS_LABEL = 0.72f;
            constexpr float RANGE_FLOOR = 0.35f;
            constexpr float AUTO_RELEASE_BAND = 0.08f;
            constexpr float TARGET_CURVE_FILL = 0.045f;
            constexpr float TARGET_CURVE_GLOW = 0.1f;
            constexpr float TARGET_CURVE_STROKE = 0.95f;
            constexpr float SWEEP_CORE = 0.98f;
            constexpr float SWEEP_GLOW = 0.28f;
            constexpr float TRACE_HEAD_HALO = 0.24f;
            constexpr float MARKER_BASE = 0.42f;
            constexpr float MARKER_ACTIVE = 0.85f;
            constexpr float THRESHOLD_MARKER = 0.48f;
            constexpr float THRESHOLD_ACTIVE_BAND = 0.09f;
            constexpr float MARKER_ACTIVE_GLOW_LINE = 0.18f;
            constexpr float GRAB_BAR_ACTIVE_BAND = 0.1f;
            constexpr float GRAB_BAR_TICK_INACTIVE = 0.72f;
            constexpr float LABEL_TEXT_ACTIVE = 0.95f;
            constexpr float LABEL_TEXT_THRESHOLD_IDLE = 0.8f;
            constexpr float LABEL_TEXT_IDLE = 0.86f;
        }
        namespace weather_control_surface {
            constexpr float DISABLED_FILL = 0.72f;
            constexpr float DISABLED_BORDER = 0.55f;
            constexpr float DISABLED_DIVIDER = 0.45f;
            constexpr float DISABLED_TEXT = 0.5f;
            constexpr float SELECTED_BORDER_FOCUSED = 0.95f;
            constexpr float SELECTED_BORDER = 0.82f;
            constexpr float SELECTED_DIVIDER = 0.22f;
            constexpr float HOVERED_BORDER = 0.78f;
            constexpr float FOCUS_RING = 0.68f;
            constexpr float DISABLED_FOCUS_MULT = 0.35f;
            constexpr float TOGGLE_PILL_SELECTED_BORDER_FOCUSED = 0.78f;
            constexpr float TOGGLE_PILL_SELECTED_BORDER = 0.64f;
            constexpr float TOGGLE_PILL_PRESSED_FILL = 0.96f;
            constexpr float TOGGLE_PILL_IDLE_FILL = 0.88f;
            constexpr float TOGGLE_PILL_HOVERED_BORDER = 0.72f;
            constexpr float TOGGLE_PILL_IDLE_BORDER = 0.46f;
            constexpr float TOGGLE_PILL_IDLE_TEXT = 0.94f;
            constexpr float ACTION_PILL_PRESSED_FILL = 0.97f;
            constexpr float ACTION_PILL_IDLE_FILL = 0.9f;
            constexpr float ACTION_PILL_HOVERED_BORDER = 0.74f;
            constexpr float ACTION_PILL_IDLE_BORDER = 0.5f;
            constexpr float ACTION_PILL_TEXT = 0.96f;
            constexpr float UTILITY_PRESSED_FILL = 0.82f;
            constexpr float UTILITY_HOVERED_FILL = 0.68f;
            constexpr float UTILITY_HOVERED_BORDER = 0.32f;
            constexpr float UTILITY_HOVERED_TEXT = 0.98f;
            constexpr float UTILITY_IDLE_TEXT = 0.8f;
            constexpr float SPLIT_COMPARE_PRESSED_FILL = 0.72f;
            constexpr float SPLIT_COMPARE_HOVERED_FILL = 0.52f;
            constexpr float SPLIT_COMPARE_HOVERED_BORDER = 0.34f;
            constexpr float SPLIT_COMPARE_IDLE_BORDER = 0.16f;
            constexpr float SPLIT_COMPARE_HOVERED_TEXT = 0.94f;
            constexpr float SPLIT_COMPARE_IDLE_TEXT = 0.86f;
            constexpr float COMPACT_DISABLED_FILL = 0.52f;
            constexpr float COMPACT_FOCUS_RING = 0.66f;
        }
        namespace transfer_curve {
            constexpr float PLOT_AREA_FILL = 0.5f;
            constexpr float UNITY_LINE = 0.4f;
            constexpr float AXIS_LABEL = 0.7f;
            constexpr float TRAIL_DOT = 0.6f;
            constexpr float DOT_GLOW_OUTER = 0.3f;
            constexpr float DOT_GLOW_INNER = 0.15f;
            constexpr float THRESHOLD_LINE = 0.6f;
            constexpr float RATIO_INDICATOR = 0.8f;
            constexpr float KNEE_GLOW = 0.2f;
            constexpr float POPUP_BG = 0.95f;
            constexpr float DIAGNOSTIC_OVERLAY = 0.5f;
            constexpr float SPECTRUM_INPUT_FILL = 0.08f;
            constexpr float SPECTRUM_INPUT_STROKE = 0.4f;
            constexpr float SPECTRUM_OUTPUT_STROKE = 0.9f;
            constexpr float DIAGNOSTIC_SECTION_LABEL = 0.8f;
            constexpr float COMPACT_VALUE_TEXT = 0.9f;
            constexpr float SPECTRUM_GRID = 0.2f;
            constexpr float SPECTRUM_LABEL = 0.6f;
            constexpr float SPECTRUM_UNIT_LABEL = 0.4f;
            constexpr float MODE_INDICATOR = 0.7f;
            constexpr float CLICK_HINT = 0.4f;
            constexpr float DUAL_DOT_GLOW = 0.1f;
        }
    }
    namespace discovery {
        constexpr uint32_t SHADOW = 0xff05060a;
        constexpr uint32_t HAIRLINE = 0xffd9dee7;
    }
    namespace preset_browser {
        constexpr uint32_t BACKGROUND = 0xff16161a;
        constexpr uint32_t OUTLINE = 0x55ffffff;
        constexpr uint32_t FOCUS_OUTLINE = 0x88ffffff;
    }
    namespace geometry {
        constexpr float STROKE_HALF_PX = 0.5f;
        constexpr float STROKE_FULL_PX = 1.0f;
        constexpr float STROKE_ONE_HALF_PX = 1.5f;
        constexpr float STROKE_THREE_QUARTER_PX = 0.75f;
    }
} // namespace shared

} // namespace bws::tokens

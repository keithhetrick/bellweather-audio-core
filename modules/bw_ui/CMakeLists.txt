cmake_minimum_required(VERSION 3.22)

set(_bws_visibility_cmake "${CMAKE_SOURCE_DIR}/cmake/BwsVisibility.cmake")
if (NOT EXISTS "${_bws_visibility_cmake}")
    set(_bws_visibility_cmake "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/BwsVisibility.cmake")
endif()
if (EXISTS "${_bws_visibility_cmake}")
    include(${_bws_visibility_cmake})
endif()

# ── Token codegen: JSON → C++ constexpr header ──────────────────────────────
# Regenerates BwTokens.h whenever any product token JSON changes.
# Manual: python3 bds/tools/generate_tokens.py --products bds/tokens/products/ \
#           --output-cpp modules/bw_ui/include/bw_ui/generated/BwTokens.h
file(GLOB _bw_token_jsons "${CMAKE_SOURCE_DIR}/bds/tokens/products/*.tokens.json")
add_custom_command(
    OUTPUT "${CMAKE_SOURCE_DIR}/modules/bw_ui/include/bw_ui/generated/BwTokens.h"
    COMMAND python3 "${CMAKE_SOURCE_DIR}/bds/tools/generate_tokens.py"
        --products "${CMAKE_SOURCE_DIR}/bds/tokens/products/"
        --output-cpp "${CMAKE_SOURCE_DIR}/modules/bw_ui/include/bw_ui/generated/BwTokens.h"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/bds/tools/generate_tokens.py"
        ${_bw_token_jsons}
    COMMENT "Generating BwTokens.h from bds/tokens/products/*.tokens.json"
)
add_custom_target(bw_tokens DEPENDS
    "${CMAKE_SOURCE_DIR}/modules/bw_ui/include/bw_ui/generated/BwTokens.h"
)
# ── /Token codegen ──────────────────────────────────────────────────────────

# ── bw_ui_tokens: header-only design-token leaf (generated/ + tokens/ + colors/) ──
# Constexpr tokens, generated header, and static color/material/shadow tables.
# No translation units, so an INTERFACE library: consumers get the headers + the
# generation build-order dependency, and link no objects. The color/material/
# shadow tables type their values as juce::Colour, so juce_graphics is a PUBLIC
# usage requirement.
add_library(bw_ui_tokens INTERFACE)
add_library(bws::bw_ui_tokens ALIAS bw_ui_tokens)
target_include_directories(bw_ui_tokens INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(bw_ui_tokens INTERFACE juce::juce_graphics)
target_compile_features(bw_ui_tokens INTERFACE cxx_std_20)
add_dependencies(bw_ui_tokens bw_tokens)
# ── /bw_ui_tokens ─────────────────────────────────────────────────────────────

# bw_ui is the umbrella target: it owns no translation units of its own - every
# implementation lives in a sub-library below. It is an INTERFACE library that
# re-exports the sub-libraries, so a consumer that links bw_ui pulls the whole
# design system transitively.
add_library(bw_ui INTERFACE)

include(BwsPIC)

# ── Font subsetting ─────────────────────────────────────────────────────────
# Embedded fonts are subset before juce_add_binary_data picks them up.
# Inter ships full Latin + Cyrillic + Greek + Vietnamese + 1000+ symbol glyphs;
# Bellweather UI uses ASCII + Latin Extended + a small set of UI symbols. CJK
# localizations (ja/ko/zh) fall back to system fonts at runtime - no embedded
# CJK font ships, so subsetting Inter/JetBrains-Mono to non-CJK is safe.
# Saves ~750 KB per plugin (1.19 MB → 0.45 MB).
# Subset target + rationale: tools/fonts/subset_ui_fonts.py header.
# Audit:
set(BWS_FONT_SUBSET "tight" CACHE STRING
    "Font subset level: tight (default; drops TrueType hinting - invisible on macOS,
     near-invisible on modern Windows DirectWrite ≥ 13px) | mid (keeps hinting as
     escape hatch if a locale or low-DPI render regression surfaces).
     Visually A/B'd in DAW on 2026-05-30 against Mid - zero perceptible difference.")
set_property(CACHE BWS_FONT_SUBSET PROPERTY STRINGS mid tight)

set(_bw_ui_font_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/stylekit/assets/fonts")
set(_bw_ui_font_subset_dir "${CMAKE_CURRENT_BINARY_DIR}/font_subsets/${BWS_FONT_SUBSET}")
set(_bw_ui_font_subset_script "${CMAKE_SOURCE_DIR}/tools/fonts/subset_ui_fonts.py")
set(_bw_ui_font_source_files
    "${_bw_ui_font_source_dir}/JetBrainsMono/JetBrainsMonoNL-Regular.ttf"
    "${_bw_ui_font_source_dir}/JetBrainsMono/JetBrainsMonoNL-Medium.ttf"
    "${_bw_ui_font_source_dir}/Inter/Inter-Regular.ttf"
    "${_bw_ui_font_source_dir}/Inter/Inter-SemiBold.ttf"
)
set(_bw_ui_font_subset_files
    "${_bw_ui_font_subset_dir}/JetBrainsMonoNL-Regular.ttf"
    "${_bw_ui_font_subset_dir}/JetBrainsMonoNL-Medium.ttf"
    "${_bw_ui_font_subset_dir}/Inter-Regular.ttf"
    "${_bw_ui_font_subset_dir}/Inter-SemiBold.ttf"
)
add_custom_command(
    OUTPUT ${_bw_ui_font_subset_files}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${_bw_ui_font_subset_dir}"
    COMMAND python3 "${_bw_ui_font_subset_script}"
        --source-dir "${_bw_ui_font_source_dir}"
        --output-dir "${_bw_ui_font_subset_dir}"
        --level "${BWS_FONT_SUBSET}"
    DEPENDS
        "${_bw_ui_font_subset_script}"
        ${_bw_ui_font_source_files}
    COMMENT "Subsetting UI fonts (ASCII + Latin Extended + UI symbols)"
    VERBATIM
)

juce_add_binary_data(bw_ui_FontAssets
    HEADER_NAME FontBinaryData.h
    NAMESPACE FontBinaryData
    SOURCES ${_bw_ui_font_subset_files}
)
# ── /Font subsetting ────────────────────────────────────────────────────────

juce_add_binary_data(bw_ui_LocalizationAssets
    HEADER_NAME LocalizationBinaryData.h
    NAMESPACE LocalizationBinaryData
    SOURCES
        resources/localization/en-US.json
        resources/localization/de-DE.json
        resources/localization/es-ES.json
        resources/localization/fr-FR.json
        resources/localization/ja-JP.json
        resources/localization/ko-KR.json
        resources/localization/pt-BR.json
        resources/localization/qps-ploc.json
        resources/localization/zh-CN.json
)

# ── bw_ui_foundation: concrete base (theme, fonts, localization, kernel contracts) ──
# kernel/ + foundation/ + localization/ + internal/ + interaction/ + ambient/
# implementations. Depends down on bw_ui_tokens; owns the Font + Localization
# binary-data (PRIVATE - only the font/localization loaders consume the symbols).
add_library(bw_ui_foundation STATIC
    src/kernel/Theme.cpp
    src/UI/Fonts.cpp
    src/UI/FontManifest.cpp
    src/UI/UiBootstrap.cpp
    src/UI/UiTheme.cpp
    src/ScaledImageCache.cpp
    src/localization/LocalizationManager.cpp
    src/localization/LocalizationHelpers.cpp
    src/UI/OptimizedMouseHandler.cpp
    src/UI/PaintGuard.cpp
    src/interaction/InteractionLogging.cpp
    src/interactions/PhysicsInteractions.cpp
    src/ambient/AmbientEffects.cpp
)
add_library(bws::bw_ui_foundation ALIAS bw_ui_foundation)
target_include_directories(bw_ui_foundation
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(bw_ui_foundation
    PUBLIC
        juce::juce_gui_basics
        juce::juce_graphics
        juce::juce_audio_processors
        juce::juce_opengl
        juce::juce_events
        juce::juce_core
        juce::juce_cryptography
        bws::bw_core
        bws::bw_ports
        bws::bw_ui_tokens
    PRIVATE
        bw_ui_FontAssets
        bw_ui_LocalizationAssets
)
set_property(TARGET bw_ui_foundation PROPERTY CXX_STANDARD 20)
set_property(TARGET bw_ui_foundation PROPERTY CXX_EXTENSIONS OFF)
bws_apply_hidden_visibility(bw_ui_foundation)
bws_apply_pic_if_linux(bw_ui_foundation)
add_dependencies(bw_ui_foundation bw_tokens)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(bw_ui_foundation PRIVATE -Wswitch -Werror=switch)
endif()

# ── bw_ui_rendering: render backends + theme/parameter adapters ──────────────────
# rendering/ (IRenderer + JUCE/Metal backends) + adapters/. Depends down on
# foundation + tokens.
add_library(bw_ui_rendering STATIC
    src/adapters/UiThemeKernelAdapter.cpp
    src/rendering/ShadowRenderer.cpp
    src/rendering/TextureRenderer.cpp
    src/rendering/MaterialRenderer.cpp
)
add_library(bws::bw_ui_rendering ALIAS bw_ui_rendering)
target_include_directories(bw_ui_rendering
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(bw_ui_rendering
    PUBLIC
        bws::bw_ui_foundation
        bws::bw_ui_tokens
        bws::bw_core
        juce::juce_gui_basics
        juce::juce_graphics
        juce::juce_opengl
        juce::juce_events
        juce::juce_core
)
set_property(TARGET bw_ui_rendering PROPERTY CXX_STANDARD 20)
set_property(TARGET bw_ui_rendering PROPERTY CXX_EXTENSIONS OFF)
bws_apply_hidden_visibility(bw_ui_rendering)
bws_apply_pic_if_linux(bw_ui_rendering)
add_dependencies(bw_ui_rendering bw_tokens)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(bw_ui_rendering PRIVATE -Wswitch -Werror=switch)
endif()

# ── bw_ui_controls: base widgets + editor support + windowing ──────────────────
# Components/ (knobs, buttons, sliders, tabs, meters, dropdowns) + Editor/ headers
# + Presets/ + windowing/. Depends down on rendering + foundation + tokens.
add_library(bw_ui_controls STATIC
    src/UI/Components/UiLabel.cpp
    src/UI/Components/UiReadout.cpp
    src/UI/Components/UiKnob.cpp
    src/UI/Components/UiControlTile.cpp
    src/UI/Components/UiLabeledSlider.cpp
    src/UI/Components/UiInlineSlider.cpp
    src/UI/Components/UiButton.cpp
    src/UI/Components/UiSectionHeader.cpp
    src/UI/Components/UiToggle.cpp
    src/UI/Components/UiTabBar.cpp
    src/UI/Components/UiGraphPanel.cpp
    src/UI/Components/UiMeter.cpp
    src/UI/Components/UiHelpPopover.cpp
    src/UI/Components/UiDropdown.cpp
    src/UI/Components/ThemedPopupMenuLookAndFeel.cpp
    src/UI/Components/ThemedTooltipWindow.cpp
    src/UI/Components/UiTabs.cpp
    src/UI/Components/UiPanel.cpp
    src/Components/UiArcKnob.cpp
    src/Components/UiResizeCorner.cpp
    src/Presets/PresetManager.cpp
    src/windowing/HostedWindowGeometry.cpp
)
add_library(bws::bw_ui_controls ALIAS bw_ui_controls)
target_include_directories(bw_ui_controls
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(bw_ui_controls
    PUBLIC
        bws::bw_ui_foundation
        bws::bw_ui_rendering
        bws::bw_ui_tokens
        bws::bw_core
        bws::bw_ports
        juce::juce_gui_basics
        juce::juce_graphics
        juce::juce_opengl
        juce::juce_events
        juce::juce_core
)
if(APPLE)
    target_sources(bw_ui_controls PRIVATE
        src/windowing/HostedWindowGeometryMac.mm
    )
    set_source_files_properties(
        src/windowing/HostedWindowGeometryMac.mm
        PROPERTIES COMPILE_OPTIONS "-fobjc-arc"
    )
    target_link_libraries(bw_ui_controls PRIVATE
        "-framework AppKit"
    )
endif()
set_property(TARGET bw_ui_controls PROPERTY CXX_STANDARD 20)
set_property(TARGET bw_ui_controls PROPERTY CXX_EXTENSIONS OFF)
bws_apply_hidden_visibility(bw_ui_controls)
bws_apply_pic_if_linux(bw_ui_controls)
add_dependencies(bw_ui_controls bw_tokens)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(bw_ui_controls PRIVATE -Wswitch -Werror=switch)
endif()

# ── bw_ui_visualizers: meters, goniometer, transfer-curve, spectral displays ────
# Visualizers/ + visualizers/. Depends down on rendering + foundation + tokens;
# links bw_dsp_metering (the loudness/level meters consume the metering DSP).
add_library(bw_ui_visualizers STATIC
)
add_library(bws::bw_ui_visualizers ALIAS bw_ui_visualizers)
target_include_directories(bw_ui_visualizers
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(bw_ui_visualizers
    PUBLIC
        bws::bw_ui_foundation
        bws::bw_ui_rendering
        bws::bw_ui_tokens
        bws::bw_dsp_metering
        juce::juce_gui_basics
        juce::juce_graphics
        juce::juce_opengl
        juce::juce_events
        juce::juce_core
)
set_property(TARGET bw_ui_visualizers PROPERTY CXX_STANDARD 20)
set_property(TARGET bw_ui_visualizers PROPERTY CXX_EXTENSIONS OFF)
bws_apply_hidden_visibility(bw_ui_visualizers)
bws_apply_pic_if_linux(bw_ui_visualizers)
add_dependencies(bw_ui_visualizers bw_tokens)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(bw_ui_visualizers PRIVATE -Wswitch -Werror=switch)
endif()

# ── Weather instrument layer (four cohesive sub-libraries) ──────────────────────
# A shared helper applies the common target settings to each weather sub-library.
function(_bw_ui_weather_target _tgt)
    add_library(bws::${_tgt} ALIAS ${_tgt})
    target_include_directories(${_tgt}
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_BINARY_DIR}
    )
    target_link_libraries(${_tgt}
        PUBLIC
            ${ARGN}
            juce::juce_gui_basics
            juce::juce_graphics
            juce::juce_opengl
            juce::juce_events
            juce::juce_core
    )
    set_property(TARGET ${_tgt} PROPERTY CXX_STANDARD 20)
    set_property(TARGET ${_tgt} PROPERTY CXX_EXTENSIONS OFF)
    bws_apply_hidden_visibility(${_tgt})
    bws_apply_pic_if_linux(${_tgt})
    add_dependencies(${_tgt} bw_tokens)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${_tgt} PRIVATE -Wswitch -Werror=switch)
    endif()
endfunction()

# weather/containers + weather/layout - panels, strips, the resizable editor shell.
add_library(bw_ui_weather_containers STATIC
    src/weather/CollapsiblePanel.cpp
    src/weather/CondensedStrip.cpp
    src/weather/PrecisionGridLayout.cpp
    src/weather/ResizableEditor.cpp
)
_bw_ui_weather_target(bw_ui_weather_containers
    bws::bw_ui_foundation bws::bw_ui_rendering bws::bw_ui_controls bws::bw_ui_tokens)

# weather/controls - domain-styled knobs, segmented controls, lamp buttons, tabs.
add_library(bw_ui_weather_controls STATIC
    src/weather/WeatherCompactAction.cpp
    src/weather/WeatherComponents.cpp
    src/weather/WeatherKnob.cpp
    src/weather/WeatherLampButton.cpp
    src/weather/WeatherSegmentedControl.cpp
    src/weather/WeatherTabGroup.cpp
)
_bw_ui_weather_target(bw_ui_weather_controls
    bws::bw_ui_foundation bws::bw_ui_rendering bws::bw_ui_controls bws::bw_ui_tokens)

# weather/displays (core) - interactive base, iris aperture, readout strip, about overlay.
add_library(bw_ui_weather_displays_core STATIC
    src/weather/InteractiveDisplay.cpp
    src/weather/IrisAperture.cpp
    src/weather/ReadoutStrip.cpp
)
_bw_ui_weather_target(bw_ui_weather_displays_core
    bws::bw_ui_foundation bws::bw_ui_rendering bws::bw_ui_controls bws::bw_ui_tokens)



# ── /Weather instrument layer ───────────────────────────────────────────────────

# ── bw_ui_preset_system: preset browser, dialogs, library state (the apex) ───────
# preset_system/ + the weather numeric-entry dialog. Depends on weather_displays_core
# (AboutOverlay) + weather_controls + controls + rendering + foundation + tokens, and
# the preset + stateflow infrastructure.
add_library(bw_ui_preset_system STATIC
    src/preset_system/WeatherPresetManager.cpp
    src/preset_system/WeatherPresetLibraryStateStore.cpp
    src/preset_system/WeatherPresetRecordAdapter.cpp
    src/preset_system/WeatherDiscoveryPrimitives.cpp
    src/preset_system/WeatherMessageDialog.cpp
    src/preset_system/ManagePresetsController.cpp
    src/preset_system/WeatherManagePresetsDialog.cpp
    src/preset_system/WeatherSavePresetDialog.cpp
    src/interactions/WeatherNumericEntryDialog.cpp
)
add_library(bws::bw_ui_preset_system ALIAS bw_ui_preset_system)
target_include_directories(bw_ui_preset_system
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(bw_ui_preset_system
    PUBLIC
        bws::bw_ui_foundation
        bws::bw_ui_rendering
        bws::bw_ui_controls
        bws::bw_ui_weather_controls
        bws::bw_ui_weather_displays_core
        bws::bw_ui_tokens
        bws::bw_preset_core
        juce::juce_gui_basics
        juce::juce_graphics
        juce::juce_opengl
        juce::juce_events
        juce::juce_core
)
set_property(TARGET bw_ui_preset_system PROPERTY CXX_STANDARD 20)
set_property(TARGET bw_ui_preset_system PROPERTY CXX_EXTENSIONS OFF)
bws_apply_hidden_visibility(bw_ui_preset_system)
bws_apply_pic_if_linux(bw_ui_preset_system)
add_dependencies(bw_ui_preset_system bw_tokens)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(bw_ui_preset_system PRIVATE -Wswitch -Werror=switch)
endif()

target_include_directories(bw_ui
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(bw_ui
    INTERFACE
        bws::bw_ui_tokens
        bws::bw_ui_foundation
        bws::bw_ui_rendering
        bws::bw_ui_controls
        bws::bw_ui_visualizers
        bws::bw_ui_weather_containers
        bws::bw_ui_weather_controls
        bws::bw_ui_weather_displays_core
        bws::bw_ui_preset_system
)

# --- Linux: JUCE GUI deps (GTK/WebKit) must be visible to the consumer target ---
# The failure you saw:
#   juce_gui_extra.cpp: fatal error: gtk/gtk.h: No such file or directory
# occurs when pkg-config include dirs are not applied to the compile that builds JUCE GUI sources.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(PkgConfig REQUIRED)

    # JUCE on Linux uses GTK for windowing.
    pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-x11-3.0)

    # JUCE uses WebKit2GTK (version varies by Ubuntu runners).
    pkg_check_modules(WEBKITGTK41 QUIET IMPORTED_TARGET webkit2gtk-4.1)
    if(WEBKITGTK41_FOUND)
        set(BWS_WEBKIT_TARGET PkgConfig::WEBKITGTK41)
        set(BWS_WEBKIT_PKG "webkit2gtk-4.1")
    else()
        pkg_check_modules(WEBKITGTK40 REQUIRED IMPORTED_TARGET webkit2gtk-4.0)
        set(BWS_WEBKIT_TARGET PkgConfig::WEBKITGTK40)
        set(BWS_WEBKIT_PKG "webkit2gtk-4.0")
    endif()

    message(STATUS "BWS Linux deps: GTK3 + ${BWS_WEBKIT_PKG}")

    # PUBLIC is intentional: consumers must also link the same GTK/WebKit libs.
    target_link_libraries(bw_ui INTERFACE PkgConfig::GTK3 ${BWS_WEBKIT_TARGET})
    target_link_libraries(bw_ui_foundation PUBLIC PkgConfig::GTK3 ${BWS_WEBKIT_TARGET})
endif()
# --- /Linux deps ---

add_library(bws::bw_ui ALIAS bw_ui)

# The umbrella has no translation units; it carries only the C++ standard as a
# usage requirement. Symbol visibility, PIC, and the enum-exhaustiveness switch
# guard are applied per sub-library above, where the objects are compiled.
target_compile_features(bw_ui INTERFACE cxx_std_20)

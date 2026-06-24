# BwsBarometer - gain staging and loudness metering utility
# Bellweather Studios
#
# Formats: VST3 / AU via JUCE.

bws_add_juce_plugin(BwsBarometer)


target_sources(BwsBarometer PRIVATE
    src/BarometerProcessor.cpp
    src/BarometerProcessor.h
    src/BarometerOptionalHooks.cpp
    src/BarometerOptionalHooks.h
    src/BarometerEditor.cpp
    src/BarometerEditorPaint.cpp
    src/BarometerEditorPresets.cpp
    src/BarometerEditorInteraction.cpp
    src/BarometerEditorSetup.cpp
    src/BarometerEditor.h
    src/BarometerWeatherPresetManager.cpp
    src/BarometerWeatherPresetManager.h
    # Signal Lock demo interface
    src/toy/SignalLockModel.cpp
    src/toy/SignalLockModel.h
    src/toy/SignalLockTypes.h
    src/toy/SignalLockTuning.h
    src/ui/SignalLockView.cpp
    src/ui/SignalLockView.h
    src/ui/BarometerToyOverlay.cpp
    src/ui/BarometerToyOverlay.h
)

# Link Bellweather Studios modules (provides include directories automatically)
target_link_libraries(BwsBarometer
    PRIVATE
        # JUCE modules via the shared aggregator
        bws::juce_shared
        # Bellweather Studios modules
        bws::bw_rt
        # bw_ui sub-library closure (Barometer uses no detachable-window code)
        bws::bw_ui_tokens
        bws::bw_ui_foundation
        bws::bw_ui_rendering
        bws::bw_ui_controls
        bws::bw_ui_visualizers
        bws::bw_ui_weather_containers
        bws::bw_ui_weather_controls
        bws::bw_ui_weather_displays_core
        bws::bw_ui_preset_system
        bws::bw_dsp_core
        bws::bw_dsp_metering
        bws::bw_dsp_processors
        bws::bw_juce_adapters
        # Commercial service adapters are omitted from public library/demo builds.
        bws::bw_toy_shell
)

bws_apply_juce_plugin_defaults(BwsBarometer)

# FEA strict mode - per-source-file via shared helper.
# cmake/BwsStrictFlags.cmake +
# for the vendor-isolation rationale.
bws_apply_strict_to_own_sources(BwsBarometer)


# =============================================================================
# Procedural UI Mode (Asset-Free)
# =============================================================================
# Barometer uses procedural rendering for minimal binary size.

# =============================================================================
# Chain Exporter
# =============================================================================
include(BwsChainExporter)
bws_add_chain_exporter(BwsBarometer)

# =============================================================================
# TESTS - Automated artifact detection and regression tests
# =============================================================================
if(BUILD_TESTING)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
        add_subdirectory(tests)
    endif()
endif()

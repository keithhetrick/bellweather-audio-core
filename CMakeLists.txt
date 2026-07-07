cmake_minimum_required(VERSION 3.22...3.31)

# Pin minimum macOS deployment target before project() so the toolchain adopts it.
# 11.0 (Big Sur) gives broad Intel + Apple-Silicon coverage.
if(APPLE AND NOT DEFINED CACHE{CMAKE_OSX_DEPLOYMENT_TARGET})
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment target")
endif()

# Universal macOS binaries: pass -DBWS_OSX_ARCHITECTURES="x86_64;arm64".
if(APPLE AND DEFINED BWS_OSX_ARCHITECTURES AND NOT BWS_OSX_ARCHITECTURES STREQUAL "")
    set(CMAKE_OSX_ARCHITECTURES "${BWS_OSX_ARCHITECTURES}" CACHE STRING
        "macOS target architectures" FORCE)
endif()

project(bellweather-audio-core VERSION 1.0.1 LANGUAGES C CXX)

if(APPLE)
    enable_language(OBJCXX)

    # Silence ranlib "has no symbols" noise from JUCE's empty headless wrapper TUs
    # (ARA/LV2) without hiding genuine archive issues. Apple-only.
    set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
    set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_C_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
endif()

# MSVC: read sources as UTF-8 and report the active C++ standard accurately.
if(MSVC)
    add_compile_options(/utf-8 /Zc:__cplusplus)
endif()

if(POLICY CMP0175)
    # Keep the 3.22-compatible custom-command behavior for this v1 source
    # release. Revisit when the minimum CMake version rises and the generated
    # token/custom-command call sites can be validated under NEW semantics.
    cmake_policy(SET CMP0175 OLD)
endif()

# Hidden visibility by default; entry points opt out per-target.
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)

# Public-library floor is C++20 (the exported bws:: targets build + consume at C++20).
# The Barometer demo links bw_ui, pinned to C++23 for BwResult interchange, so its
# profile builds at C++23 to keep the standard consistent across that boundary.
if(BWS_BUILD_BAROMETER_PLUGIN)
    set(CMAKE_CXX_STANDARD 23)
else()
    set(CMAKE_CXX_STANDARD 20)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
if(BUILD_TESTING)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# The library surface is the default. Turn ON only when
# explicitly validating the JUCE-backed demo/plugin lane.
option(BWS_BUILD_BAROMETER_PLUGIN "Build the Barometer plugin (requires JUCE)" OFF)

include(BwsVisibility)
include(BwsLinkerFlags)
include(BwsModuleSupport)
include(BwsStrictFlags)

if(BWS_BUILD_BAROMETER_PLUGIN)
    # Plugin identity comes from cmake/registries/plugin_manifest.json.
    include(BwsPluginManifest)
    bws_load_plugin_manifest()
    bws_validate_manifest()
    include(BwsJuceDefaults)
    include(BwsJuceProvision)
endif()

add_subdirectory(modules)

option(BWS_AUDIO_CORE_INSTALL "Install + export the reusable library as a find_package package" ${PROJECT_IS_TOP_LEVEL})
if(BWS_AUDIO_CORE_INSTALL)
    include(BwsInstallExport)
    bws_install_audio_core(
        bw_audio_types bw_core bw_dsp_core bw_dsp_metering bw_dsp_processors
        bw_fft_adapters bw_ports bw_rt bw_state_types)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests/public_surface)
endif()

if(BWS_BUILD_BAROMETER_PLUGIN)
    add_subdirectory(plugins/production)
endif()

# Verifies the framework boundary: JUCE lives in the adapter/plugin layer, not
# in the reusable core modules.
add_test(NAME "Core framework boundary"
         COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/check_framework_boundary.py"
                 --root "${CMAKE_CURRENT_SOURCE_DIR}")
set_tests_properties("Core framework boundary" PROPERTIES LABELS "framework-boundary")

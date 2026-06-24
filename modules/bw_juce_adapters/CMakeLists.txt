cmake_minimum_required(VERSION 3.22)
project(bw_juce_adapters LANGUAGES CXX)

# =============================================================================
# bw_juce_adapters - JUCE adapter layer.
#
# Framework-facing code lives in this module and the plugin targets. Core DSP
# and runtime modules stay below this adapter boundary.
#
# Contents:
# juce::Processor base class for all plugins
#   ValueTreeRetainer  - juce::ValueTree lifetime retainer
#   JuceScheduler      - IScheduler port implementation
#   PlatformAdapter    - system info (OS name, machine ID, SHA256)
#   BufferAdapter      - zero-copy juce::AudioBuffer <-> BufferView bridge
#   JuceOversamplerAdapter - UpsamplerBlock concept adapter
#
# Optional commercial port adapters live in a separate module, keeping this one
# focused on host/framework plumbing.
# =============================================================================

add_library(bw_juce_adapters STATIC
    src/JuceScheduler.cpp
    src/HostEnvironment.cpp
    src/JucePresetFileRepository.cpp
    src/JucePresetJsonCodec.cpp
    src/JucePresetStateBridge.cpp
    src/PluginBusLayoutPolicy.cpp
    src/BwsAudioProcessor.cpp
    src/CaptureSystemInfo.cpp
    src/MakeJuceScheduler.cpp
    src/RetainValueTree.cpp
)

add_library(bws::bw_juce_adapters ALIAS bw_juce_adapters)

include(BwsPIC)
bws_apply_pic_if_linux(bw_juce_adapters)

target_include_directories(bw_juce_adapters
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(bw_juce_adapters
    PUBLIC
        bws::bw_core
        bws::bw_audio_types
        bws::bw_state_types
        bws::bw_ports
        bws::bw_preset_core
        bws::bw_rt
        juce::juce_audio_basics
        juce::juce_audio_processors
        juce::juce_core
        juce::juce_cryptography
        juce::juce_dsp
        juce::juce_events
)

# --- Linux: JUCE GUI deps must be visible wherever JUCE GUI sources compile ---
# bw_juce_adapters can pull in juce_gui_extra via its JUCE-facing adapter surface,
# so Linux GTK/WebKit pkg-config include/link state must propagate here too.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-x11-3.0)

    pkg_check_modules(WEBKITGTK41 QUIET IMPORTED_TARGET webkit2gtk-4.1)
    if(WEBKITGTK41_FOUND)
        set(BWS_JUCE_ADAPTERS_WEBKIT_TARGET PkgConfig::WEBKITGTK41)
    else()
        pkg_check_modules(WEBKITGTK40 REQUIRED IMPORTED_TARGET webkit2gtk-4.0)
        set(BWS_JUCE_ADAPTERS_WEBKIT_TARGET PkgConfig::WEBKITGTK40)
    endif()

    target_link_libraries(bw_juce_adapters
        PUBLIC
            PkgConfig::GTK3
            ${BWS_JUCE_ADAPTERS_WEBKIT_TARGET}
    )
endif()
# --- /Linux deps ---


target_compile_features(bw_juce_adapters PUBLIC cxx_std_20)

include(BwsWarnings)
bws_add_warnings(bw_juce_adapters)

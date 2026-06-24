# cmake/BwsStrictFlags.cmake
#
# Apply Bellweather strict compile flags (currently the Clang Function Effect
# Analysis errors `-Werror=function-effects` and
# `-Werror=perf-constraint-implies-noexcept`) to first-party sources only,
# excluding JUCE module sources that `juce_add_plugin` injects into the same
# target via `target_sources`.
#
# JUCE-touching code lives at the adapter layer (plugin shells +
# bw_juce_adapters); DSP/logic lives in non-JUCE modules. For non-JUCE module
# targets, plain `target_compile_options` is sufficient. For plugin targets
# built via `juce_add_plugin`, the source-file-property pattern below is
# necessary because target-level options also hit the JUCE module .mm/.cpp
# files compiled INTO the plugin target.
#
# Usage:
#   bws_apply_strict_to_own_sources(<target>)
#     - globs ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp + *.h AND
#       ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp + *.h (Barometer uses the
#       capital-S convention) and applies the strict flags as source-file
#       COMPILE_OPTIONS.
#
#   bws_apply_strict_to_own_sources(<target> SOURCES file1.cpp file2.cpp ...)
#     - applies to an explicit source list, skipping the glob.
#
# Activation is gated by the option BWS_ENABLE_FEA_STRICT (declared at the
# superbuild root). When OFF, this function is a no-op so callers can invoke
# it unconditionally.

function(bws_apply_strict_to_own_sources TARGET)
    if(NOT BWS_ENABLE_FEA_STRICT)
        return()
    endif()

    if(NOT TARGET ${TARGET})
        message(WARNING "bws_apply_strict_to_own_sources: target ${TARGET} does not exist")
        return()
    endif()

    cmake_parse_arguments(ARG "" "" "SOURCES" ${ARGN})

    if(NOT ARG_SOURCES)
        file(GLOB_RECURSE ARG_SOURCES
            CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h")
    endif()

    set_source_files_properties(${ARG_SOURCES} PROPERTIES
        COMPILE_OPTIONS
        "-Werror=function-effects;-Werror=perf-constraint-implies-noexcept")
endfunction()

# cmake/BwsLinkerFlags.cmake
# =============================================================================
# BwsLinkerFlags - Release build linker optimization flags.
#
# Separate from BwsVisibility.cmake by design:
#   BwsVisibility  = compiler flags (symbol visibility, -fvisibility=hidden)
#   BwsLinkerFlags = linker flags   (dead stripping, LTO-related overrides, etc.)
#
# Usage:
#   bws_apply_plugin_linker_flags(<plugin_target>)
#
# Wired automatically via bws_apply_juce_plugin_defaults() for all plugin
# targets. The function iterates JUCE_FORMATS to apply flags to the actual
# linked MODULE targets (formats), not the STATIC
# shared-code target - STATIC library link options have no effect.
#
# =============================================================================

function(bws_apply_plugin_linker_flags target)
    if(NOT TARGET ${target})
        message(WARNING "bws_apply_plugin_linker_flags: target '${target}' does not exist")
        return()
    endif()

    # Iterate JUCE format targets (formats).
    # The main plugin target is STATIC - link options must go on the MODULE
    # format targets that produce the actual linked binaries.
    get_target_property(formats ${target} JUCE_FORMATS)
    if(NOT formats)
        return()
    endif()

    foreach(fmt IN LISTS formats)
        if(NOT TARGET ${target}_${fmt})
            continue()
        endif()

        # -----------------------------------------------------------------
        # -dead_strip (Apple linker, Release only)
        #
        # Removes unreferenced code and data from the final binary after LTO.
        # Complementary to juce::juce_recommended_lto_flags - LTO eliminates
        # dead code across TU boundaries; -dead_strip removes whatever
        # remains in the final linked binary.
        #
        # Uses if(CMAKE_BUILD_TYPE) guard instead of generator expression
        # to avoid CMake <3.24 bug with LINKER: prefix inside genex
        # (project minimum is cmake 3.22; all presets use single-config Ninja).
        #
        # Apple-only. Other platforms use their own linker conventions.
        #
        # Reference: Apple ld man page, -dead_strip option.
        # -----------------------------------------------------------------
        if(APPLE AND CMAKE_BUILD_TYPE STREQUAL "Release")
            target_link_options(${target}_${fmt} PRIVATE
                "LINKER:-dead_strip"
            )
        endif()

    endforeach()
endfunction()

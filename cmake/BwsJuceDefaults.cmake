# cmake/BwsJuceDefaults.cmake
# Standard JUCE compile definitions for all Bellweather Studios plugins.
# Disables unused JUCE features that increase binary size and attack surface.
#
# JUCE_WEB_BROWSER is conditionally enabled per-plugin via bws_add_web_ui().
# The generator expression reads BWS_HAS_WEB_UI, set by BwsWebUi.cmake.
# Generator expressions evaluate at generation time (after all CMakeLists.txt
# are processed), so call order between this function and bws_add_web_ui()
# does not matter.
#
function(bws_apply_juce_plugin_defaults target)
    if (NOT TARGET ${target})
        message(WARNING "bws_apply_juce_plugin_defaults: target ${target} does not exist")
        return()
    endif()
    target_compile_definitions(${target} PUBLIC
        # ── Format / protocol ──────────────────────────────────────────
        JUCE_VST3_CAN_REPLACE_VST2=0

        # ── Browser / WebKit ───────────────────────────────────────────
        $<IF:$<BOOL:$<TARGET_PROPERTY:${target},BWS_HAS_WEB_UI>>,JUCE_WEB_BROWSER=1,JUCE_WEB_BROWSER=0>

        # ── Network ────────────────────────────────────────────────────
        JUCE_USE_CURL=0

        # ── Audio codecs ───────────────────────────────────────────────
        # No production plugin decodes Ogg, FLAC, or MP3. WAV/AIFF remain
        # available for plugin paths that need basic file-format support.
        JUCE_USE_OGGVORBIS=0
        JUCE_USE_FLAC=0
        JUCE_USE_MP3AUDIOFORMAT=0
        JUCE_USE_WINDOWS_MEDIA_FORMAT=0
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target} PRIVATE -Wno-deprecated-declarations)
        get_target_property(formats ${target} JUCE_FORMATS)
        if(formats)
            foreach(fmt IN LISTS formats)
                if(TARGET ${target}_${fmt})
                    target_compile_options(${target}_${fmt} PRIVATE -Wno-deprecated-declarations)
                endif()
                string(TOLOWER "${fmt}" fmt_lower)
                if(TARGET ${target}_${fmt_lower}_helper)
                    target_compile_options(${target}_${fmt_lower}_helper PRIVATE -Wno-deprecated-declarations)
                endif()
            endforeach()
        endif()
    endif()

    # Apply release-build linker optimizations to all format targets
    bws_apply_plugin_linker_flags(${target})
endfunction()

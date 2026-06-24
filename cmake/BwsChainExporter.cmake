# BwsChainExporter.cmake
# ─────────────────────────────────────────────────────────────────────────
# bws_add_chain_exporter(<PluginTarget>)
#
# Adds a chain exporter executable for a plugin. The exporter reads
# kSignalChain / kThreadCrossings constexpr declarations and outputs
# a .chain.json file.
#
# Expects a source file at:
#   tools/exporters/<plugin_lower>_chain_exporter.cpp
#
# If the source doesn't exist, the call is silently skipped.
# ─────────────────────────────────────────────────────────────────────────

include(${CMAKE_SOURCE_DIR}/cmake/BwsVisibility.cmake)

function(bws_add_chain_exporter PLUGIN_TARGET)
    string(TOLOWER "${PLUGIN_TARGET}" PLUGIN_LOWER)
    set(EXPORTER_TARGET "${PLUGIN_LOWER}_chain_exporter")
    set(EXPORTER_SRC "${CMAKE_SOURCE_DIR}/tools/exporters/${PLUGIN_LOWER}_chain_exporter.cpp")
    set(OUTPUT_JSON "${CMAKE_BINARY_DIR}/chain/${PLUGIN_LOWER}.chain.json")

    if(NOT EXISTS "${EXPORTER_SRC}")
        message(STATUS "No chain exporter for ${PLUGIN_TARGET} - skipping")
        return()
    endif()

    # Ensure output directory exists
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/chain")

    add_executable(${EXPORTER_TARGET} ${EXPORTER_SRC})

    # Link against the plugin target and bw_dsp for the serializer.
    # Also link the plugin's own PRIVATE dependencies (bw_rt, bw_ui, etc.)
    # since the exporter #includes Processor.h which cascades through
    # those headers. PRIVATE link on the plugin means include dirs don't
    # propagate - so we re-link them here.
    get_target_property(_plugin_libs ${PLUGIN_TARGET} LINK_LIBRARIES)
    target_link_libraries(${EXPORTER_TARGET} PRIVATE
        ${PLUGIN_TARGET}
        bw_dsp
    )
    if(_plugin_libs)
        target_link_libraries(${EXPORTER_TARGET} PRIVATE ${_plugin_libs})
    endif()

    # Include paths: project root (for relative plugin includes) and
    # JuceLibraryCode generated headers.
    # Use the plugin target's BINARY_DIR to locate JuceLibraryCode correctly,
    # since plugins are nested under plugins/production/ in the build tree.
    get_target_property(_plugin_bindir ${PLUGIN_TARGET} BINARY_DIR)
    target_include_directories(${EXPORTER_TARGET} PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${_plugin_bindir}/${PLUGIN_TARGET}_artefacts/JuceLibraryCode
    )

    target_compile_definitions(${EXPORTER_TARGET} PRIVATE
        JUCE_TARGET_HAS_BINARY_DATA=0
    )

    # Suppress macro-redefined warnings from JUCE plugin metadata
    if(MSVC)
        target_compile_options(${EXPORTER_TARGET} PRIVATE /wd4005)
    else()
        target_compile_options(${EXPORTER_TARGET} PRIVATE -Wno-macro-redefined)
    endif()

    bws_apply_hidden_visibility(${EXPORTER_TARGET})
    set_property(TARGET ${EXPORTER_TARGET} PROPERTY CXX_STANDARD 20)
    set_property(TARGET ${EXPORTER_TARGET} PROPERTY CXX_EXTENSIONS OFF)

    # POST_BUILD: run the exporter and write JSON to the chain/ directory.
    # Two-step: generate, then validate non-empty.
    add_custom_command(
        TARGET ${EXPORTER_TARGET} POST_BUILD
        COMMAND $<TARGET_FILE:${EXPORTER_TARGET}> > "${OUTPUT_JSON}"
        COMMENT "Exporting chain descriptor for ${PLUGIN_TARGET}"
    )
endfunction()

# Convenience target: build all chain exporters
if(NOT TARGET all_chain_exporters)
    add_custom_target(all_chain_exporters)
endif()

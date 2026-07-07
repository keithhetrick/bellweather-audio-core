# Centralized plugin manifest reader.
# Loads cmake/registries/plugin_manifest.json and provides accessor functions.
# Replaces: plugin_identity.cmake, plugins_registry.cmake, bws_plugin_identity.cmake.

# Load the manifest once. Call from root CMakeLists.txt before any plugin subdirectories.
function(bws_load_plugin_manifest)
    set(_manifest_path "${CMAKE_SOURCE_DIR}/cmake/registries/plugin_manifest.json")
    if(NOT EXISTS "${_manifest_path}")
        message(FATAL_ERROR "Plugin manifest not found: ${_manifest_path}")
    endif()
    file(READ "${_manifest_path}" _json)
    set_property(GLOBAL PROPERTY BWS_PLUGIN_MANIFEST_JSON "${_json}")

    # Build the plugin name list (replaces plugins_registry.cmake)
    string(JSON _count LENGTH "${_json}" "plugins")
    if(_count LESS 1)
        message(FATAL_ERROR "Plugin manifest contains no plugins")
    endif()
    set(_names "")
    math(EXPR _last "${_count} - 1")
    foreach(_i RANGE 0 ${_last})
        string(JSON _id GET "${_json}" "plugins" ${_i} "id")
        list(APPEND _names "${_id}")
    endforeach()
    set_property(GLOBAL PROPERTY BWS_PLUGIN_NAMES "${_names}")

    # Read company name
    string(JSON _company GET "${_json}" "company" "name")
    set_property(GLOBAL PROPERTY BWS_COMPANY_NAME "${_company}")

    message(STATUS "BWS manifest: loaded ${_count} plugins from ${_manifest_path}")
endfunction()

# Internal: find the array index for a plugin ID. Returns -1 if not found.
function(_bws_manifest_find_index plugin_id out_var)
    get_property(_json GLOBAL PROPERTY BWS_PLUGIN_MANIFEST_JSON)
    if(NOT _json)
        message(FATAL_ERROR "Plugin manifest not loaded. Call bws_load_plugin_manifest() first.")
    endif()
    string(JSON _count LENGTH "${_json}" "plugins")
    math(EXPR _last "${_count} - 1")
    foreach(_i RANGE 0 ${_last})
        string(JSON _id GET "${_json}" "plugins" ${_i} "id")
        if(_id STREQUAL "${plugin_id}")
            set(${out_var} ${_i} PARENT_SCOPE)
            return()
        endif()
    endforeach()
    set(${out_var} -1 PARENT_SCOPE)
endfunction()

# Get a single string field for a plugin.
# Usage: bws_manifest_get("version" out_var)
function(bws_manifest_get plugin_id field out_var)
    _bws_manifest_find_index("${plugin_id}" _idx)
    if(_idx EQUAL -1)
        message(FATAL_ERROR "Plugin '${plugin_id}' not found in manifest")
    endif()
    get_property(_json GLOBAL PROPERTY BWS_PLUGIN_MANIFEST_JSON)
    string(JSON _val ERROR_VARIABLE _err GET "${_json}" "plugins" ${_idx} "${field}")
    if(_err)
        set(${out_var} "" PARENT_SCOPE)
    else()
        set(${out_var} "${_val}" PARENT_SCOPE)
    endif()
endfunction()

# Get an array field as a CMake semicolon-separated list.
# Usage: bws_manifest_get_list("formats" out_list)
function(bws_manifest_get_list plugin_id field out_list)
    _bws_manifest_find_index("${plugin_id}" _idx)
    if(_idx EQUAL -1)
        message(FATAL_ERROR "Plugin '${plugin_id}' not found in manifest")
    endif()
    get_property(_json GLOBAL PROPERTY BWS_PLUGIN_MANIFEST_JSON)
    string(JSON _len ERROR_VARIABLE _err LENGTH "${_json}" "plugins" ${_idx} "${field}")
    if(_err OR _len LESS 1)
        set(${out_list} "" PARENT_SCOPE)
        return()
    endif()
    set(_items "")
    math(EXPR _last_item "${_len} - 1")
    foreach(_j RANGE 0 ${_last_item})
        string(JSON _item GET "${_json}" "plugins" ${_idx} "${field}" ${_j})
        list(APPEND _items "${_item}")
    endforeach()
    set(${out_list} "${_items}" PARENT_SCOPE)
endfunction()

# Get a boolean field. Returns TRUE or FALSE.
function(_bws_manifest_get_bool plugin_id field out_var)
    _bws_manifest_find_index("${plugin_id}" _idx)
    if(_idx EQUAL -1)
        message(FATAL_ERROR "Plugin '${plugin_id}' not found in manifest")
    endif()
    get_property(_json GLOBAL PROPERTY BWS_PLUGIN_MANIFEST_JSON)
    string(JSON _val ERROR_VARIABLE _err GET "${_json}" "plugins" ${_idx} "${field}")
    if(_err)
        set(${out_var} FALSE PARENT_SCOPE)
        return()
    endif()

    # CMake JSON booleans may arrive as true/false or ON/OFF depending on version/context.
    if("${_val}" STREQUAL "true"
       OR "${_val}" STREQUAL "TRUE"
       OR "${_val}" STREQUAL "ON"
       OR "${_val}" STREQUAL "1")
        set(${out_var} TRUE PARENT_SCOPE)
    else()
        set(${out_var} FALSE PARENT_SCOPE)
    endif()
endfunction()

function(_bws_manifest_channel_layout_policy_to_int policy out_var)
    if("${policy}" STREQUAL "matched_mono_stereo")
        set(${out_var} 1 PARENT_SCOPE)
    elseif("${policy}" STREQUAL "matched_mono_stereo_sidechain")
        set(${out_var} 2 PARENT_SCOPE)
    elseif("${policy}" STREQUAL "mono_to_stereo")
        set(${out_var} 3 PARENT_SCOPE)
    else()
        set(${out_var} 0 PARENT_SCOPE)
    endif()
endfunction()

# Populate all BWS_* variables for a plugin. Call before juce_add_plugin().
# Sets in PARENT_SCOPE: BWS_VERSION, BWS_PRODUCT_NAME, BWS_BUNDLE_ID,
# BWS_MANUFACTURER_CODE, BWS_PLUGIN_CODE, BWS_FORMATS, BWS_COMPANY_NAME,
# BWS_IS_SYNTH, BWS_NEEDS_MIDI_INPUT, BWS_NEEDS_MIDI_OUTPUT, BWS_IS_MIDI_EFFECT,
# BWS_EDITOR_WANTS_KEYBOARD_FOCUS, BWS_VST3_CATEGORIES,
# BWS_CHANNEL_LAYOUT_POLICY
function(bws_manifest_populate plugin_id)
    bws_manifest_get("${plugin_id}" "version" _version)
    bws_manifest_get("${plugin_id}" "displayName" _display)
    bws_manifest_get("${plugin_id}" "bundleId" _bundle)
    bws_manifest_get("${plugin_id}" "manufacturerCode" _manu)
    bws_manifest_get("${plugin_id}" "pluginCode" _code)
    bws_manifest_get_list("${plugin_id}" "formats" _formats)

    get_property(_company GLOBAL PROPERTY BWS_COMPANY_NAME)

    _bws_manifest_get_bool("${plugin_id}" "isSynth" _is_synth)
    _bws_manifest_get_bool("${plugin_id}" "needsMidiInput" _midi_in)
    _bws_manifest_get_bool("${plugin_id}" "needsMidiOutput" _midi_out)
    _bws_manifest_get_bool("${plugin_id}" "isMidiEffect" _midi_fx)
    _bws_manifest_get_bool("${plugin_id}" "editorWantsKeyboardFocus" _kbd_focus)

    bws_manifest_get_list("${plugin_id}" "vst3Categories" _vst3_cats)
    bws_manifest_get("${plugin_id}" "auMainType" _au_main_type)
    bws_manifest_get("${plugin_id}" "channelLayoutPolicy" _channel_layout_policy)
    _bws_manifest_channel_layout_policy_to_int("${_channel_layout_policy}" _channel_layout_policy_int)

    set(BWS_VERSION "${_version}" PARENT_SCOPE)
    set(BWS_PRODUCT_NAME "${_display}" PARENT_SCOPE)
    set(BWS_BUNDLE_ID "${_bundle}" PARENT_SCOPE)
    set(BWS_MANUFACTURER_CODE "${_manu}" PARENT_SCOPE)
    set(BWS_PLUGIN_CODE "${_code}" PARENT_SCOPE)
    set(BWS_FORMATS "${_formats}" PARENT_SCOPE)
    set(BWS_COMPANY_NAME "${_company}" PARENT_SCOPE)
    set(BWS_IS_SYNTH "${_is_synth}" PARENT_SCOPE)
    set(BWS_NEEDS_MIDI_INPUT "${_midi_in}" PARENT_SCOPE)
    set(BWS_NEEDS_MIDI_OUTPUT "${_midi_out}" PARENT_SCOPE)
    set(BWS_IS_MIDI_EFFECT "${_midi_fx}" PARENT_SCOPE)
    set(BWS_EDITOR_WANTS_KEYBOARD_FOCUS "${_kbd_focus}" PARENT_SCOPE)
    set(BWS_VST3_CATEGORIES "${_vst3_cats}" PARENT_SCOPE)
    set(BWS_AU_MAIN_TYPE "${_au_main_type}" PARENT_SCOPE)
    set(BWS_CHANNEL_LAYOUT_POLICY "${_channel_layout_policy_int}" PARENT_SCOPE)
endfunction()

# =============================================================================
#  Shared JUCE module aggregator (consolidation target)
#
# INTERFACE library that bundles the union of JUCE modules production plugins
# need. Plugins link `bws::juce_shared` instead of listing juce::* modules
# individually. One edit point for the module list.
#
# CAVEAT: This does NOT reduce compile time. JUCE modules are themselves
# INTERFACE libraries (per juce_add_module design); their sources still
# compile into every consumer's object directory. Real compile-dedup is
#  (OBJECT/STATIC aggregator owning JUCE sources directly).
# =============================================================================
if(NOT TARGET bws_juce_shared)
    add_library(bws_juce_shared INTERFACE)
    add_library(bws::juce_shared ALIAS bws_juce_shared)

    target_link_libraries(bws_juce_shared
        INTERFACE
            juce::juce_audio_basics
            juce::juce_audio_processors
            juce::juce_audio_utils
            juce::juce_audio_formats
            juce::juce_core
            juce::juce_cryptography
            juce::juce_data_structures
            juce::juce_dsp
            juce::juce_events
            juce::juce_graphics
            juce::juce_gui_basics
            juce::juce_gui_extra
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags
    )
endif()

# One-line plugin setup: populate manifest, project(), juce_add_plugin(),
# juce_generate_juce_header(), and unity-build disable - all in one call.
# Usage: bws_add_juce_plugin(BwsBarometer)
# After calling this, only target_sources/link_libraries/compile_definitions remain.
macro(bws_add_juce_plugin plugin_id)
    bws_manifest_populate(${plugin_id})
    project(${plugin_id} VERSION ${BWS_VERSION})

    # Build juce_add_plugin arguments
    set(_bws_jap_args
        VERSION ${PROJECT_VERSION}
        COMPANY_NAME "${BWS_COMPANY_NAME}"
        PLUGIN_MANUFACTURER_CODE ${BWS_MANUFACTURER_CODE}
        PLUGIN_CODE ${BWS_PLUGIN_CODE}
        FORMATS ${BWS_FORMATS}
        PRODUCT_NAME "${BWS_PRODUCT_NAME}"
        BUNDLE_ID "${BWS_BUNDLE_ID}"
        IS_SYNTH ${BWS_IS_SYNTH}
        NEEDS_MIDI_INPUT ${BWS_NEEDS_MIDI_INPUT}
        NEEDS_MIDI_OUTPUT ${BWS_NEEDS_MIDI_OUTPUT}
        IS_MIDI_EFFECT ${BWS_IS_MIDI_EFFECT}
        EDITOR_WANTS_KEYBOARD_FOCUS ${BWS_EDITOR_WANTS_KEYBOARD_FOCUS}
        COPY_PLUGIN_AFTER_BUILD FALSE
    )

    if(BWS_VST3_CATEGORIES)
        list(APPEND _bws_jap_args VST3_CATEGORIES ${BWS_VST3_CATEGORIES})
    endif()
    if(BWS_AU_MAIN_TYPE)
        list(APPEND _bws_jap_args AU_MAIN_TYPE "${BWS_AU_MAIN_TYPE}")
    endif()

    juce_add_plugin(${plugin_id} ${_bws_jap_args})
    juce_generate_juce_header(${plugin_id})

    target_compile_definitions(${plugin_id}
        PUBLIC
            BWS_PLUGIN_CHANNEL_LAYOUT_POLICY=${BWS_CHANNEL_LAYOUT_POLICY}
    )

    # Disable unity build for JUCE compatibility + post-build completion banner
    set_target_properties(${plugin_id} PROPERTIES UNITY_BUILD OFF)
    foreach(_bws_fmt ${BWS_FORMATS})
        if(TARGET ${plugin_id}_${_bws_fmt})
            set_target_properties(${plugin_id}_${_bws_fmt} PROPERTIES UNITY_BUILD OFF)
            add_custom_command(TARGET ${plugin_id}_${_bws_fmt} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E echo ""
                COMMAND ${CMAKE_COMMAND} -E echo "  =========================================="
                COMMAND ${CMAKE_COMMAND} -E echo "  BUILD COMPLETE: ${BWS_PRODUCT_NAME} ${_bws_fmt} v${BWS_VERSION}"
                COMMAND ${CMAKE_COMMAND} -E echo "  =========================================="
                COMMAND ${CMAKE_COMMAND} -E echo ""
            )
        endif()
    endforeach()
endmacro()

# Validate manifest at configure time (replaces bws_validate_plugin_identities).
function(bws_validate_manifest)
    get_property(_json GLOBAL PROPERTY BWS_PLUGIN_MANIFEST_JSON)
    if(NOT _json)
        message(FATAL_ERROR "Plugin manifest not loaded.")
    endif()
    string(JSON _count LENGTH "${_json}" "plugins")
    math(EXPR _last "${_count} - 1")

    # Collect all codes for uniqueness checks
    set(_seen_codes "")
    set(_seen_bundles "")
    foreach(_i RANGE 0 ${_last})
        string(JSON _id GET "${_json}" "plugins" ${_i} "id")
        string(JSON _manu GET "${_json}" "plugins" ${_i} "manufacturerCode")
        string(JSON _code GET "${_json}" "plugins" ${_i} "pluginCode")
        string(JSON _bundle GET "${_json}" "plugins" ${_i} "bundleId")

        # 4-char manufacturer code
        string(LENGTH "${_manu}" _manu_len)
        if(NOT _manu_len EQUAL 4)
            message(FATAL_ERROR "Manifest: '${_id}' manufacturerCode '${_manu}' must be exactly 4 characters.")
        endif()

        # 4-char plugin code
        string(LENGTH "${_code}" _code_len)
        if(NOT _code_len EQUAL 4)
            message(FATAL_ERROR "Manifest: '${_id}' pluginCode '${_code}' must be exactly 4 characters.")
        endif()

        # Reverse-DNS bundle ID
        if(NOT _bundle MATCHES "\\.")
            message(WARNING "Manifest: '${_id}' bundleId '${_bundle}' does not look like reverse-DNS.")
        endif()

        # Plugin code uniqueness
        if("${_code}" IN_LIST _seen_codes)
            message(FATAL_ERROR "Manifest: duplicate pluginCode '${_code}' for plugin '${_id}'.")
        endif()
        list(APPEND _seen_codes "${_code}")

        # Bundle ID uniqueness
        if("${_bundle}" IN_LIST _seen_bundles)
            message(FATAL_ERROR "Manifest: duplicate bundleId '${_bundle}' for plugin '${_id}'.")
        endif()
        list(APPEND _seen_bundles "${_bundle}")
    endforeach()

    message(STATUS "BWS manifest: validated ${_count} plugins (all codes unique)")
endfunction()

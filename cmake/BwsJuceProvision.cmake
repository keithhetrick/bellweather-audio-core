#[[
 JUCE provisioning helper (root-level)
 Priority order (default is AUTO_LOCAL_FIRST → prefer explicit/local path, otherwise FetchContent):
 1) Explicit cache variables (BWS_JUCE_PROVIDER/BWS_JUCE_PATH)
 2) Environment variables (BWS_JUCE_PATH, BWS_JUCE_LOCAL_PATH, or JUCE_PATH)
 3) Vendored JUCE directory at external/JUCE (SUBMODULE)
 4) FetchContent (pinned, when enabled)
]]

# Allow callers that already provisioned JUCE (e.g. parent project) to skip.
if (TARGET juce::juce_core)
    return()
endif()

# Pinned versions (single source of truth)
include(BwsVersions)

set(_env_juce_path "$ENV{BWS_JUCE_PATH}")
if (_env_juce_path STREQUAL "" AND NOT "$ENV{BWS_JUCE_LOCAL_PATH}" STREQUAL "")
    set(_env_juce_path "$ENV{BWS_JUCE_LOCAL_PATH}")
endif()
if (_env_juce_path STREQUAL "" AND NOT "$ENV{JUCE_PATH}" STREQUAL "")
    set(_env_juce_path "$ENV{JUCE_PATH}")
endif()

option(BWS_JUCE_FETCH "Allow pinned JUCE download via FetchContent when no local path is provided" ON)

if (NOT DEFINED BWS_JUCE_PROVIDER OR BWS_JUCE_PROVIDER STREQUAL "")
    if (BWS_JUCE_FETCH)
        set(BWS_JUCE_PROVIDER "AUTO_LOCAL_FIRST")
    else()
        set(BWS_JUCE_PROVIDER "PATH")
    endif()
endif()
set(BWS_JUCE_PROVIDER "${BWS_JUCE_PROVIDER}" CACHE STRING "JUCE provider: LOCAL, FETCHCONTENT, PATH, SUBMODULE, AUTO_LOCAL_FIRST (default AUTO_LOCAL_FIRST when BWS_JUCE_FETCH=ON)" FORCE)
set_property(CACHE BWS_JUCE_PROVIDER PROPERTY STRINGS LOCAL FETCHCONTENT PATH SUBMODULE AUTO_LOCAL_FIRST)

set(_provider_raw "${BWS_JUCE_PROVIDER}")
string(TOUPPER "${_provider_raw}" _provider_normalized)
if (_provider_normalized STREQUAL "LOCAL")
    set(_provider_normalized "PATH")
endif()

set(BWS_JUCE_PATH "${BWS_JUCE_PATH}" CACHE PATH "Path to JUCE when BWS_JUCE_PROVIDER=PATH")
if (BWS_JUCE_PATH STREQUAL "" AND NOT _env_juce_path STREQUAL "")
    set(BWS_JUCE_PATH "${_env_juce_path}" CACHE PATH "Path to JUCE when BWS_JUCE_PROVIDER=PATH" FORCE)
endif()

# Auto-discovery for PATH provider when no explicit path is set.
function(_bws_try_set_auto_juce_path)
    if (NOT BWS_JUCE_PATH STREQUAL "")
        return()
    endif()

    file(GLOB _build_dep_candidates LIST_DIRECTORIES TRUE "${CMAKE_SOURCE_DIR}/build/*/_deps/juce-src")
    file(GLOB _build_checkout_candidates LIST_DIRECTORIES TRUE "${CMAKE_SOURCE_DIR}/build/*/JUCE")
    set(_candidates
        "${CMAKE_SOURCE_DIR}/build/dev/_deps/juce-src"
        "${CMAKE_SOURCE_DIR}/build/debug/_deps/juce-src"
        "${CMAKE_SOURCE_DIR}/deps/JUCE"
        "${CMAKE_SOURCE_DIR}/third_party/JUCE"
        "$ENV{HOME}/JUCE"
    )
    list(APPEND _candidates ${_build_dep_candidates} ${_build_checkout_candidates})
    if (DEFINED ENV{BWS_JUCE_LOCAL_PATH} AND NOT "$ENV{BWS_JUCE_LOCAL_PATH}" STREQUAL "")
        list(INSERT _candidates 0 "$ENV{BWS_JUCE_LOCAL_PATH}")
    endif()
    if (DEFINED ENV{BWS_JUCE_PATH} AND NOT "$ENV{BWS_JUCE_PATH}" STREQUAL "")
        list(INSERT _candidates 0 "$ENV{BWS_JUCE_PATH}")
    endif()
    if (DEFINED ENV{JUCE_PATH} AND NOT "$ENV{JUCE_PATH}" STREQUAL "")
        list(INSERT _candidates 1 "$ENV{JUCE_PATH}")
    endif()

    set(_found "")
    foreach(_cand IN LISTS _candidates)
        if (NOT EXISTS "${_cand}")
            continue()
        endif()
        if (EXISTS "${_cand}/modules" AND EXISTS "${_cand}/extras/Build/CMake/JUCEUtils.cmake")
            set(_found "${_cand}")
            break()
        endif()
    endforeach()

    if (NOT _found STREQUAL "")
        set(BWS_JUCE_PATH "${_found}" CACHE PATH "Auto-detected JUCE path" FORCE)
        message(STATUS "BWS JUCE: auto-detected path: ${BWS_JUCE_PATH}")
    endif()
endfunction()

if (NOT DEFINED BWS_JUCE_FETCH_TAG OR BWS_JUCE_FETCH_TAG STREQUAL "")
    set(BWS_JUCE_FETCH_TAG "${BWS_JUCE_PINNED_SHA}")
endif()
set(BWS_JUCE_FETCH_TAG "${BWS_JUCE_FETCH_TAG}" CACHE STRING "JUCE git tag/commit to fetch when using FetchContent")
set(BWS_JUCE_GIT_TAG "${BWS_JUCE_FETCH_TAG}" CACHE STRING "JUCE git tag/commit to fetch when using FetchContent")
set(BWS_JUCE_GIT_REPOSITORY "https://github.com/juce-framework/JUCE.git" CACHE STRING "JUCE git repository URL")
set(BWS_JUCE_MIN_VERSION "8.0.0" CACHE STRING "Minimum JUCE version required by Bellweather")
set(BWS_JUCE_TESTED_VERSIONS "8.0.12" CACHE STRING "JUCE versions validated by Bellweather (semicolon-separated)")
set(_bws_fetch_hint "Set -DBWS_JUCE_PATH=/abs/path/to/JUCE (or env BWS_JUCE_PATH/BWS_JUCE_LOCAL_PATH/JUCE_PATH). Leave BWS_JUCE_FETCH=ON (default) without a path to auto-fetch JUCE ${BWS_JUCE_PINNED_TAG} at ${BWS_JUCE_GIT_TAG}.")

function(_bws_check_juce_version juce_root)
    get_property(_bws_version_checked GLOBAL PROPERTY BWS_JUCE_VERSION_CHECKED)
    if (_bws_version_checked)
        return()
    endif()

    set(_candidate_headers
        "${juce_root}/modules/juce_core/system/juce_StandardHeader.h"
        "${juce_root}/modules/juce_core/system/juce_TargetPlatform.h"
        "${juce_root}/modules/juce_core/juce_core.h"
    )
    set(_juce_major "")
    set(_juce_minor "")
    set(_juce_patch "")
    set(_parsed FALSE)
    set(_tried_headers "")

    foreach(_juce_header IN LISTS _candidate_headers)
        list(APPEND _tried_headers "${_juce_header}")
        if (NOT EXISTS "${_juce_header}")
            continue()
        endif()

        file(READ "${_juce_header}" _juce_header_contents)
        string(REGEX MATCH "#define[ \t]+JUCE_MAJOR_VERSION[ \t]+([0-9]+)" _juce_major_line "${_juce_header_contents}")
        if (NOT _juce_major_line)
            continue()
        endif()
        set(_juce_major "${CMAKE_MATCH_1}")

        string(REGEX MATCH "#define[ \t]+JUCE_MINOR_VERSION[ \t]+([0-9]+)" _juce_minor_line "${_juce_header_contents}")
        if (_juce_minor_line)
            set(_juce_minor "${CMAKE_MATCH_1}")
        else()
            set(_juce_minor "0")
        endif()

        string(REGEX MATCH "#define[ \t]+JUCE_BUILDNUMBER[ \t]+([0-9]+)" _juce_patch_line "${_juce_header_contents}")
        if (_juce_patch_line)
            set(_juce_patch "${CMAKE_MATCH_1}")
        else()
            string(REGEX MATCH "#define[ \t]+JUCE_BUILD_NUMBER[ \t]+([0-9]+)" _juce_patch_line_alt "${_juce_header_contents}")
            if (_juce_patch_line_alt)
                set(_juce_patch "${CMAKE_MATCH_1}")
            else()
                set(_juce_patch "0")
            endif()
        endif()

        set(_parsed TRUE)
        break()
    endforeach()

    if (NOT _parsed)
        list(JOIN _tried_headers ", " _tried_join)
        message(WARNING "JUCE version check skipped; could not parse JUCE_MAJOR_VERSION from: ${_tried_join}")
        return()
    endif()

    set(_juce_version "${_juce_major}.${_juce_minor}.${_juce_patch}")
    set(BWS_JUCE_VERSION_DETECTED "${_juce_version}" CACHE STRING "Detected JUCE version" FORCE)

    if (_juce_version VERSION_LESS BWS_JUCE_MIN_VERSION)
        message(FATAL_ERROR "JUCE ${_juce_version} is below required minimum ${BWS_JUCE_MIN_VERSION}. Set -DBWS_JUCE_PROVIDER=FETCHCONTENT -DBWS_JUCE_FETCH_TAG=${BWS_JUCE_FETCH_TAG} or BWS_JUCE_PATH to a newer JUCE checkout.")
    endif()

    set(_tested_suffix "")
    set(_bws_tested_list "${BWS_JUCE_TESTED_VERSIONS}")
    if (NOT _bws_tested_list STREQUAL "")
        string(REPLACE ";" "|" _bws_tested_regex "${_bws_tested_list}")
        if (NOT _juce_version MATCHES "(${_bws_tested_regex})")
            message(WARNING "JUCE version ${_juce_version} differs from tested set {${_bws_tested_list}}. For the tested version, configure with -DBWS_JUCE_PROVIDER=FETCHCONTENT -DBWS_JUCE_FETCH_TAG=${BWS_JUCE_FETCH_TAG} or point BWS_JUCE_PATH to that tag.")
        else()
            set(_tested_suffix " (tested set {${_bws_tested_list}})")
        endif()
    endif()

    message(STATUS "JUCE version ${_juce_version} meets minimum ${BWS_JUCE_MIN_VERSION}${_tested_suffix}")

    set_property(GLOBAL PROPERTY BWS_JUCE_VERSION_CHECKED TRUE)
endfunction()

set(_project_root "${PROJECT_SOURCE_DIR}")
if (NOT DEFINED PROJECT_SOURCE_DIR)
    set(_project_root "${CMAKE_SOURCE_DIR}")
endif()

set(_vendored_dir "${_project_root}/external/JUCE")
set(_vendored_is_checkout FALSE)
set(_pathfile_present FALSE)
if (EXISTS "${_vendored_dir}" AND NOT IS_DIRECTORY "${_vendored_dir}")
    set(_pathfile_present TRUE)
endif()
if (IS_DIRECTORY "${_vendored_dir}" AND EXISTS "${_vendored_dir}/modules/CMakeLists.txt")
    set(_vendored_is_checkout TRUE)
endif()
set(BWS_JUCE_PATHFILE_PRESENT ${_pathfile_present} CACHE BOOL "True when external/JUCE exists as a non-directory path file" FORCE)

set(_effective_provider "${_provider_normalized}")
set(_effective_path "${BWS_JUCE_PATH}")

if (BWS_JUCE_PATHFILE_PRESENT)
    message(FATAL_ERROR
        "external/JUCE is a pathfile/symlink, not a JUCE checkout. "
        "Remove the pathfile and either vendor JUCE as external/JUCE or set "
        "BWS_JUCE_PROVIDER=PATH and BWS_JUCE_PATH=/abs/path/to/JUCE.")
endif()

# Auto-resolve provider when left at the default or AUTO_LOCAL_FIRST.
# IMPORTANT: AUTO_LOCAL_FIRST must try local auto-detection BEFORE falling back to FetchContent.
if (_effective_provider STREQUAL "AUTO_LOCAL_FIRST")
    if (NOT _effective_path STREQUAL "")
        set(_effective_provider "PATH") # explicit cache path wins
    elseif (NOT _env_juce_path STREQUAL "")
        set(_effective_provider "PATH") # environment path wins
        set(_effective_path "${_env_juce_path}")
    elseif (_vendored_is_checkout)
        set(_effective_provider "SUBMODULE")
    else()
        # For AUTO_LOCAL_FIRST (and also PATH-ish configs), attempt auto-detection candidates (e.g. $HOME/JUCE)
        _bws_try_set_auto_juce_path()
        if (NOT "${BWS_JUCE_PATH}" STREQUAL "")
            set(_effective_provider "PATH")
            set(_effective_path "${BWS_JUCE_PATH}")
        elseif (_effective_provider STREQUAL "AUTO_LOCAL_FIRST")
            # Only fall back to FetchContent if explicitly allowed.
            if (BWS_JUCE_FETCH)
                set(_effective_provider "FETCHCONTENT")
            else()
                message(FATAL_ERROR
                    "BWS_JUCE_FETCH=OFF and no local JUCE checkout was found. "
                    "Provide JUCE via external/JUCE (SUBMODULE) or set -DBWS_JUCE_PATH=/abs/path/to/JUCE "
                    "(or env BWS_JUCE_PATH/BWS_JUCE_LOCAL_PATH/JUCE_PATH).")
            endif()
        endif()
    endif()
endif()

if (_effective_provider STREQUAL "FETCHCONTENT" AND NOT _vendored_is_checkout AND EXISTS "${_vendored_dir}")
    message(STATUS "external/JUCE exists but is not a JUCE checkout (missing modules/CMakeLists.txt); using FetchContent instead.")
endif()

if (_effective_provider STREQUAL "PATH")
    if (_effective_path STREQUAL "")
        if (DEFINED ENV{BWS_JUCE_PATH})
            set(_effective_path "$ENV{BWS_JUCE_PATH}")
        elseif(DEFINED ENV{BWS_JUCE_LOCAL_PATH})
            set(_effective_path "$ENV{BWS_JUCE_LOCAL_PATH}")
        elseif(DEFINED ENV{JUCE_PATH})
            set(_effective_path "$ENV{JUCE_PATH}")
        endif()
    endif()

    if (_effective_path STREQUAL "")
        _bws_try_set_auto_juce_path()
        set(_effective_path "${BWS_JUCE_PATH}")
    endif()

    if (_effective_path STREQUAL "")
        set(_search_candidates
            "${CMAKE_SOURCE_DIR}/build/dev/_deps/juce-src"
            "${CMAKE_SOURCE_DIR}/build/debug/_deps/juce-src"
            "${CMAKE_SOURCE_DIR}/deps/JUCE"
            "${CMAKE_SOURCE_DIR}/third_party/JUCE"
            "$ENV{HOME}/JUCE"
        )
        file(GLOB _search_build_dep_candidates LIST_DIRECTORIES TRUE "${CMAKE_SOURCE_DIR}/build/*/_deps/juce-src")
        file(GLOB _search_build_checkout_candidates LIST_DIRECTORIES TRUE "${CMAKE_SOURCE_DIR}/build/*/JUCE")
        list(APPEND _search_candidates ${_search_build_dep_candidates} ${_search_build_checkout_candidates})
        if (DEFINED ENV{BWS_JUCE_LOCAL_PATH} AND NOT "$ENV{BWS_JUCE_LOCAL_PATH}" STREQUAL "")
            list(INSERT _search_candidates 0 "$ENV{BWS_JUCE_LOCAL_PATH}")
        endif()
        if (DEFINED ENV{BWS_JUCE_PATH} AND NOT "$ENV{BWS_JUCE_PATH}" STREQUAL "")
            list(INSERT _search_candidates 0 "$ENV{BWS_JUCE_PATH}")
        endif()
        if (DEFINED ENV{JUCE_PATH} AND NOT "$ENV{JUCE_PATH}" STREQUAL "")
            list(INSERT _search_candidates 1 "$ENV{JUCE_PATH}")
        endif()
        list(REMOVE_DUPLICATES _search_candidates)
        list(JOIN _search_candidates ", " _search_list)
        message(FATAL_ERROR "BWS_JUCE_PROVIDER=PATH but no JUCE path found. Searched: ${_search_list}. ${_bws_fetch_hint}")
    endif()

    get_filename_component(_resolved_juce "${_effective_path}" ABSOLUTE)
    if (NOT EXISTS "${_resolved_juce}/modules/CMakeLists.txt")
        message(FATAL_ERROR "JUCE path '${_resolved_juce}' is missing modules/CMakeLists.txt; set BWS_JUCE_PATH (or BWS_JUCE_LOCAL_PATH/JUCE_PATH) to a JUCE checkout or use the pinned FetchContent path.")
    endif()

_bws_check_juce_version("${_resolved_juce}")
    message(STATUS "Using JUCE from explicit path: ${_resolved_juce}")
    add_subdirectory("${_resolved_juce}" JUCE)
    set(JUCE_SOURCE_DIR "${_resolved_juce}" CACHE PATH "Resolved JUCE source directory" FORCE)
elseif (_effective_provider STREQUAL "SUBMODULE")
    get_filename_component(_resolved_juce "${_vendored_dir}" ABSOLUTE)
    if (NOT IS_DIRECTORY "${_resolved_juce}")
        message(FATAL_ERROR "BWS_JUCE_PROVIDER=SUBMODULE but ${_resolved_juce} is not a directory.")
    endif()
    if (NOT EXISTS "${_resolved_juce}/modules/CMakeLists.txt")
        message(FATAL_ERROR "BWS_JUCE_PROVIDER=SUBMODULE but ${_resolved_juce} does not look like a JUCE checkout (missing modules/CMakeLists.txt).")
    endif()

    _bws_check_juce_version("${_resolved_juce}")
    message(STATUS "Using JUCE from vendored directory: ${_resolved_juce}")
    add_subdirectory("${_resolved_juce}" JUCE)
    set(JUCE_SOURCE_DIR "${_resolved_juce}" CACHE PATH "Resolved JUCE source directory" FORCE)
elseif (_effective_provider STREQUAL "FETCHCONTENT")
    if (NOT BWS_JUCE_FETCH)
        message(FATAL_ERROR "BWS_JUCE_FETCH=OFF but FetchContent was requested. ${_bws_fetch_hint}")
    endif()
    include(FetchContent)
    message(STATUS "Fetching JUCE ${BWS_JUCE_PINNED_TAG} (${BWS_JUCE_GIT_TAG}) from ${BWS_JUCE_GIT_REPOSITORY}")
    message(STATUS "Offline? Set BWS_JUCE_PROVIDER=PATH and BWS_JUCE_PATH=/path/to/JUCE to use a local checkout.")

    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY ${BWS_JUCE_GIT_REPOSITORY}
        GIT_TAG ${BWS_JUCE_GIT_TAG}
        SOURCE_DIR "${CMAKE_BINARY_DIR}/_deps/juce-src"
        BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/juce-build"
        FIND_PACKAGE_ARGS NAMES JUCE
    )

    set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
    FetchContent_MakeAvailable(JUCE)
    if (DEFINED juce_SOURCE_DIR)
        set(JUCE_SOURCE_DIR "${juce_SOURCE_DIR}" CACHE PATH "Resolved JUCE source directory" FORCE)
    endif()
    if (DEFINED JUCE_SOURCE_DIR)
        _bws_check_juce_version("${JUCE_SOURCE_DIR}")
    endif()
else()
    message(FATAL_ERROR "Unknown BWS_JUCE_PROVIDER '${_effective_provider}'. Expected one of FETCHCONTENT, PATH, SUBMODULE.")
endif()

set(BWS_JUCE_PROVIDER_EFFECTIVE "${_effective_provider}" CACHE STRING "Resolved JUCE provider after auto-detection" FORCE)
set(BWS_JUCE_PATH_EFFECTIVE "${JUCE_SOURCE_DIR}" CACHE PATH "Resolved JUCE source directory after provisioning" FORCE)
if (_effective_provider STREQUAL "FETCHCONTENT")
    message(STATUS "BWS JUCE: provider=${_effective_provider} path=n/a tag=${BWS_JUCE_GIT_TAG}")
else()
    message(STATUS "BWS JUCE: provider=${_effective_provider} path=${JUCE_SOURCE_DIR} tag=${BWS_JUCE_GIT_TAG}")
endif()

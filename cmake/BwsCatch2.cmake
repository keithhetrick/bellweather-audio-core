# cmake/BwsCatch2.cmake
# Canonical Catch2 v3 find-or-fetch. Include once per test target.
option(BWS_FETCH_TEST_DEPS "Fetch test dependencies (Catch2) when not found" ON)
set(BWS_CATCH2_PINNED_TAG "v3.7.0")
set(BWS_CATCH2_PINNED_SHA "31588bb4f56b638dd5afc28d3ebff9b9dcefb88d")

function(bws_fetch_catch2)
    set(_bws_catch2_source_dir "")
    if(Catch2_SOURCE_DIR)
        set(_bws_catch2_source_dir "${Catch2_SOURCE_DIR}")
    elseif(catch2_SOURCE_DIR)
        set(_bws_catch2_source_dir "${catch2_SOURCE_DIR}")
    endif()

    if(TARGET Catch2::Catch2WithMain)
        if(_bws_catch2_source_dir)
            list(APPEND CMAKE_MODULE_PATH ${_bws_catch2_source_dir}/extras)
            set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" PARENT_SCOPE)
        endif()
        if(_bws_catch2_source_dir AND EXISTS "${_bws_catch2_source_dir}/extras/Catch.cmake")
            include("${_bws_catch2_source_dir}/extras/Catch.cmake")
        else()
            include(Catch)
        endif()
        return()
    endif()

    find_package(Catch2 3 CONFIG QUIET)
    if (NOT Catch2_FOUND)
        if (NOT BWS_FETCH_TEST_DEPS)
            message(FATAL_ERROR
                "Catch2 (v3) not found. Install it, or configure with -DBWS_FETCH_TEST_DEPS=ON to fetch it.")
        endif()
        include(FetchContent)
        message(STATUS "Catch2 not found, fetching ${BWS_CATCH2_PINNED_TAG} (${BWS_CATCH2_PINNED_SHA})...")
        FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG        ${BWS_CATCH2_PINNED_SHA}
        )
        FetchContent_MakeAvailable(Catch2)
        if(Catch2_SOURCE_DIR)
            set(_bws_catch2_source_dir "${Catch2_SOURCE_DIR}")
        elseif(catch2_SOURCE_DIR)
            set(_bws_catch2_source_dir "${catch2_SOURCE_DIR}")
        endif()
    endif()
    # Ensure Catch module is on the path regardless of how Catch2 was found.
    # Catch2_SOURCE_DIR is cached globally by FetchContent.
    if(_bws_catch2_source_dir)
        list(APPEND CMAKE_MODULE_PATH ${_bws_catch2_source_dir}/extras)
        set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" PARENT_SCOPE)
        include("${_bws_catch2_source_dir}/extras/Catch.cmake")
    else()
        include(Catch)
    endif()
endfunction()

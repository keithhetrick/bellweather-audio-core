# cmake/BwsWarnings.cmake
# Standard compiler warning flags for compiled modules. BWS_WARNINGS_AS_ERRORS
# (default OFF) promotes them to errors; opt in per build/lane, not globally.
option(BWS_WARNINGS_AS_ERRORS "Treat compiler warnings as errors on instrumented targets" OFF)

# Catch2's INTERNAL_CATCH_UNIQUE_NAME expands to __COUNTER__, which recent Clang
# flags as a C2y extension under -Wpedantic. Probe once with -Werror (so the probe
# fails on compilers that do not know the flag) and suppress only that one
# diagnostic where supported, keeping -Wpedantic -Werror on for everything else.
include(CheckCXXCompilerFlag)
if(NOT MSVC)
    set(_bws_saved_required_flags "${CMAKE_REQUIRED_FLAGS}")
    set(CMAKE_REQUIRED_FLAGS "-Werror")
    check_cxx_compiler_flag("-Wc2y-extensions" BWS_HAS_WC2Y_EXTENSIONS)
    set(CMAKE_REQUIRED_FLAGS "${_bws_saved_required_flags}")
endif()

function(bws_add_warnings target)
    if (NOT TARGET ${target})
        message(WARNING "bws_add_warnings: target ${target} does not exist")
        return()
    endif()
    # Optional STRICT: add implicit-conversion diagnostics (NASA Power-of-Ten
    # Rule 10). Opt in per target for low-math surfaces (parsers, contracts);
    # not applied blanket because DSP/FFT math is legitimately conversion-heavy.
    set(_bws_strict OFF)
    if("STRICT" IN_LIST ARGN)
        set(_bws_strict ON)
    endif()
    if(MSVC)
        # /Zc:__cplusplus: Make MSVC report the real __cplusplus value.
        # Without this, MSVC reports 199711L regardless of standard,
        # breaking BwCompilerFeatures.h gates and its static_assert.
        target_compile_options(${target} PRIVATE /W4 /Zc:__cplusplus)
        if(_bws_strict)
            target_compile_options(${target} PRIVATE /w14244 /w14245 /w14267)
        endif()
        if(BWS_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow)
        if(BWS_HAS_WC2Y_EXTENSIONS)
            target_compile_options(${target} PRIVATE -Wno-c2y-extensions)
        endif()
        if(_bws_strict)
            target_compile_options(${target} PRIVATE -Wconversion)
        endif()
        if(BWS_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

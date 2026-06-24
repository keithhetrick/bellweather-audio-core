# cmake/BwsWarnings.cmake
# Standard compiler warning flags for compiled modules. BWS_WARNINGS_AS_ERRORS
# (default OFF) promotes them to errors; opt in per build/lane, not globally.
option(BWS_WARNINGS_AS_ERRORS "Treat compiler warnings as errors on instrumented targets" OFF)

function(bws_add_warnings target)
    if (NOT TARGET ${target})
        message(WARNING "bws_add_warnings: target ${target} does not exist")
        return()
    endif()
    if(MSVC)
        # /Zc:__cplusplus: Make MSVC report the real __cplusplus value.
        # Without this, MSVC reports 199711L regardless of standard,
        # breaking BwCompilerFeatures.h gates and its static_assert.
        target_compile_options(${target} PRIVATE /W4 /Zc:__cplusplus)
        if(BWS_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow)
        if(BWS_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

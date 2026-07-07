cmake_minimum_required(VERSION 3.22)
project(bw_rt LANGUAGES CXX)

# =============================================================================
# bw_rt - RT safety infrastructure.
#
# Zero external dependencies beyond stdlib.
# Audio thread detection, violation recording, lock-free ring buffer.
#
# Framework adapters live outside this module. Swap the adapter and this module
# survives unchanged.
# =============================================================================

option(BWS_ENABLE_RT_GUARD "Enable RT guard overrides" ON)
option(BWS_RT_GUARD_DEFER_TO_RTSAN "Disable RT guard global allocation overrides for ThreadSanitizer builds" OFF)

set(_bws_tsan_build OFF)
foreach(_bws_flag_var IN ITEMS CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
    if("${${_bws_flag_var}}" MATCHES "(^| )-fsanitize=([^ ]*,)?thread(,[^ ]*)?($| )")
        set(_bws_tsan_build ON)
    endif()
endforeach()

if(_bws_tsan_build)
    set(BWS_RT_GUARD_DEFER_TO_RTSAN ON CACHE BOOL
        "Disable RT guard global allocation overrides for ThreadSanitizer builds" FORCE)
endif()

set(_bws_enable_rt_guard_overrides ${BWS_ENABLE_RT_GUARD})
if(BWS_RT_GUARD_DEFER_TO_RTSAN)
    set(_bws_enable_rt_guard_overrides OFF)
endif()

set(_bw_rt_sources
    src/AudioThreadScope.cpp
    src/RtViolationBuffer.cpp
    src/RtGuard.cpp
)

if(_bws_enable_rt_guard_overrides)
    list(APPEND _bw_rt_sources src/RtGuardOverrides.cpp)
else()
    list(APPEND _bw_rt_sources src/RtGuardNoOverrides.cpp)
endif()

add_library(bw_rt STATIC ${_bw_rt_sources})

add_library(bws::bw_rt ALIAS bw_rt)

include(BwsPIC)
bws_apply_pic_if_linux(bw_rt)

target_include_directories(bw_rt
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Zero external dependencies. Intentional.
# Framework adapters live outside this module.

target_compile_features(bw_rt PUBLIC cxx_std_20)
target_compile_definitions(bw_rt
    PUBLIC
        BWS_ENABLE_RT_GUARD=$<IF:$<BOOL:${_bws_enable_rt_guard_overrides}>,1,0>
        $<$<BOOL:${BWS_RT_GUARD_DEFER_TO_RTSAN}>:BWS_RT_GUARD_DEFER_TO_RTSAN=1>
)

include(BwsWarnings)
bws_add_warnings(bw_rt)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

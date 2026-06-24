# cmake/BwsModuleSupport.cmake
# =============================================================================
# C++20 named module support detection and wiring.
#
# Sets BW_HAS_MODULES cache variable based on:
#   1. CMake version >= 3.28 (FILE_SET CXX_MODULES support)
#   2. Ninja >= 1.11 (p1689r5 dependency scanning)
#   3. NOT Apple Clang (experimental, unstable - bail out until stable)
#   4. Compiler module support (compile test)
#
# Usage:
# include
#   bws_enable_module_for_target(<target> <cppm_file>)
#
# Graceful degradation: if any requirement is unmet, BW_HAS_MODULES = OFF
# and bws_enable_module_for_target() is a no-op. Header fallback active.
#
# BMI ABI constraint: when modules activate, all consumers must compile
# at the same C++ standard as the BMI. The root CMAKE_CXX_STANDARD (23)
# determines the BMI standard.
# =============================================================================

# -------------------------------------------------------------------------
# bws_enable_module_for_target(<target> <cppm_file>)
#
# Adds a .cppm module interface file to a target's FILE_SET CXX_MODULES.
# PRIVATE compile definition - consumers opt in individually.
# No-op if BW_HAS_MODULES is OFF.
#
# MUST be defined before the detection logic below, because early return()
# exits the entire file - the function would be unreachable if defined
# after return() calls.
# -------------------------------------------------------------------------
function(bws_enable_module_for_target target cppm_file)
    if(NOT BW_HAS_MODULES)
        return()
    endif()

    if(NOT TARGET ${target})
        message(WARNING
            "bws_enable_module_for_target: target '${target}' does not exist"
        )
        return()
    endif()

    get_filename_component(_cppm_dir "${cppm_file}" DIRECTORY)

    target_sources(${target}
        PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS "${_cppm_dir}"
        FILES "${cppm_file}"
    )

    # PRIVATE - only this target's own sources see BW_HAS_MODULES=1.
    # Consumers that want to import must opt in individually.
    target_compile_definitions(${target}
        PRIVATE BW_HAS_MODULES=1
    )

    message(STATUS
        "BwsModuleSupport: Module interface '${cppm_file}' added to '${target}'"
    )
endfunction()

# =========================================================================
# Detection logic - sets BW_HAS_MODULES cache variable
# =========================================================================

option(BWS_ENABLE_CXX_MODULES "Enable optional C++ named-module file sets when the toolchain supports them" OFF)

if(NOT BWS_ENABLE_CXX_MODULES)
    set(BW_HAS_MODULES OFF CACHE BOOL "C++20 named module support" FORCE)
    return()
endif()

# -------------------------------------------------------------------------
# Requirement 1: CMake version
# -------------------------------------------------------------------------
if(CMAKE_VERSION VERSION_LESS "3.28")
    message(STATUS
        "BwsModuleSupport: CMake ${CMAKE_VERSION} < 3.28 - modules disabled."
    )
    set(BW_HAS_MODULES OFF CACHE BOOL "C++20 named module support" FORCE)
    return()
endif()

# -------------------------------------------------------------------------
# Requirement 2: Ninja version (p1689r5 dependency scanning)
# -------------------------------------------------------------------------
if(CMAKE_GENERATOR MATCHES "Ninja")
    execute_process(
        COMMAND ninja --version
        OUTPUT_VARIABLE _ninja_version
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(_ninja_version VERSION_LESS "1.11")
        message(STATUS
            "BwsModuleSupport: Ninja ${_ninja_version} < 1.11 - modules disabled."
        )
        set(BW_HAS_MODULES OFF CACHE BOOL "C++20 named module support" FORCE)
        return()
    endif()
elseif(NOT CMAKE_GENERATOR MATCHES "Visual Studio")
    message(STATUS
        "BwsModuleSupport: Generator '${CMAKE_GENERATOR}' unsupported - modules disabled."
    )
    set(BW_HAS_MODULES OFF CACHE BOOL "C++20 named module support" FORCE)
    return()
endif()

# -------------------------------------------------------------------------
# Requirement 3: Apple Clang bail-out
# Apple Clang has experimental module support but __cpp_modules is
# UNDEFINED (probe data: both c++20 and c++23 mode). BMI generation
# and multi-TU import are unreliable. Default to OFF until Apple
# ships stable module support (likely Xcode 27+).
# -------------------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    message(STATUS
        "BwsModuleSupport: Apple Clang - modules disabled (experimental, unstable)."
    )
    set(BW_HAS_MODULES OFF CACHE BOOL "C++20 named module support" FORCE)
    return()
endif()

# -------------------------------------------------------------------------
# Requirement 4: Compiler module support (compile test)
# Only reached on non-Apple compilers (GCC, upstream Clang, MSVC).
# -------------------------------------------------------------------------
include(CheckSourceCompiles)
set(CMAKE_REQUIRED_FLAGS "-std=c++20")
check_source_compiles(CXX
    "export module bws_test_module;
     export int answer() { return 42; }"
    _BW_MODULES_COMPILE_TEST
)
unset(CMAKE_REQUIRED_FLAGS)

if(NOT _BW_MODULES_COMPILE_TEST)
    message(STATUS
        "BwsModuleSupport: Compiler does not support C++20 modules - disabled."
    )
    set(BW_HAS_MODULES OFF CACHE BOOL "C++20 named module support" FORCE)
    return()
endif()

# -------------------------------------------------------------------------
# All requirements met - enable modules
# -------------------------------------------------------------------------
set(BW_HAS_MODULES ON CACHE BOOL "C++20 named module support" FORCE)
message(STATUS
    "BwsModuleSupport: C++20 named modules ENABLED "
    "(CMake ${CMAKE_VERSION}, compiler verified)"
)

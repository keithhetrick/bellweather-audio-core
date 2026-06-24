cmake_minimum_required(VERSION 3.22)
project(bw_core LANGUAGES CXX)

# bw_core - framework-neutral foundation utilities: fallible-result type (BwResult),
# enum-to-underlying helper (BwToUnderlying), and C++ feature/compiler detection
# (BwCompilerFeatures). Zero dependencies; the base layer ports + types build on.

add_library(bw_core INTERFACE)
add_library(bws::bw_core ALIAS bw_core)

target_include_directories(bw_core INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(bw_core INTERFACE cxx_std_20)

# Gate A: framework boundary in public headers.
add_custom_target(bw_core_gate_a
    COMMAND bash -c
        "! grep -rE 'juce::|#include.*juce' \
        '${CMAKE_CURRENT_SOURCE_DIR}/include/' \
        | grep -vE '^\\ *\\*|^\\ *//' \
        | grep -q . \
        && echo 'Gate A PASSED: framework boundary holds in bw_core headers' \
        || (echo 'Gate A FAILED: JUCE found in bw_core headers' && exit 1)"
    COMMENT "Gate A: bw_core header purity"
    VERBATIM
)

if(BUILD_TESTING)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
        add_subdirectory(tests)
    endif()
endif()

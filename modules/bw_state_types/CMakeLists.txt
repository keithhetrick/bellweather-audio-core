cmake_minimum_required(VERSION 3.22)
project(bw_state_types LANGUAGES CXX)

# bw_state_types - framework-neutral state/persistence value types: serialization payload
# (BwStateBlob), preset metadata, and host-environment snapshot (SystemInfo).
# Depends only on the standard library.

add_library(bw_state_types INTERFACE)
add_library(bws::bw_state_types ALIAS bw_state_types)

target_include_directories(bw_state_types INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(bw_state_types INTERFACE cxx_std_20)

add_custom_target(bw_state_types_gate_a
    COMMAND bash -c
        "! grep -rE 'juce::|#include.*juce' \
        '${CMAKE_CURRENT_SOURCE_DIR}/include/' \
        | grep -vE '^\\ *\\*|^\\ *//' \
        | grep -q . \
        && echo 'Gate A PASSED: framework boundary holds in bw_state_types headers' \
        || (echo 'Gate A FAILED: JUCE found in bw_state_types headers' && exit 1)"
    COMMENT "Gate A: bw_state_types header purity"
    VERBATIM
)

if(BUILD_TESTING)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
        add_subdirectory(tests)
    endif()
endif()

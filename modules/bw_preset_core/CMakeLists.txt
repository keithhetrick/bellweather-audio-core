cmake_minimum_required(VERSION 3.22)
project(bw_preset_core LANGUAGES CXX)

add_library(bw_preset_core STATIC
    src/bw_preset_core_link_anchor.cpp
)
add_library(bws::bw_preset_core ALIAS bw_preset_core)

target_include_directories(bw_preset_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(bw_preset_core
    PUBLIC
        bws::bw_state_types
)

target_compile_features(bw_preset_core PUBLIC cxx_std_20)

add_custom_target(bw_preset_core_gate_a
    COMMAND ${CMAKE_COMMAND} -E echo
        "Running Gate A: bw_preset_core framework-boundary scan..."
    COMMAND bash -c
        "if grep -rE 'juce::|#include.*juce' \
        '${CMAKE_CURRENT_SOURCE_DIR}/include/' \
        '${CMAKE_CURRENT_SOURCE_DIR}/src/' \
        | grep -vE '^\\ *\\*|^\\ *//' \
        | grep -q .; then \
            echo 'Gate A FAILED: framework-specific code found in bw_preset_core'; \
            exit 1; \
        else \
            echo 'Gate A PASSED: framework boundary holds in bw_preset_core'; \
        fi"
    COMMENT "Gate A: bw_preset_core framework boundary"
    VERBATIM
)

add_test(
    NAME "bw_preset_core: framework boundary gate"
    COMMAND bash -c
        "if grep -rE 'juce::|#include.*juce' \
        '${CMAKE_CURRENT_SOURCE_DIR}/include/' \
        '${CMAKE_CURRENT_SOURCE_DIR}/src/' \
        | grep -vE '^\\ *\\*|^\\ *//' \
        | grep -q .; then \
            echo 'Gate A FAILED: framework-specific code found in bw_preset_core'; \
            exit 1; \
        else \
            echo 'Gate A PASSED: framework boundary holds in bw_preset_core'; \
        fi"
)

cmake_minimum_required(VERSION 3.22)
project(bw_ports LANGUAGES CXX)

# bw_ports - the hexagon boundary: abstract interfaces the application exposes to
# adapters (scheduling, HTTP, plugin host, preset/state
# persistence). Pure interfaces; speaks in the foundation + value types below.

add_library(bw_ports INTERFACE)
add_library(bws::bw_ports ALIAS bw_ports)

target_include_directories(bw_ports INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(bw_ports INTERFACE cxx_std_20)

target_link_libraries(bw_ports INTERFACE
    bws::bw_core
    bws::bw_audio_types
    bws::bw_state_types
)

add_custom_target(bw_ports_gate_a
    COMMAND bash -c
        "! grep -rE 'juce::|#include.*juce' \
        '${CMAKE_CURRENT_SOURCE_DIR}/include/' \
        | grep -vE '^\\ *\\*|^\\ *//' \
        | grep -q . \
        && echo 'Gate A PASSED: framework boundary holds in bw_ports headers' \
        || (echo 'Gate A FAILED: JUCE found in bw_ports headers' && exit 1)"
    COMMENT "Gate A: bw_ports header purity"
    VERBATIM
)

if(BUILD_TESTING)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
        add_subdirectory(tests)
    endif()
endif()

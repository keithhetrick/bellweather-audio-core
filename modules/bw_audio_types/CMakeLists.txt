cmake_minimum_required(VERSION 3.22)
project(bw_audio_types LANGUAGES CXX)

# bw_audio_types - framework-neutral audio value types: buffer views (owning + non-owning),
# process context, parameter snapshot, transport state, biquad filter, and linear
# parameter smoothers. The DSP foundation; depends only on the standard library.

add_library(bw_audio_types INTERFACE)
add_library(bws::bw_audio_types ALIAS bw_audio_types)

target_include_directories(bw_audio_types INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(bw_audio_types INTERFACE cxx_std_20)

# Gate A: framework boundary in public headers.
add_custom_target(bw_audio_types_gate_a
    COMMAND bash -c
        "! grep -rE 'juce::|#include.*juce' \
        '${CMAKE_CURRENT_SOURCE_DIR}/include/' \
        | grep -vE '^\\ *\\*|^\\ *//' \
        | grep -q . \
        && echo 'Gate A PASSED: framework boundary holds in bw_audio_types headers' \
        || (echo 'Gate A FAILED: JUCE found in bw_audio_types headers' && exit 1)"
    COMMENT "Gate A: bw_audio_types header purity"
    VERBATIM
)

if(BUILD_TESTING)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
        add_subdirectory(tests)
    endif()
endif()

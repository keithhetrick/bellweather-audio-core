cmake_minimum_required(VERSION 3.22)
project(bw_dsp_processors LANGUAGES CXX)

# bw_dsp_processors - signal chain tools (EQ, filters, stereo, channel ops)

add_library(bw_dsp_processors INTERFACE)
add_library(bws::bw_dsp_processors ALIAS bw_dsp_processors)

target_include_directories(bw_dsp_processors INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(bw_dsp_processors INTERFACE cxx_std_20)

target_link_libraries(bw_dsp_processors INTERFACE
    bws::bw_dsp_core
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

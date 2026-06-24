cmake_minimum_required(VERSION 3.22)
project(bw_dsp_metering LANGUAGES CXX)

# bw_dsp_metering - measurement, safety, metering primitives

add_library(bw_dsp_metering INTERFACE)
add_library(bws::bw_dsp_metering ALIAS bw_dsp_metering)

target_include_directories(bw_dsp_metering INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(bw_dsp_metering INTERFACE cxx_std_20)

target_link_libraries(bw_dsp_metering INTERFACE
    bws::bw_dsp_core
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

cmake_minimum_required(VERSION 3.22)
project(bw_dsp_core LANGUAGES CXX)

# bw_dsp_core - framework-neutral DSP infrastructure
# DspConstants, ChainSpec, concepts, and DSP infrastructure

add_library(bw_dsp_core INTERFACE)
add_library(bws::bw_dsp_core ALIAS bw_dsp_core)

target_include_directories(bw_dsp_core INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(bw_dsp_core INTERFACE cxx_std_20)

target_link_libraries(bw_dsp_core INTERFACE
    bws::bw_core
    bws::bw_audio_types
    bws::bw_fft_adapters
)

# AVX2+FMA on x86_64 (Haswell-class and newer); ARM64 uses NEON by default.
# Carried by bw_dsp_core so DSP headers receive consistent scalar/FMA codegen
# and SIMD-capable consumers inherit the same baseline. Consumers targeting
# pre-AVX2 x86 can configure -DBWS_ENABLE_AVX2=OFF.
option(BWS_ENABLE_AVX2 "Emit AVX2+FMA on x86_64" ON)
if(BWS_ENABLE_AVX2 AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    if(MSVC)
        target_compile_options(bw_dsp_core INTERFACE /arch:AVX2)
    else()
        target_compile_options(bw_dsp_core INTERFACE -mavx2 -mfma)
    endif()
endif()

# Optional module tests live under tests/ when present.
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

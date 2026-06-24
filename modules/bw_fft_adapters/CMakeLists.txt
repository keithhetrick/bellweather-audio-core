cmake_minimum_required(VERSION 3.22)
project(bw_fft_adapters LANGUAGES CXX C)

add_library(bw_fft_adapters STATIC
    src/BwsFFT.cpp
    vendor/pffft/pffft.c
)
add_library(bws::bw_fft_adapters ALIAS bw_fft_adapters)

if(MSVC)
    set_source_files_properties(vendor/pffft/pffft.c PROPERTIES COMPILE_OPTIONS /wd4456)
else()
    set_source_files_properties(vendor/pffft/pffft.c PROPERTIES COMPILE_OPTIONS -Wno-shadow)
endif()

target_include_directories(bw_fft_adapters
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vendor/pffft
)

target_compile_features(bw_fft_adapters PUBLIC cxx_std_20)

if(APPLE)
    target_link_libraries(bw_fft_adapters PUBLIC "-framework Accelerate")
    target_compile_definitions(bw_fft_adapters PRIVATE BWS_FFT_USE_VDSP=1)
else()
    target_compile_definitions(bw_fft_adapters PRIVATE BWS_FFT_USE_PFFFT=1)
endif()

include(BwsWarnings)
bws_add_warnings(bw_fft_adapters)

include(BwsPIC)
bws_apply_pic_if_linux(bw_fft_adapters)

add_subdirectory(tests)

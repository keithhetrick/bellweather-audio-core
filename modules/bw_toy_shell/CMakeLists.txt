cmake_minimum_required(VERSION 3.22)
project(bw_toy_shell LANGUAGES CXX)

# bw_toy_shell - shared primitives for plugin easter-egg games.

add_library(bw_toy_shell STATIC
    src/TimerPump.cpp
    src/HiddenClick.cpp
    src/HintCard.cpp
    src/HudUtils.cpp
)
add_library(bws::bw_toy_shell ALIAS bw_toy_shell)

target_include_directories(bw_toy_shell
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(bw_toy_shell PUBLIC cxx_std_20)

target_link_libraries(bw_toy_shell
    PUBLIC
        juce::juce_gui_basics
)

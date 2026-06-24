# Module build order (a module's dependencies precede it).
add_subdirectory(bw_core)
add_subdirectory(bw_audio_types)
add_subdirectory(bw_state_types)
add_subdirectory(bw_ports)
add_subdirectory(bw_rt)
add_subdirectory(bw_fft_adapters)
add_subdirectory(bw_dsp_core)
add_subdirectory(bw_dsp_metering)
add_subdirectory(bw_dsp_processors)
if(BWS_BUILD_BAROMETER_PLUGIN)  # Barometer support
    add_subdirectory(bw_preset_core)
endif()
if(BWS_BUILD_BAROMETER_PLUGIN)  # Barometer support
    add_subdirectory(bw_juce_adapters)
endif()
if(BWS_BUILD_BAROMETER_PLUGIN)  # Barometer support
    add_subdirectory(bw_toy_shell)
endif()
if(BWS_BUILD_BAROMETER_PLUGIN)  # Barometer support
    add_subdirectory(bw_ui)
endif()

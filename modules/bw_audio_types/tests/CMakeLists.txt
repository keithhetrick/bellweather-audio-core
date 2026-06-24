include(BwsCatch2)
bws_fetch_catch2()

add_executable(bw_audio_types_tests
    test_buffer_view.cpp
    test_const_buffer_view.cpp
    test_param_snapshot.cpp
    test_process_context.cpp
    test_rt_smoothed_param.cpp
    test_transport_state.cpp
    test_biquad_magnitude.cpp
)

target_link_libraries(bw_audio_types_tests PRIVATE
    bws::bw_audio_types
    Catch2::Catch2WithMain
)

target_compile_features(bw_audio_types_tests PRIVATE cxx_std_20)

catch_discover_tests(bw_audio_types_tests PROPERTIES LABELS "bws-library")

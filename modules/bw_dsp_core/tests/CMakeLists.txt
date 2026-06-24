# bw_dsp_core unit tests. New utilities add their test files to this executable;
# the parent gates this dir under if(BUILD_TESTING). Catch2 + catch_discover_tests.

include(BwsCatch2)
bws_fetch_catch2()

add_executable(bw_dsp_core_tests
    test_dsp_constants.cpp
    test_tpt_one_pole.cpp
)

# Internal scheduling/routing tests, built when their sources are present.
foreach(internal_test
        test_transport_edge_tracker.cpp
        test_sub_block_scheduler.cpp
        test_antiderivative_aa.cpp
        test_engine_router_scrub.cpp
        test_crossfade_sr_invariance.cpp)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${internal_test}")
        target_sources(bw_dsp_core_tests PRIVATE ${internal_test})
    endif()
endforeach()

target_link_libraries(bw_dsp_core_tests PRIVATE
    bws::bw_dsp_core
    Catch2::Catch2WithMain
)

target_compile_features(bw_dsp_core_tests PRIVATE cxx_std_20)

catch_discover_tests(bw_dsp_core_tests PROPERTIES LABELS "bws-library")

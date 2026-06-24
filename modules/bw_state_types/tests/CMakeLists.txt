include(BwsCatch2)
bws_fetch_catch2()

add_executable(bw_state_types_tests
    test_bw_state_blob.cpp
)

target_link_libraries(bw_state_types_tests PRIVATE
    bws::bw_state_types
    Catch2::Catch2WithMain
)

target_compile_features(bw_state_types_tests PRIVATE cxx_std_20)

catch_discover_tests(bw_state_types_tests PROPERTIES LABELS "bws-library")

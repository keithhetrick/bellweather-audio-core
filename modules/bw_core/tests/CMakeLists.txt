include(BwsCatch2)
bws_fetch_catch2()

add_executable(bw_core_tests
    test_bw_result.cpp
)

target_link_libraries(bw_core_tests PRIVATE
    bws::bw_core
    Catch2::Catch2WithMain
)

target_compile_features(bw_core_tests PRIVATE cxx_std_20)

catch_discover_tests(bw_core_tests PROPERTIES LABELS "bws-library")

# Second build of the same suite with the C++20 fallback path forced, so the
# non-std::expected BwResult is actually compiled and exercised on this toolchain.
add_executable(bw_core_tests_fallback
    test_bw_result.cpp
)

target_compile_definitions(bw_core_tests_fallback PRIVATE BW_HAS_EXPECTED=0)

target_link_libraries(bw_core_tests_fallback PRIVATE
    bws::bw_core
    Catch2::Catch2WithMain
)

target_compile_features(bw_core_tests_fallback PRIVATE cxx_std_20)

catch_discover_tests(bw_core_tests_fallback PROPERTIES LABELS "bws-library")

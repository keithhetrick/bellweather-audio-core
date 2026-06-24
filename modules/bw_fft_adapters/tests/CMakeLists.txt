include(BwsCatch2)
bws_fetch_catch2()

add_executable(bw_fft_adapters_tests test_fft.cpp)

target_link_libraries(bw_fft_adapters_tests PRIVATE
    bws::bw_fft_adapters
    Catch2::Catch2WithMain
)

catch_discover_tests(bw_fft_adapters_tests PROPERTIES LABELS "bws-library")

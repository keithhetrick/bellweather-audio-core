include(BwsCatch2)
bws_fetch_catch2()

add_executable(bw_ports_tests
    test_preset_state_persistence_contract.cpp
)

target_link_libraries(bw_ports_tests PRIVATE
    bws::bw_ports
    Catch2::Catch2WithMain
)

target_compile_features(bw_ports_tests PRIVATE cxx_std_20)

catch_discover_tests(bw_ports_tests PROPERTIES LABELS "bws-library")

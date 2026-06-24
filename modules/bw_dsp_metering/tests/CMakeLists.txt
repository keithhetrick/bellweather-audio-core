# bw_dsp_metering unit tests
#
# Loudness (Bs1770Meter) + true-peak conformance/behavior tests. New test files
# add to this same target; the parent gates this dir under if(BUILD_TESTING).

include(BwsCatch2)
bws_fetch_catch2()

set(BW_TECH3341_FIXTURES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tech3341")

add_executable(bw_dsp_metering_tests
    alloc_counter_global_operators.cpp
    bs1770_meter_test.cpp
    bs1770_conformance_test.cpp
    true_peak_meter_test.cpp
    meter_ballistics_test.cpp
)

target_include_directories(bw_dsp_metering_tests PRIVATE
    # Shared test helpers (AllocCounter).
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(bw_dsp_metering_tests PRIVATE
    bws::bw_dsp_metering
    Catch2::Catch2WithMain
)

target_compile_definitions(bw_dsp_metering_tests PRIVATE
    BW_TECH3341_FIXTURES_DIR="${BW_TECH3341_FIXTURES_DIR}"
)


target_compile_features(bw_dsp_metering_tests PRIVATE cxx_std_20)

add_test(NAME bw_dsp_metering_all
    COMMAND bw_dsp_metering_tests --reporter console)
set_tests_properties(bw_dsp_metering_all PROPERTIES
    LABELS "bpts_calibration")

add_test(NAME bw_dsp_metering_meter_ballistics_gate
    COMMAND bw_dsp_metering_tests "[meter_ballistics]" --reporter console --success)
set_tests_properties(bw_dsp_metering_meter_ballistics_gate PROPERTIES
    LABELS "bpts_gate"
    FAIL_REGULAR_EXPRESSION "No test cases matched")

# BS.1770 / EBU Tech 3341 conformance: EBU published-absolute values, 48 kHz-
# anchored sample-rate correctness, and calibration-independent structural invariants.
add_test(NAME bw_dsp_metering_calibration
    COMMAND bw_dsp_metering_tests "[calibration]" --reporter console --success)
set_tests_properties(bw_dsp_metering_calibration PROPERTIES
    LABELS "bpts_calibration"
    FAIL_REGULAR_EXPRESSION "No test cases matched")

# Bs1770Meter analytics-lifecycle + LRA-stability + small-block RT-safety lane.
# Not spec-calibration, so it has its own ctest entry. The conformance CI selects
# it by name-regex (`ctest -R bw_dsp_metering`); the bpts_calibration LABEL is
# retained for label-based local runs (`ctest -L bpts_calibration`).
add_test(NAME bw_dsp_metering_analytics
    COMMAND bw_dsp_metering_tests "[bs1770][analytics]" --reporter console --success)
set_tests_properties(bw_dsp_metering_analytics PROPERTIES
    LABELS "bpts_calibration"
    FAIL_REGULAR_EXPRESSION "No test cases matched")

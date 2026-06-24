# bw_dsp_processors unit tests

include(BwsCatch2)
bws_fetch_catch2()

add_executable(bw_dsp_processors_tests
    utility_processors_test.cpp
)

target_link_libraries(bw_dsp_processors_tests PRIVATE
    bws::bw_dsp_processors
    bws::bw_dsp_core      # PhaseInverter
    Catch2::Catch2WithMain
)

# Air tests need bw_dsp_effects; built only when those sources are present.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/air_dark_processor_test.cpp")
    target_sources(bw_dsp_processors_tests PRIVATE
        air_dark_processor_test.cpp
        air_tone_macro_test.cpp
    )
    target_link_libraries(bw_dsp_processors_tests PRIVATE bws::bw_dsp_effects)
endif()

target_compile_features(bw_dsp_processors_tests PRIVATE cxx_std_20)

catch_discover_tests(bw_dsp_processors_tests PROPERTIES LABELS "bws-library")

# Gate-enforced contract lane for the shared utility processors.
add_test(NAME bw_dsp_processors_utility_gate
    COMMAND bw_dsp_processors_tests "[utility]" --reporter console --success)
set_tests_properties(bw_dsp_processors_utility_gate PROPERTIES
    LABELS "bpts_gate"
    FAIL_REGULAR_EXPRESSION "No test cases matched")

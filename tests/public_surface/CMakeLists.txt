find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(BWS_PUBLIC_SURFACE_MODULES
    bw_core
    bw_audio_types
    bw_state_types
    bw_ports
    bw_rt
    bw_fft_adapters
    bw_dsp_core
    bw_dsp_metering
    bw_dsp_processors
)

execute_process(
    COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_SOURCE_DIR}/tools/check_public_surface_smokes.py"
            --cmake-list "${CMAKE_CURRENT_LIST_FILE}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE _bws_public_surface_check
)
if(NOT _bws_public_surface_check EQUAL 0)
    message(FATAL_ERROR "Public-surface smoke list is out of sync with docs/MODULES.md")
endif()

include(BwsWarnings)

add_custom_target(bw_public_surface_compile_tests)

foreach(_module IN LISTS BWS_PUBLIC_SURFACE_MODULES)
    set(_target "${_module}_public_surface_smoke")
    add_executable("${_target}" "${_module}_public_surface_smoke.cpp")
    target_link_libraries("${_target}" PRIVATE "bws::${_module}")
    target_compile_features("${_target}" PRIVATE cxx_std_20)

    # Each smoke TU compiles its module's public headers, so warnings (and
    # -Werror under BWS_WARNINGS_AS_ERRORS) reach the header-only modules here.
    bws_add_warnings("${_target}")

    # These smoke TUs do not use C++20/23 named modules. CMake otherwise turns on
    # module dependency scanning for C++20+ targets, which requires a
    # clang-scan-deps binary the cleanroom toolchain does not ship. Disable
    # scanning so the build does not hard-fail on a missing scanner.
    set_target_properties("${_target}" PROPERTIES CXX_SCAN_FOR_MODULES OFF)

    add_dependencies(bw_public_surface_compile_tests "${_target}")
    add_test(NAME "${_module}: public surface smoke" COMMAND "${_target}")
    set_tests_properties("${_module}: public surface smoke" PROPERTIES LABELS "public-surface")
endforeach()

if(BUILD_TESTING)
    include(BwsCatch2)
    bws_fetch_catch2()

    add_executable(bw_rt_tests
        test_rt_guard_alloc.cpp
        test_rt_containment_observability.cpp
        test_lock_free_ring_buffer.cpp
        test_rt_safety_macros.cpp
        test_cross_stage_invariant.cpp
        test_composition_fuzz.cpp
        test_snap_to_zero.cpp
    )

    target_link_libraries(bw_rt_tests PRIVATE
        bw_rt
        Catch2::Catch2WithMain
    )

    target_compile_features(bw_rt_tests PRIVATE cxx_std_20)

    if(NOT BWS_RT_GUARD_DEFER_TO_RTSAN)
        target_compile_definitions(bw_rt_tests PRIVATE
            BWS_ENABLE_RT_GUARD=1
        )
    endif()

    # Per-target function-effect strictness for the RtSafety smoke test.
    if (BWS_ENABLE_FEA_STRICT)
        target_compile_options(bw_rt_tests PRIVATE
            -Werror=function-effects
            -Werror=perf-constraint-implies-noexcept)
    endif()

    catch_discover_tests(bw_rt_tests PROPERTIES LABELS "bws-library")
endif()

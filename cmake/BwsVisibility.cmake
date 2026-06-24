function(bws_apply_hidden_visibility target)
    if (NOT TARGET ${target})
        message(WARNING "bws_apply_hidden_visibility: target ${target} does not exist")
        return()
    endif()

    # Align visibility for all TU linked together to avoid weak-symbol/visibility ld warnings.
    set_target_properties(${target} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target} PRIVATE
            -fvisibility=hidden
            -fvisibility-inlines-hidden
        )
    endif()
endfunction()

# cmake/BwsPIC.cmake
# Linux PIC guard - avoids TLS relocation errors when modules are
# linked into plugin shared objects.
function(bws_apply_pic_if_linux target)
    if (NOT TARGET ${target})
        message(WARNING "bws_apply_pic_if_linux: target ${target} does not exist")
        return()
    endif()
    if(UNIX AND NOT APPLE)
        set_property(TARGET ${target} PROPERTY POSITION_INDEPENDENT_CODE ON)
    endif()
endfunction()

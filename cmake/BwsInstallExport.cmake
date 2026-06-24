# Installs the reusable library as the BellweatherAudioCore find_package package
# (namespaced bws:: targets).
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Captured at include-scope: CMAKE_CURRENT_LIST_DIR inside the function resolves
# to the caller's list dir, not this module's.
set(_BWS_INSTALL_EXPORT_DIR "${CMAKE_CURRENT_LIST_DIR}")

# bws_install_audio_core(<target>...) - install the targets, their headers, and a
# relocatable package config so downstreams can find_package(BellweatherAudioCore).
function(bws_install_audio_core)
    set(_targets ${ARGN})

    install(TARGETS ${_targets}
        EXPORT BellweatherAudioCoreTargets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

    foreach(_t IN LISTS _targets)
        install(DIRECTORY "${CMAKE_SOURCE_DIR}/modules/${_t}/include/"
                DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
    endforeach()

    set(_cmakedir "${CMAKE_INSTALL_LIBDIR}/cmake/BellweatherAudioCore")

    install(EXPORT BellweatherAudioCoreTargets
        NAMESPACE bws::
        FILE BellweatherAudioCoreTargets.cmake
        DESTINATION "${_cmakedir}")

    configure_package_config_file(
        "${_BWS_INSTALL_EXPORT_DIR}/BellweatherAudioCoreConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/BellweatherAudioCoreConfig.cmake"
        INSTALL_DESTINATION "${_cmakedir}")

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/BellweatherAudioCoreConfigVersion.cmake"
        VERSION "${PROJECT_VERSION}"
        COMPATIBILITY SameMajorVersion)

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/BellweatherAudioCoreConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/BellweatherAudioCoreConfigVersion.cmake"
        DESTINATION "${_cmakedir}")
endfunction()

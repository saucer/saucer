function(nuget_message LEVEL MESSAGE)
    message(${LEVEL} "NuGet: ${MESSAGE}")
endfunction()

function(nuget_find OUTPUT)
    set(nuget_PATH "${CMAKE_CURRENT_BINARY_DIR}")
    cmake_path(APPEND nuget_PATH "nuget" "bin" "nuget.exe")

    if (EXISTS "${nuget_PATH}")
        set(${OUTPUT} "${nuget_PATH}" PARENT_SCOPE)
        return()
    endif()

    find_program(NUGET nuget)

    if (NOT NUGET MATCHES "NOTFOUND")
        set(${OUTPUT} "${NUGET}" PARENT_SCOPE)
        return()
    endif()

    nuget_message(STATUS "Downloading to '${nuget_PATH}'")
    file(DOWNLOAD "https://dist.nuget.org/win-x86-commandline/latest/nuget.exe" "${nuget_PATH}")

    set(${OUTPUT} "${nuget_PATH}" PARENT_SCOPE)
endfunction()

function(nuget_setup OUTPUT)
    nuget_find(nuget_PATH)

    # Ensure that the default NuGet-Sources are present

    execute_process(
        COMMAND         "${nuget_PATH}" sources
        OUTPUT_VARIABLE nuget_SOURCES
        COMMAND_ERROR_IS_FATAL ANY
    )

    if (NOT "${nuget_SOURCES}" MATCHES "https:\/\/api\.nuget\.org\/v3\/index\.json")
        execute_process(
            COMMAND "${nuget_PATH}" sources Add -Name "nuget.org" -Source "https://api.nuget.org/v3/index.json"
            OUTPUT_QUIET
            COMMAND_ERROR_IS_FATAL ANY
        )
        nuget_message(STATUS "Added 'nuget.org' source")
    endif()

    set(${OUTPUT} "${nuget_PATH}" PARENT_SCOPE)
endfunction()

function(NuGetCreateLibrary)
    cmake_parse_arguments(nuget "" "NAME;VERSION;INCLUDE;LIBRARY" "" ${ARGN})

    add_library(${nuget_NAME} STATIC IMPORTED)

    set_target_properties(${nuget_NAME} PROPERTIES
        IMPORTED_LOCATION             "${nuget_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${nuget_INCLUDE}"
    )

    if (NOT saucer_install)
        return()
    endif()

    include(CMakePackageConfigHelpers)
    include(GNUInstallDirs)

    install(
        FILES       ${nuget_LIBRARY}
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/${nuget_NAME}-${nuget_VERSION}
    )

    install(
        DIRECTORY   ${nuget_INCLUDE}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${nuget_NAME}-${nuget_VERSION}
    )

    cmake_path(GET nuget_LIBRARY FILENAME package_LIBRARY)

    configure_package_config_file(
        cmake/nugetConfig.cmake.in
        ${PROJECT_BINARY_DIR}/${nuget_NAME}Config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${nuget_NAME}-${nuget_VERSION}
        PATH_VARS CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_INCLUDEDIR
    )

    write_basic_package_version_file(
        ${PROJECT_BINARY_DIR}/${nuget_NAME}ConfigVersion.cmake
        VERSION ${nuget_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(
        FILES ${PROJECT_BINARY_DIR}/${nuget_NAME}Config.cmake ${PROJECT_BINARY_DIR}/${nuget_NAME}ConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${nuget_NAME}-${nuget_VERSION}
    )
endfunction()

function(NugetAddPackage)
    cmake_parse_arguments(nuget "" "NAME;PACKAGE;VERSION;INSTALL" "LIBRARY;INCLUDE;ALTERNATIVES" ${ARGN})

    nuget_setup(nuget_PATH)
    nuget_message(STATUS "Adding package ${nuget_PACKAGE}@${nuget_VERSION}")

    set(package_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    cmake_path(APPEND package_DIRECTORY "nuget" "packages")

    set(package_PATH "${package_DIRECTORY}")
    cmake_path(APPEND package_PATH "${nuget_PACKAGE}.${nuget_VERSION}")

    if (NOT EXISTS "${package_PATH}")
        execute_process(
            COMMAND "${nuget_PATH}" install "${nuget_PACKAGE}" -Version "${nuget_VERSION}" -OutputDirectory "${package_DIRECTORY}"
            OUTPUT_QUIET
            COMMAND_ERROR_IS_FATAL ANY
        )
    endif()

    set(${nuget_NAME}_PATH "${package_PATH}" PARENT_SCOPE)

    set(package_GLOB "${package_PATH}")
    cmake_path(APPEND package_GLOB ${nuget_LIBRARY})
    file(GLOB_RECURSE package_LIBRARY "${package_GLOB}")

    set(package_INCLUDE "${package_PATH}")
    cmake_path(APPEND package_INCLUDE ${nuget_INCLUDE})

    NuGetCreateLibrary(
        NAME      ${nuget_NAME}
        VERSION   ${nuget_VERSION}
        INCLUDE   ${package_INCLUDE}
        LIBRARY   ${package_LIBRARY}
    )
endfunction()

function(NuGetFindPackage)
    cmake_parse_arguments(nuget "" "NAME" "ALTERNATIVES" ${ARGN})

    if (NOT saucer_prefer_remote AND NOT TARGET "${nuget_NAME}")
        foreach(alternative IN LISTS nuget_ALTERNATIVES)
            string(REPLACE "|" ";" alternative "${alternative}")

            list(GET alternative 0 alternative_PACKAGE)
            list(GET alternative 1 alternative_TARGET)

            find_package(${alternative_PACKAGE} QUIET)

            if (NOT ${alternative_PACKAGE}_FOUND)
                continue()
            endif()

            add_library(${nuget_NAME} ALIAS ${alternative_TARGET})
            break()
        endforeach()
    endif()

    if (TARGET ${nuget_NAME})
        nuget_message(STATUS "Found package ${nuget_NAME}")
        return()
    endif()

    NuGetAddPackage(${ARGV})
endfunction()

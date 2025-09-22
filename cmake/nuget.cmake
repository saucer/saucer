function(nuget_message LEVEL MESSAGE)
    message(${LEVEL} "NuGet: ${MESSAGE}")
endfunction()

function(nuget_setup OUTPUT)
    set(nuget_PATH "${CMAKE_CURRENT_BINARY_DIR}")
    cmake_path(APPEND nuget_PATH "nuget" "bin" "nuget.exe")

    if (NOT EXISTS "${nuget_PATH}")
        find_program(NUGET nuget)
    endif()

    if (NUGET MATCHES "NOTFOUND")
        file(DOWNLOAD "https://dist.nuget.org/win-x86-commandline/latest/nuget.exe" "${nuget_PATH}")
        nuget_message(STATUS "Downloading to '${nuget_PATH}'")
    else()
        set(nuget_PATH "${NUGET}")
    endif()

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

function(NugetAddPackage)
    cmake_parse_arguments(nuget "" "NAME;PACKAGE;VERSION" "LIBRARIES;INCLUDES;ALTERNATIVES" ${ARGN})

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
    cmake_path(APPEND package_GLOB ${nuget_LIBRARIES})
    file(GLOB_RECURSE package_LIBRARIES "${package_GLOB}")

    set(package_INCLUDES "${package_PATH}")
    cmake_path(APPEND package_INCLUDES ${nuget_INCLUDES})

    add_library(${nuget_NAME} STATIC IMPORTED)

    set_target_properties(${nuget_NAME} PROPERTIES
        IMPORTED_LOCATION "${package_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${package_INCLUDES}"
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

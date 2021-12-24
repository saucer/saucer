function(nuget_add OUTPUT_NAME NAME VERSION)
    find_program(NUGET nuget)

    if (NUGET STREQUAL "NUGET-NOTFOUND")
        if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/tools/nuget-cli.exe")
            message(STATUS "[NuGet] NuGet-CLI not found, downloading...")
            file(DOWNLOAD "https://dist.nuget.org/win-x86-commandline/latest/nuget.exe" "${CMAKE_CURRENT_BINARY_DIR}/tools/nuget-cli.exe")
        endif()
        set(NUGET "${CMAKE_CURRENT_BINARY_DIR}/tools/nuget-cli.exe")
    endif()

    execute_process(COMMAND ${NUGET} install ${NAME} -Version ${VERSION} -ExcludeVersion -OutputDirectory ${CMAKE_CURRENT_BINARY_DIR}/packages OUTPUT_QUIET COMMAND_ERROR_IS_FATAL ANY)
    set(${OUTPUT_NAME}_PATH "${CMAKE_CURRENT_BINARY_DIR}/packages/${NAME}" PARENT_SCOPE)
endfunction()
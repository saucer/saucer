function(nuget_add OUTPUT_NAME NAME VERSION)
    find_program(NUGET nuget)

    message(STATUS "[NuGet] Downloading Package: ${NAME} [${VERSION}]")

    if (NUGET STREQUAL "NUGET-NOTFOUND")
        if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/nuget/nuget.exe")
            message(STATUS "[NuGet] NuGet-CLI not found, downloading...")
            file(DOWNLOAD "https://dist.nuget.org/win-x86-commandline/latest/nuget.exe" "${CMAKE_CURRENT_BINARY_DIR}/nuget/nuget.exe")
        endif()

        set(NUGET "${CMAKE_CURRENT_BINARY_DIR}/nuget/nuget.exe")
    endif()
    
    execute_process(COMMAND 
        ${NUGET} help 
        OUTPUT_VARIABLE NUGET_INFO
    )
    
    string(REGEX MATCH "NuGet Version: ([0-9.]+)" _ "${NUGET_INFO}")
    message(STATUS "[NuGet] Found at: ${NUGET} [${CMAKE_MATCH_1}]")

    execute_process(COMMAND 
        ${NUGET} sources 
        OUTPUT_VARIABLE NUGET_SOURCES
    )
    
    if (NOT "${NUGET_SOURCES}" MATCHES "https:\/\/api\.nuget\.org\/v3\/index\.json")
        message(WARNING "[NuGet] Official NuGet Sources missing!")
        
        execute_process(COMMAND 
            ${NUGET} sources Add -Name "nuget.org" -Source "https://api.nuget.org/v3/index.json"
        )
    endif()

    execute_process(COMMAND 
        ${NUGET} install ${NAME} -Version ${VERSION} -ExcludeVersion -OutputDirectory "${CMAKE_CURRENT_BINARY_DIR}/nuget" 
        OUTPUT_QUIET COMMAND_ERROR_IS_FATAL ANY
    )
    
    set(${OUTPUT_NAME}_PATH "${CMAKE_CURRENT_BINARY_DIR}/nuget/${NAME}" PARENT_SCOPE)
endfunction()
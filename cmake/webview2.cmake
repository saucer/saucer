include("nuget.cmake")

if (saucer_windows_arch STREQUAL "Default")
    if (CMAKE_GENERATOR_PLATFORM)
        set(saucer_windows_arch "${CMAKE_GENERATOR_PLATFORM}")
    elseif (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(saucer_windows_arch "x64")
    else()
        set(saucer_windows_arch "Win32")
    endif()

    message(STATUS "[saucer] Windows Arch is 'Default', using ${saucer_windows_arch}")
endif()

if (NOT saucer_prefer_remote)
    find_package(webview2 REQUIRED)
    return()
endif()

if (NOT TARGET webview2::webview2)
    nuget_add(WebView2 "Microsoft.Web.WebView2" ${saucer_webview2_version})

    add_library(webview2 STATIC IMPORTED)
    add_library(webview2::webview2 ALIAS webview2)

    if (saucer_windows_arch STREQUAL "ARM64")
        set(webview2_lib_path "${WebView2_PATH}/build/native/arm64/WebView2LoaderStatic.lib")
    elseif (saucer_windows_arch STREQUAL "x64")
        set(webview2_lib_path "${WebView2_PATH}/build/native/x64/WebView2LoaderStatic.lib")
    else()
        set(webview2_lib_path "${WebView2_PATH}/build/native/x86/WebView2LoaderStatic.lib")
    endif()

    set_target_properties(webview2 PROPERTIES
        IMPORTED_LOCATION "${webview2_lib_path}"
        INTERFACE_INCLUDE_DIRECTORIES "${WebView2_PATH}/build/native/include"
    )
endif()

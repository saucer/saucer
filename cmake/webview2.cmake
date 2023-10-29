include("cmake/nuget.cmake")

if (NOT saucer_prefer_remote)
    find_package(webview2 CONFIG REQUIRED)
    return()
endif()

if (NOT TARGET webview2::webview2)
    nuget_add(WebView2 "Microsoft.Web.WebView2" ${saucer_webview2_version})

    add_library(webview2 STATIC IMPORTED)
    add_library(webview2::webview2 ALIAS webview2)

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
          set(webview2_lib_path "${WebView2_PATH}/build/native/x64/WebView2LoaderStatic.lib")
    else()
          set(webview2_lib_path "${WebView2_PATH}/build/native/x86/WebView2LoaderStatic.lib")
    endif()

    set_target_properties(webview2 PROPERTIES
        IMPORTED_LOCATION "${webview2_lib_path}"
        INTERFACE_INCLUDE_DIRECTORIES "${WebView2_PATH}/build/native/include" 
    )
endif()

cmake_minimum_required(VERSION 3.16)
project(saucer LANGUAGES CXX VERSION 1.0)

# --------------------------------------------------------------------------------------------------------
# Create library
# --------------------------------------------------------------------------------------------------------

add_library(${PROJECT_NAME})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W3 /WX)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Werror -Wextra -pedantic)
endif()

# --------------------------------------------------------------------------------------------------------
# Include "include" folder
# --------------------------------------------------------------------------------------------------------

target_include_directories(${PROJECT_NAME} PUBLIC "include")

# --------------------------------------------------------------------------------------------------------
# Library options
# --------------------------------------------------------------------------------------------------------

option(saucer_examples "Build examples" OFF)

set(saucer_bridge_types None JSON)
set(saucer_bridge JSON CACHE STRING "The bridge to use")
set_property(CACHE saucer_bridge PROPERTY STRINGS ${saucer_bridge_types})

set(saucer_backend_types Default Qt Qt6 WebView2)
set(saucer_backend Default CACHE STRING "The backend to use")
set_property(CACHE saucer_backend PROPERTY STRINGS ${saucer_backend_types})

if (NOT saucer_bridge IN_LIST saucer_bridge_types)
    message(FATAL_ERROR "[Saucer] Bridge must be one of ${saucer_bridge_types}")
else()
    message(STATUS "[Saucer] Using bridge: ${saucer_bridge}")
endif()

if (NOT saucer_backend IN_LIST saucer_backend_types)
    message(FATAL_ERROR "[Saucer] Backend must be one of ${saucer_backend_types}")
else()
    message(STATUS "[Saucer] Using backend: ${saucer_backend}")
    if (saucer_backend STREQUAL "Default")
        if (WIN32)
            set(saucer_backend WebView2)
        else()
            set(saucer_backend Qt)
        endif()
        
        message(STATUS "[Saucer] Deault backend for current platformn is: ${saucer_backend}")
    endif()    
endif()

# --------------------------------------------------------------------------------------------------------
# Configure Library depending on options
# --------------------------------------------------------------------------------------------------------

include(FetchContent)

if (saucer_backend STREQUAL "Qt")
    target_sources(${PROJECT_NAME} PRIVATE "src/webview.qt5.cpp" "src/window.qt5.cpp")

    find_package(Qt5 COMPONENTS Widgets WebEngineWidgets WebChannel REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets Qt5::WebEngineWidgets Qt5::WebChannel)
elseif (saucer_backend STREQUAL "Qt6")
    target_sources(${PROJECT_NAME} PRIVATE "src/webview.qt6.cpp" "src/window.qt6.cpp")

    find_package(Qt6 COMPONENTS Widgets WebEngineWidgets WebChannel REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::WebEngineWidgets Qt6::WebChannel)
elseif (saucer_backend STREQUAL "WebView2")
    target_sources(${PROJECT_NAME} PRIVATE "src/window.win32.cpp" "src/webview.webview2.cpp")

    option(WIL_BUILD_PACKAGING "" OFF)
    option(WIL_BUILD_TESTS  "" OFF)
    FetchContent_Declare(wil GIT_REPOSITORY "https://github.com/microsoft/wil")
    FetchContent_MakeAvailable(wil)

    include("NuGet.cmake")
    nuget_add(WebView2 "Microsoft.Web.WebView2" 1.0.1020.30)
    target_include_directories(${PROJECT_NAME} PRIVATE ${WebView2_PATH}/build/native/include)

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${WebView2_PATH}/build/native/x64/WebView2LoaderStatic.lib)
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE ${WebView2_PATH}/build/native/x86/WebView2LoaderStatic.lib)
    endif()

    target_link_libraries(${PROJECT_NAME} PRIVATE WIL::WIL Dwmapi)
endif()

if (saucer_bridge STREQUAL "JSON")
    target_sources(${PROJECT_NAME} PRIVATE "src/bridge.json.cpp")
    
    FetchContent_Declare(json GIT_REPOSITORY "https://github.com/nlohmann/json/" GIT_TAG v3.10.4 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(json)

    target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json)

    if (UNIX)
        target_link_libraries(${PROJECT_NAME} PUBLIC pthread)
    endif()
endif()

if (saucer_bridge STREQUAL "JSON" OR saucer_backend STREQUAL "WebView2")
    FetchContent_Declare(lockpp GIT_REPOSITORY "https://github.com/Soundux/lockpp" GIT_TAG v1.0.0)
    FetchContent_MakeAvailable(lockpp)

    target_link_libraries(${PROJECT_NAME} PUBLIC lockpp)
endif()

# --------------------------------------------------------------------------------------------------------
# Examples
# --------------------------------------------------------------------------------------------------------

if (saucer_examples)
    if (saucer_bridge STREQUAL "JSON")
        message(STATUS "[Saucer] Building JSON-Bridge Example")
        add_subdirectory("examples/json_bridge")
    endif()
endif()
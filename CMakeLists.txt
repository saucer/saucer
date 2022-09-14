cmake_minimum_required(VERSION 3.16)
project(saucer LANGUAGES CXX VERSION 1.0)

# --------------------------------------------------------------------------------------------------------
# Library options
# --------------------------------------------------------------------------------------------------------

option(saucer_tests             "Build tests" OFF)
option(saucer_examples          "Build examples" OFF)
option(saucer_smartview         "Enable the smartview" ON)
option(saucer_shared            "Build a shared library" OFF)
option(saucer_impl_modules      "Enable implementation modules" OFF)
option(saucer_remote_webview2   "Download NuGet to link webview2" ON)
option(saucer_prefer_remote     "Prefer remote packages over local packages" ON)

set(saucer_backend_types Default Qt Qt6 WebView2)
set(saucer_backend Default CACHE STRING "The backend to use")
set_property(CACHE saucer_backend PROPERTY STRINGS ${saucer_backend_types})
set(saucer_webview2_version "1.0.1245.22" CACHE STRING "The WebView2 version to use")

if (NOT saucer_smartview)
    message(STATUS "[Saucer] Not using smartview, native interoperability will be disabled")
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
        
        message(STATUS "[Saucer] Deault backend for current platform is: ${saucer_backend}")
    endif()    
endif()

if (saucer_backend MATCHES "^Qt6?$")
    message(STATUS "[Saucer] Enabling AUTOMOC for Qt backend")
    set(CMAKE_AUTOMOC ON)
endif()

# --------------------------------------------------------------------------------------------------------
# Create library
# --------------------------------------------------------------------------------------------------------

add_library(${PROJECT_NAME} STATIC)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)

if (saucer_shared)
    message(STATUS "[Saucer] Building shared library")
    
    add_library(${PROJECT_NAME}_shared SHARED)
    add_library(${PROJECT_NAME}::shared ALIAS ${PROJECT_NAME}_shared)

    target_sources(${PROJECT_NAME}_shared PRIVATE "shared/exports.cpp" "shared/serializer.cpp")    
    
    target_compile_features(${PROJECT_NAME}_shared PRIVATE cxx_std_17)
    target_link_libraries(${PROJECT_NAME}_shared PUBLIC ${PROJECT_NAME})
    set_target_properties(${PROJECT_NAME}_shared PROPERTIES CXX_VISIBILITY_PRESET default CXX_STANDARD 17 CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)

    if (WIN32)
        target_compile_definitions(${PROJECT_NAME}_shared PRIVATE _CRT_SECURE_NO_WARNINGS=1)
        set_target_properties(${PROJECT_NAME}_shared PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
    endif()
endif()

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W3 /WX)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Werror -Wextra -pedantic)
endif()

# --------------------------------------------------------------------------------------------------------
# Include "include" folder
# --------------------------------------------------------------------------------------------------------

target_include_directories(${PROJECT_NAME} PUBLIC "include")
target_include_directories(${PROJECT_NAME} PRIVATE "include/saucer")

# --------------------------------------------------------------------------------------------------------
# Generate "constants" file
# --------------------------------------------------------------------------------------------------------

string(TOLOWER ${saucer_backend} saucer_backed_lower)
configure_file("constants.hpp.in" "${CMAKE_CURRENT_SOURCE_DIR}/include/saucer/constants.hpp")

# --------------------------------------------------------------------------------------------------------
# Configure Library depending on options
# --------------------------------------------------------------------------------------------------------

include(FetchContent)

if (saucer_backend STREQUAL "Qt")
    target_sources(${PROJECT_NAME} PRIVATE "src/webview.qt5.cpp" "src/window.qt5.cpp")
    target_sources(${PROJECT_NAME} PRIVATE "src/webview.qt5.impl.hpp" "src/window.qt5.impl.hpp")

    find_package(Qt5 COMPONENTS Widgets WebEngineWidgets WebChannel REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets Qt5::WebEngineWidgets Qt5::WebChannel)
    
    if (UNIX)
        target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
    endif()
elseif (saucer_backend STREQUAL "Qt6")
    target_sources(${PROJECT_NAME} PRIVATE "src/webview.qt6.cpp" "src/window.qt6.cpp")
    target_sources(${PROJECT_NAME} PRIVATE "src/webview.qt6.impl.hpp" "src/window.qt6.impl.hpp")

    find_package(Qt6 COMPONENTS Widgets WebEngineWidgets WebChannel REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::WebEngineWidgets Qt6::WebChannel)
elseif (saucer_backend STREQUAL "WebView2")
    target_sources(${PROJECT_NAME} PRIVATE "src/window.win32.cpp" "src/webview.webview2.cpp")

    if (NOT saucer_prefer_remote)
        find_package(wil CONFIG REQUIRED)
    else()
        option(WIL_BUILD_PACKAGING "" OFF)
        option(WIL_BUILD_TESTS  "" OFF)
    
        FetchContent_Declare(wil GIT_REPOSITORY "https://github.com/microsoft/wil")
        FetchContent_MakeAvailable(wil)
    endif()

    if (NOT saucer_remote_webview2)
        find_package(webview2 CONFIG REQUIRED)
        target_link_libraries(${PROJECT_NAME} PRIVATE webview2::webview2)
    else()
        include("NuGet.cmake")
        nuget_add(WebView2 "Microsoft.Web.WebView2" ${saucer_webview2_version})
        target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE=1 _UNICODE=1)
        target_include_directories(${PROJECT_NAME} PRIVATE ${WebView2_PATH}/build/native/include)

        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            target_link_libraries(${PROJECT_NAME} PRIVATE ${WebView2_PATH}/build/native/x64/WebView2LoaderStatic.lib)
        else()
            target_link_libraries(${PROJECT_NAME} PRIVATE ${WebView2_PATH}/build/native/x86/WebView2LoaderStatic.lib)
        endif()
    endif()

    target_link_libraries(${PROJECT_NAME} PRIVATE WIL::WIL Dwmapi Shlwapi)
endif()

if (saucer_smartview)
    target_sources(${PROJECT_NAME} PRIVATE "src/smartview.cpp" "src/module.cpp")

    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
    set(EXPECTED_ENABLE_TESTS OFF)
    set(CMAKE_CXX_STANDARD 17)

    if (NOT saucer_prefer_remote)
        find_package(fmt CONFIG REQUIRED)
        find_package(tl-expected CONFIG REQUIRED)
        find_package(nlohmann_json CONFIG REQUIRED)
    else()
        option(EXPECTED_BUILD_TESTS OFF)

        FetchContent_Declare(expected GIT_REPOSITORY "https://github.com/TartanLlama/expected" GIT_TAG 96d547c03d2feab8db64c53c3744a9b4a7c8f2c5)
        FetchContent_Declare(json GIT_REPOSITORY "https://github.com/nlohmann/json/" GIT_TAG v3.10.5 GIT_SHALLOW TRUE)
        FetchContent_Declare(fmt GIT_REPOSITORY "https://github.com/fmtlib/fmt" GIT_TAG 8.1.1)
        FetchContent_MakeAvailable(json expected fmt)
    endif()

    target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json tl::expected fmt::fmt)
endif()

if (saucer_impl_modules)
    message(STATUS "[Saucer] Enabling Implementation Module support")

    target_include_directories(${PROJECT_NAME} PUBLIC "modules")

    if (saucer_backend STREQUAL "Qt")
        target_link_libraries(${PROJECT_NAME} PUBLIC Qt5::Widgets Qt5::WebEngineWidgets)
    endif()

    if (saucer_backend STREQUAL "Qt6")
        target_link_libraries(${PROJECT_NAME} PUBLIC Qt6::Widgets Qt6::WebEngineWidgets)
    endif()

    if (saucer_backend STREQUAL "WebView2")
        target_link_libraries(${PROJECT_NAME} PUBLIC WIL::WIL)
        target_include_directories(${PROJECT_NAME} PUBLIC ${WebView2_PATH}/build/native/include)
    endif()
endif()

if (saucer_smartview)
    if (NOT saucer_prefer_remote)
        find_path(LOCKPP_INCLUDE_DIRS "lockpp/lock.hpp")
        target_include_directories(${PROJECT_NAME} PUBLIC ${LOCKPP_INCLUDE_DIRS})
    else()
        FetchContent_Declare(lockpp GIT_REPOSITORY "https://github.com/Soundux/lockpp" GIT_TAG v1.0.2)
        FetchContent_MakeAvailable(lockpp)

        target_link_libraries(${PROJECT_NAME} PUBLIC lockpp::lockpp)
    endif()
endif()

if (NOT saucer_prefer_remote)
    find_path(EREIGNIS_INCLUDE_DIRS "ereignis/event.hpp")
    target_include_directories(${PROJECT_NAME} PUBLIC ${EREIGNIS_INCLUDE_DIRS})
else()
    FetchContent_Declare(ereignis GIT_REPOSITORY "https://github.com/Soundux/ereignis" GIT_TAG v1.1)
    FetchContent_MakeAvailable(ereignis)

    target_link_libraries(${PROJECT_NAME} PUBLIC soundux::ereignis)
endif()

# --------------------------------------------------------------------------------------------------------
# Examples
# --------------------------------------------------------------------------------------------------------

if (saucer_examples)
    if (saucer_smartview)
        message(STATUS "[Saucer] Building JSON-Bridge Example")
        add_subdirectory("examples/json-bridge")
    endif()
        
    add_subdirectory("examples/manual-embedding")
    add_subdirectory("examples/cli-embedding")
endif()

# --------------------------------------------------------------------------------------------------------
# Tests
# --------------------------------------------------------------------------------------------------------

if (saucer_tests)
    message(STATUS "[Saucer] Building Tests")

    if (NOT saucer_prefer_remote)
        find_package(catch2 CONFIG REQUIRED)
    else()
        FetchContent_Declare(Catch2 GIT_REPOSITORY "https://github.com/catchorg/Catch2" GIT_TAG v2.13.7)
        option(CATCH_BUILD_STATIC_LIBRARY "" ON)
        FetchContent_MakeAvailable(Catch2)
    endif()

    file(GLOB test_src "tests/*.cpp")
    add_executable(${PROJECT_NAME}_tests ${test_src})
    target_compile_definitions(${PROJECT_NAME} PUBLIC THROW_ASSERT=1)
    target_link_libraries(${PROJECT_NAME}_tests PRIVATE Catch2::Catch2 Catch2::Catch2WithMain ${PROJECT_NAME})

    target_compile_features(${PROJECT_NAME}_tests PRIVATE cxx_std_17)
    set_target_properties(${PROJECT_NAME}_tests PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
endif()

# --------------------------------------------------------------------------------------------------------
# Install Targets
# --------------------------------------------------------------------------------------------------------

install(DIRECTORY "include/" TYPE INCLUDE)
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib INCLUDES DESTINATION include)
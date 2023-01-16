cmake_minimum_required(VERSION 3.16)
project(saucer LANGUAGES CXX VERSION 1.0)

# --------------------------------------------------------------------------------------------------------
# Check requirements
# --------------------------------------------------------------------------------------------------------

if (MSVC AND MSVC_TOOLSET_VERSION LESS 143)
    message(WARNING "[Saucer] MSVC Toolsets below 14.3 are not supported")
endif()

# --------------------------------------------------------------------------------------------------------
# Library options
# --------------------------------------------------------------------------------------------------------

option(saucer_tests             "Build tests"                                OFF)
option(saucer_assert            "Enable asserts"                              ON)
option(saucer_wrapper           "Build C-Wrapper"                            OFF)
option(saucer_examples          "Build examples"                             OFF)
option(saucer_modules           "Enable modules"                             OFF)
option(saucer_smartview         "Enable the smartview"                        ON)
option(saucer_prefer_remote     "Prefer remote packages over local packages"  ON)
option(saucer_remote_webview2   "Acquire WebView2 through Nughet"             ON)

set(saucer_webview2_version "1.0.1462.37" CACHE STRING "The WebView2 version to use")

set(saucer_backend_types Default Qt5 Qt6 WebView2)
set(saucer_backend Default CACHE STRING "The backend to use")
set_property(CACHE saucer_backend PROPERTY STRINGS ${saucer_backend_types})

# --------------------------------------------------------------------------------------------------------
# Ensure backend is set to allowed value
# --------------------------------------------------------------------------------------------------------

if (NOT saucer_backend IN_LIST saucer_backend_types)
    message(FATAL_ERROR "[Saucer] Backend must be one of ${saucer_backend_types}")
else()
    message(STATUS "[Saucer] Using backend: ${saucer_backend}")

    if (saucer_backend STREQUAL "Default")
        if (WIN32)
            set(saucer_backend WebView2)
        else()
            set(saucer_backend Qt5)
        endif()
        
        message(STATUS "[Saucer] Deault backend for current platform is: ${saucer_backend}")
    endif()    
endif()

# AUTOMOC has to be enabled before `add_library` is called.

if (saucer_backend MATCHES "^Qt.$")
    set(CMAKE_AUTOMOC ON)
    message(STATUS "[Saucer] Using AUTOMOC for ${saucer_backend}")
endif()

# When using saucer in a shared library we need to enable position independent code

if (saucer_wrapper)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# --------------------------------------------------------------------------------------------------------
# Create library
# --------------------------------------------------------------------------------------------------------

add_library(${PROJECT_NAME} STATIC)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W3 /WX)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror -pedantic -pedantic-errors -Wfatal-errors)
endif()

# --------------------------------------------------------------------------------------------------------
# Include directories
# --------------------------------------------------------------------------------------------------------

target_include_directories(${PROJECT_NAME} PUBLIC "include")
target_include_directories(${PROJECT_NAME} PRIVATE "include/saucer" "private")

# --------------------------------------------------------------------------------------------------------
# Generate "constants" file
# --------------------------------------------------------------------------------------------------------

string(TOLOWER ${saucer_backend} saucer_backend_lower)
string(REGEX REPLACE "[a-zA-Z]+" "" saucer_backend_version "${saucer_backend}")
configure_file("constants.hpp.in" "${CMAKE_CURRENT_SOURCE_DIR}/include/saucer/constants.hpp")

# --------------------------------------------------------------------------------------------------------
# Link Dependencies and Add Sources
# --------------------------------------------------------------------------------------------------------

include("Deps.cmake")
include("NuGet.cmake")

message(STATUS "[Saucer] Prefer Remote: ${saucer_prefer_remote}")

set(saucer_conditional_linkage PRIVATE)

if (saucer_modules)
    set(saucer_conditional_linkage PUBLIC)
    message(STATUS "[Saucer] Conditional Linkage is set to PUBLIC")
endif()

if (saucer_backend MATCHES "^Qt.$")
    target_sources(${PROJECT_NAME} PRIVATE "src/window.qt.cpp" "src/webview.qt.cpp")
    target_sources(${PROJECT_NAME} PRIVATE "src/webview.qt${saucer_backend_version}.cpp")
    target_sources(${PROJECT_NAME} PRIVATE "src/window.qt.impl.cpp" "src/webview.qt.impl.cpp")
    target_sources(${PROJECT_NAME} PRIVATE "private/window.qt.impl.hpp" "private/webview.qt.impl.hpp")

    find_package(Qt${saucer_backend_version} COMPONENTS Widgets WebEngineWidgets WebChannel REQUIRED)
    
    target_link_libraries(${PROJECT_NAME} ${saucer_conditional_linkage} 
        Qt${saucer_backend_version}::Widgets 
        Qt${saucer_backend_version}::WebChannel
        Qt${saucer_backend_version}::WebEngineWidgets 
    )
endif()

if (saucer_backend STREQUAL "WebView2")
    target_sources(${PROJECT_NAME} PRIVATE "src/window.win32.cpp" "src/webview.webview2.cpp")
    target_sources(${PROJECT_NAME} PRIVATE "src/window.win32.impl.cpp" "src/webview.webview2.impl.cpp" "src/utils.win32.cpp")

    option(WIL_BUILD_TESTS     "" OFF)
    option(WIL_BUILD_PACKAGING "" OFF)

    add_dependency(wil URL "https://github.com/microsoft/wil" TAG master)
    target_link_libraries(${PROJECT_NAME} ${saucer_conditional_linkage} WIL::WIL Shlwapi)

    if (saucer_remote_webview2)
        nuget_add(WebView2 "Microsoft.Web.WebView2" ${saucer_webview2_version})

        target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE=1 _UNICODE=1)
        target_include_directories(${PROJECT_NAME} ${saucer_conditional_linkage} ${WebView2_PATH}/build/native/include)

        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            target_link_libraries(${PROJECT_NAME} PRIVATE ${WebView2_PATH}/build/native/x64/WebView2LoaderStatic.lib)
        else()
            target_link_libraries(${PROJECT_NAME} PRIVATE ${WebView2_PATH}/build/native/x86/WebView2LoaderStatic.lib)
        endif()
    else()
        find_package(webview2 CONFIG REQUIRED)
        target_link_libraries(${PROJECT_NAME} PRIVATE webview2::webview2)
    endif()
endif()

if (saucer_smartview)
    target_sources(${PROJECT_NAME} PRIVATE "src/smartview.cpp" "src/serializer.json.cpp")

    option(EXPECTED_BUILD_TESTS OFF)

    add_dependency(lockpp                URL "https://github.com/Soundux/lockpp"           TAG   v1.0.2 HEADER "lockpp/lock.hpp" LIB_NAME lockpp::lockpp)
    add_dependency(nlohmann_json         URL "https://github.com/nlohmann/json/"           TAG  v3.11.2)
    add_dependency(boost_callable_traits URL "https://github.com/boostorg/callable_traits" TAG  2a56a3a)
    add_dependency(tl-expected           URL "https://github.com/TartanLlama/expected"     TAG  b74fecd)
    add_dependency(fmt                   URL "https://github.com/fmtlib/fmt"               TAG    9.1.0)
        
    target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json tl::expected fmt::fmt Boost::callable_traits lockpp::lockpp)
endif()

add_dependency(ereignis URL "https://github.com/Soundux/ereignis" TAG v1.1 HEADER "ereignis/event.hpp" LIB_NAME soundux::ereignis)
target_link_libraries(${PROJECT_NAME} PUBLIC soundux::ereignis)

# --------------------------------------------------------------------------------------------------------
# Install Targets
# --------------------------------------------------------------------------------------------------------

install(DIRECTORY "include/" TYPE INCLUDE)
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib INCLUDES DESTINATION include)

# --------------------------------------------------------------------------------------------------------
# Tests and Examples
# --------------------------------------------------------------------------------------------------------

if (saucer_tests)
    message(STATUS "[Saucer] Building Tests")
    add_subdirectory(tests)
endif()

if (saucer_wrapper)
    message(STATUS "[Saucer] Building C-Wrapper")
    add_subdirectory(lib)
endif()

if (saucer_examples)
    message(STATUS "[Saucer] Building Examples")
    
    if (saucer_smartview)
        add_subdirectory("examples/json-bridge")
    endif()
        
    add_subdirectory("examples/manual-embedding")
    add_subdirectory("examples/cli-embedding")
endif()
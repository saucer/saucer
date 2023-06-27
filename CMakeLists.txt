cmake_minimum_required(VERSION 3.16)
project(saucer LANGUAGES CXX VERSION 2.0)

# --------------------------------------------------------------------------------------------------------
# Check requirements
# --------------------------------------------------------------------------------------------------------

if (MSVC AND MSVC_TOOLSET_VERSION LESS 143)
    message(WARNING "[saucer] MSVC Toolsets below 14.3 are not supported")
endif()

# --------------------------------------------------------------------------------------------------------
# Library options
# --------------------------------------------------------------------------------------------------------

option(saucer_modules           "Enable modules"                                   OFF)

option(saucer_package_all       "Add all required dependencies to install target"  OFF)
option(saucer_prefer_remote     "Prefer remote packages over local packages"        ON)

option(saucer_tests             "Build tests"                                      OFF)
option(saucer_examples          "Build examples"                                   OFF)

set(saucer_webview2_version "1.0.1462.37" CACHE STRING "The WebView2 version to use (Ignored when using offline packages)")
set(saucer_backend "Default" CACHE STRING "The backend to use, will use the most appropiate one for the current platform by default")

# --------------------------------------------------------------------------------------------------------
# Set "saucer_prefer_remote" and "CPM_USE_LOCAL_PACKAGES" to equal values
# --------------------------------------------------------------------------------------------------------

if (saucer_prefer_remote)
    message(STATUS "[saucer] Avoiding local packages as 'saucer_prefer_remote' is ON")
    set(CPM_DOWNLOAD_ALL ON)
endif()

# --------------------------------------------------------------------------------------------------------
# Ensure valid library options
# --------------------------------------------------------------------------------------------------------

set(saucer_valid_backends Qt5 Qt6 WebView2 Default)
set_property(CACHE saucer_backend PROPERTY STRINGS ${saucer_valid_backends})

if (NOT saucer_backend IN_LIST saucer_valid_backends)
    message(FATAL_ERROR "Bad Backend, expected one of ${saucer_valid_backends}")
endif()

# --------------------------------------------------------------------------------------------------------
# Determine appropiate default backend
# --------------------------------------------------------------------------------------------------------

if (saucer_backend STREQUAL "Default")
    if (WIN32)
        set(saucer_backend WebView2)
    else()
        set(saucer_backend Qt5)
    endif()

    message(STATUS "[saucer] Backend is 'Defaut', using ${saucer_backend}")
endif()

# --------------------------------------------------------------------------------------------------------
# Setup Library
# --------------------------------------------------------------------------------------------------------

add_library(${PROJECT_NAME} STATIC)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS saucer)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20 CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    # We only enable warnings if built standalone.
    message(STATUS "[saucer] Enabling compilation warnings")

    if (MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /W3 /WX)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror -pedantic -pedantic-errors -Wfatal-errors)
    endif()
endif()

# --------------------------------------------------------------------------------------------------------
# Include directories
# --------------------------------------------------------------------------------------------------------

target_include_directories(${PROJECT_NAME} PUBLIC  "include")
target_include_directories(${PROJECT_NAME} PRIVATE "include/saucer" "private")

# --------------------------------------------------------------------------------------------------------
# Generate "constants" file
# --------------------------------------------------------------------------------------------------------

string(TOLOWER ${saucer_backend} saucer_backend_lower)
string(REGEX REPLACE "[a-zA-Z]+" "" saucer_backend_version "${saucer_backend}")
configure_file("constants.hpp.in" "${CMAKE_CURRENT_SOURCE_DIR}/include/saucer/constants.hpp")

# --------------------------------------------------------------------------------------------------------
# Setup Sources
# --------------------------------------------------------------------------------------------------------

target_sources(${PROJECT_NAME} PRIVATE 
    "src/utils.color.cpp"
    "src/smartview.cpp"
    "src/serializer.json.cpp"
)

# --------------------------------------------------------------------------------------------------------
# Setup Dependencies
# --------------------------------------------------------------------------------------------------------

include("cmake/cpm.cmake")

CPMFindPackage(
  NAME           lockpp
  VERSION        1.0.2
  GIT_REPOSITORY "https://github.com/Soundux/lockpp"
)

CPMFindPackage(
  NAME           nlohmann_json
  VERSION        3.11.2
  GIT_REPOSITORY "https://github.com/nlohmann/json"
)

CPMFindPackage(
  NAME           boost_callable_traits
  GIT_TAG        2a56a3a
  GIT_REPOSITORY "https://github.com/boostorg/callable_traits"
)

CPMFindPackage(
  NAME           tl-expected
  VERSION        1.1.0
  GIT_REPOSITORY "https://github.com/TartanLlama/expected"
  OPTIONS        "EXPECTED_BUILD_TESTS OFF"
)

CPMFindPackage(
  NAME           ereignis
  VERSION        2.0
  GIT_REPOSITORY "https://github.com/Soundux/ereignis"
)

CPMFindPackage(
  NAME           fmt
  GIT_TAG        9.1.0
  GIT_REPOSITORY "https://github.com/fmtlib/fmt"
)

target_link_libraries(${PROJECT_NAME} PUBLIC lockpp nlohmann_json boost_callable_traits tl::expected ereignis fmt)

# --------------------------------------------------------------------------------------------------------
# Setup Backends
# --------------------------------------------------------------------------------------------------------

#! TODO: Conditional Linkage based on saucer_modules
#! TODO: Packaging
#! TODO: Windows

if (saucer_backend MATCHES "^Qt.$")
    set(qt_ver ${saucer_backend_version})

    file(GLOB qt_sources 
      "src/*.qt.*cpp"
      "private/*.qt.*hpp"
      "src/*.qt${qt_ver}.*cpp"
    )

    target_sources(${PROJECT_NAME} PRIVATE ${qt_sources})
    set_target_properties(${PROJECT_NAME} PROPERTIES AUTOMOC ON)

    find_package(Qt${qt_ver} COMPONENTS Widgets WebEngineWidgets WebChannel REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt${qt_ver}::Widgets Qt${qt_ver}::WebChannel Qt${qt_ver}::WebEngineWidgets)
endif()

# --------------------------------------------------------------------------------------------------------
# Setup Tests
# --------------------------------------------------------------------------------------------------------

if (saucer_tests)
    message(STATUS "[saucer] Building Tests")
    add_subdirectory(tests)
endif()

# --------------------------------------------------------------------------------------------------------
# Setup Examples
# --------------------------------------------------------------------------------------------------------

if (saucer_examples)
    message(STATUS "[saucer] Building Examples")
    
    add_subdirectory("examples/manual-embedding")
    add_subdirectory("examples/cli-embedding")
    add_subdirectory("examples/json-bridge")
endif()
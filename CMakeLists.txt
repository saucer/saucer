cmake_minimum_required(VERSION 3.16)
project(saucer LANGUAGES CXX VERSION 2.0)

# --------------------------------------------------------------------------------------------------------
# Check requirements
# --------------------------------------------------------------------------------------------------------

if (MSVC AND MSVC_TOOLSET_VERSION LESS 143)
    message(WARNING "[saucer] MSVC Toolsets below 14.3 are not supported")
endif()

# --------------------------------------------------------------------------------------------------------
# Library switches
# --------------------------------------------------------------------------------------------------------

option(saucer_modules           "Enable modules"                                    ON)

option(saucer_package_all       "Add all required dependencies to install target"  OFF)
option(saucer_prefer_remote     "Prefer remote packages over local packages"        ON)

option(saucer_tests             "Build tests"                                      OFF)
option(saucer_examples          "Build examples"                                   OFF)

# --------------------------------------------------------------------------------------------------------
# Library options
# --------------------------------------------------------------------------------------------------------

set(saucer_webview2_version "1.0.1901.177"  CACHE STRING "The WebView2 version to use (Ignored when using offline packages)")
set(saucer_backend          "Default"       CACHE STRING "The backend to use, will use the most appropiate one for the current platform by default")

# --------------------------------------------------------------------------------------------------------
# Set "saucer_prefer_remote" and "CPM_USE_LOCAL_PACKAGES" to equal values
# --------------------------------------------------------------------------------------------------------

if (saucer_prefer_remote)
    message(STATUS "[saucer] Avoiding local packages as 'saucer_prefer_remote' is ON")
endif()

set(CPM_DOWNLOAD_ALL ${saucer_prefer_remote})

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

# We only enable warnings if built standalone.
if (NOT MSVC AND CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror -pedantic -pedantic-errors -Wfatal-errors)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wno-unknown-warning-option -Wno-missing-field-initializers -Wno-cast-function-type)
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
    "src/smartview.cpp"
    "src/utils.color.cpp"
    "src/serializer.glaze.cpp"
)

# --------------------------------------------------------------------------------------------------------
# Setup Dependencies
# --------------------------------------------------------------------------------------------------------

include("cmake/cpm.cmake")
include("cmake/nuget.cmake")

CPMFindPackage(
  NAME           lockpp
  VERSION        2.2
  GIT_REPOSITORY "https://github.com/Soundux/lockpp"
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
  VERSION        2.1
  GIT_REPOSITORY "https://github.com/Soundux/ereignis"
)

CPMFindPackage(
  NAME           fmt
  GIT_TAG        10.1.0
  GIT_REPOSITORY "https://github.com/fmtlib/fmt"
)

CPMFindPackage(
  NAME           glaze
  VERSION        1.5.2
  GIT_REPOSITORY "https://github.com/stephenberry/glaze"
)

target_link_libraries(${PROJECT_NAME} PUBLIC lockpp boost_callable_traits tl::expected glaze::glaze ereignis fmt)

# --------------------------------------------------------------------------------------------------------
# Setup Linkage
# └ we use conditional linkage, because we want to publicly link against qt/webview2 when module support
#   is enabled, so that consumers of the library can easily use the undlerying API for their modules
#   without having to link qt/webview2 themselves.
# --------------------------------------------------------------------------------------------------------

set(saucer_linkage PRIVATE)

if (saucer_modules)
    set(saucer_linkage PUBLIC)
endif()

# --------------------------------------------------------------------------------------------------------
# Setup Backends
# --------------------------------------------------------------------------------------------------------

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
    target_link_libraries(${PROJECT_NAME} ${saucer_linkage} Qt${qt_ver}::Widgets Qt${qt_ver}::WebChannel Qt${qt_ver}::WebEngineWidgets)
endif()

if (saucer_backend STREQUAL "WebView2")
    file(GLOB win_sources 
      "src/*.win32.*cpp"
      "src/*.webview2.*cpp"
    )

    target_sources(${PROJECT_NAME} PRIVATE ${win_sources})
    target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE=1 _UNICODE=1)

    include("cmake/webview2.cmake")

    CPMFindPackage(
      NAME           wil
      GIT_TAG        master
      GIT_REPOSITORY "https://github.com/microsoft/wil"
      OPTIONS        "WIL_BUILD_TESTS OFF" "WIL_BUILD_PACKAGING OFF"
    )    

    target_link_libraries(${PROJECT_NAME} ${saucer_linkage} WIL::WIL Shlwapi webview2::webview2)
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
        
    add_subdirectory("examples/json-bridge")
    add_subdirectory("examples/embedding")
endif()

# --------------------------------------------------------------------------------------------------------
# Setup Packaging Target
# └ We build this artifact so that people who don't use CMake can manually include all required headers
#   of our dependencies and link against our static-lib.
# --------------------------------------------------------------------------------------------------------

install(TARGETS  ${PROJECT_NAME} 
        EXPORT   ${PROJECT_NAME}-artifact
        INCLUDES DESTINATION include
        LIBRARY  DESTINATION lib
        ARCHIVE  DESTINATION lib
        RUNTIME  DESTINATION bin
)

if (saucer_prefer_remote AND saucer_backend STREQUAL "WebView2")
  install(FILES ${webview2_lib_path} DESTINATION lib)
endif()

install(DIRECTORY "$<TARGET_PROPERTY:fmt,INTERFACE_INCLUDE_DIRECTORIES>/fmt" TYPE INCLUDE)
install(DIRECTORY "$<TARGET_PROPERTY:glaze_glaze,INTERFACE_INCLUDE_DIRECTORIES>/glaze" TYPE INCLUDE)
install(DIRECTORY "$<TARGET_PROPERTY:boost_callable_traits,INTERFACE_INCLUDE_DIRECTORIES>/boost" TYPE INCLUDE)

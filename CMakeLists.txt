cmake_minimum_required(VERSION 3.25)
project(saucer LANGUAGES CXX VERSION 7.0.0)

# --------------------------------------------------------------------------------------------------------
# Library switches
# --------------------------------------------------------------------------------------------------------

option(saucer_static            "Build a static library"                            ON)
option(saucer_modules           "Enable module support"                             ON)
option(saucer_prefer_remote     "Prefer remote packages over local packages"        ON)

option(saucer_examples          "Build examples"                                   OFF)
option(saucer_tests             "Build tests"                                      OFF)

option(saucer_msvc_hack         "Fix mutex crashes on mismatching runtimes"        OFF) # See VS2022 17.10 Changelog
option(saucer_private_webkit    "Enable private api usage for wkwebview"            ON)

option(saucer_no_version_check  "Skip compiler version check"                      OFF)

# --------------------------------------------------------------------------------------------------------
# Check requirements
# └ The main use case of this check is to prevent users from reporting
#   issues only to find out that their compiler is too old.
# --------------------------------------------------------------------------------------------------------

macro(saucer_mark_outdated)
  if (${ARGN})
    set(saucer_cxx_outdated TRUE)
  else()
    set(saucer_cxx_outdated FALSE)
  endif()
endmacro()

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  saucer_mark_outdated(CMAKE_CXX_COMPILER_VERSION LESS 19.43)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  saucer_mark_outdated(CMAKE_CXX_COMPILER_VERSION LESS 14)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
  saucer_mark_outdated(CMAKE_CXX_COMPILER_VERSION LESS 17)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  saucer_mark_outdated(CMAKE_CXX_COMPILER_VERSION LESS 18)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  saucer_mark_outdated(CMAKE_CXX_COMPILER_VERSION LESS 17)
endif()

if (WIN32 AND MINGW)
  message(WARNING "[saucer] WebView2 requires certain headers that are not provided by MinGW!")
  message(WARNING "[saucer] Expect to run into issues...")
endif()

if (NOT saucer_no_sdk_version_check AND WIN32 AND CMAKE_SYSTEM_VERSION LESS 10.0.2263)
    message(FATAL_ERROR "[saucer] The used Windows SDK seems to be outdated! You can disable this check with '-Dsaucer_no_sdk_version_check=ON'")
endif()

if (NOT saucer_no_compiler_version_check AND saucer_cxx_outdated)
    message(FATAL_ERROR "[saucer] The used compiler seems to be outdated! You can disable this check with '-Dsaucer_no_compiler_version_check=ON'")
endif()

# --------------------------------------------------------------------------------------------------------
# Library options
# --------------------------------------------------------------------------------------------------------

set(saucer_webview2_version "1.0.3296.44"   CACHE STRING "The WebView2 version to use (Ignored when using offline packages)")
set(saucer_windows_arch     "Default"       CACHE STRING "The windows architecture (Win32, x64, ARM64) used for linking against NuGet packages")
set(saucer_backend          "Default"       CACHE STRING "The backend to use, will use the most appropriate one for the current platform by default")
set(saucer_serializer       "Glaze"         CACHE STRING "The built-in serializer to use for e.g. request parsing. Also used as the default smartview serializer")

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

set(saucer_valid_backends Qt5 Qt6 WebKitGtk WebView2 WebKit Default)
set_property(CACHE saucer_backend PROPERTY STRINGS ${saucer_valid_backends})

if (NOT saucer_backend IN_LIST saucer_valid_backends)
  message(FATAL_ERROR "Bad Backend, expected one of ${saucer_valid_backends}")
endif()

set(saucer_valid_serializers Glaze Rflpp None)
set_property(CACHE saucer_serializer PROPERTY STRINGS ${saucer_valid_serializers})

if (NOT saucer_serializer IN_LIST saucer_valid_serializers)
  message(FATAL_ERROR "Bad Serializer, expected one of ${saucer_valid_serializers}")
endif()

# --------------------------------------------------------------------------------------------------------
# Determine appropriate default backend
# --------------------------------------------------------------------------------------------------------

if (saucer_backend STREQUAL "Default")
  if (WIN32)
    set(saucer_backend WebView2)
  elseif (APPLE)
    set(saucer_backend WebKit)
  else()
    set(saucer_backend WebKitGtk)
  endif()

  message(STATUS "[saucer] Backend is 'Default', using ${saucer_backend}")
endif()

# --------------------------------------------------------------------------------------------------------
# Setup Library
# --------------------------------------------------------------------------------------------------------

if (saucer_static)
  add_library(${PROJECT_NAME} STATIC)
else()
  add_library(${PROJECT_NAME} SHARED)
endif()

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 23 CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)

if (PROJECT_IS_TOP_LEVEL AND NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT MATCHES "MSVC")
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror -pedantic -pedantic-errors -Wfatal-errors)
endif()

if (NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT MATCHES "MSVC")
  target_compile_options(${PROJECT_NAME} PRIVATE -Wno-unknown-warning-option -Wno-missing-field-initializers -Wno-cast-function-type)
endif()

# We are not using `CMAKE_CXX_COMPILER_FRONTEND_VARIANT` here because ClangCL behaves kind of weird
# and expects flags to disable warnings in GNU fashion regardless of whether it is in MSVC- or GNU-CLI mode

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  target_compile_options(${PROJECT_NAME} PUBLIC -Wno-attributes=sc::)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  target_compile_options(${PROJECT_NAME} PUBLIC /wd5030 /Zc:preprocessor /utf-8)
else()
  target_compile_options(${PROJECT_NAME} PUBLIC -Wno-unknown-attributes)
endif()

# --------------------------------------------------------------------------------------------------------
# Setup Compile Definitions
# --------------------------------------------------------------------------------------------------------

if (saucer_msvc_hack)
  target_compile_definitions(${PROJECT_NAME} PUBLIC _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR)
endif()

if (saucer_private_webkit)
  target_compile_definitions(${PROJECT_NAME} PUBLIC SAUCER_WEBKIT_PRIVATE)
endif()

# --------------------------------------------------------------------------------------------------------
# Setup Linkage
# └ To make module development easier, all system libraries will be linked publicly if module
#   support is enabled.
# --------------------------------------------------------------------------------------------------------

set(saucer_linkage PRIVATE)

if (saucer_modules)
  set(saucer_linkage PUBLIC)
endif()

# --------------------------------------------------------------------------------------------------------
# Include directories
# --------------------------------------------------------------------------------------------------------

target_include_directories(${PROJECT_NAME} PRIVATE "include/saucer")
target_include_directories(${PROJECT_NAME} PRIVATE "private/saucer")

target_include_directories(
  ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                         $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

# --------------------------------------------------------------------------------------------------------
# Miscellaneous CMake-Setup
# --------------------------------------------------------------------------------------------------------

if (saucer_backend STREQUAL "Qt5")
  set(QT_VERSION 5)
  set(QT_REQUIRED_VERSION 5.0.0)
endif()

if (saucer_backend STREQUAL "Qt6")
  set(QT_VERSION 6)
  set(QT_REQUIRED_VERSION 6.7.0)
endif()

if (saucer_serializer STREQUAL "Glaze")
  set(SERIALIZER_DEP "glaze 5.6.0")
endif()

if (saucer_serializer STREQUAL "Rflpp")
  set(SERIALIZER_DEP "reflectcpp 0.20.0")
endif()

# --------------------------------------------------------------------------------------------------------
# Setup Sources
# --------------------------------------------------------------------------------------------------------

target_sources(${PROJECT_NAME} PRIVATE
    "src/error.cpp"
    "src/error.impl.cpp"

    "src/request.cpp"
    "src/module/unstable.cpp"
    
    "src/app.cpp"
    "src/window.cpp"
    "src/webview.cpp"
    "src/smartview.cpp"
    
    "src/webview.impl.cpp"
)

# --------------------------------------------------------------------------------------------------------
# Setup Dependencies
# --------------------------------------------------------------------------------------------------------

include("cmake/cpm.cmake")
include("cmake/nuget.cmake")
include("cmake/module.cmake")

CPMFindPackage(
  NAME           lockpp
  VERSION        3.2.0
  GIT_REPOSITORY "https://github.com/Curve/lockpp"
)

CPMFindPackage(
  NAME           coco
  VERSION        3.2.1
  GIT_REPOSITORY "https://github.com/Curve/coco"
)

CPMFindPackage(
  NAME           rebind
  VERSION        5.3.1
  GIT_REPOSITORY "https://github.com/Curve/rebind"
)

CPMFindPackage(
  NAME           ereignis
  VERSION        6.2.0
  GIT_REPOSITORY "https://github.com/Curve/ereignis"
)

CPMFindPackage(
  NAME           flagpp
  VERSION        3.1.0
  GIT_REPOSITORY "https://github.com/Curve/flagpp"
)

CPMFindPackage(
  NAME           polo 
  VERSION        1.0.1
  GIT_REPOSITORY "https://github.com/Curve/polo"
)

CPMFindPackage(
  NAME           fill
  VERSION        1.2.2
  GIT_REPOSITORY "https://github.com/saucer/fill"
  OPTIONS        "fill_namespace std" "fill_silent ON"
)

target_link_libraries(${PROJECT_NAME} ${saucer_linkage} cr::lockpp cr::flagpp)
target_link_libraries(${PROJECT_NAME} PUBLIC            cr::coco cr::rebind cr::ereignis cr::polo saucer::fill)

# --------------------------------------------------------------------------------------------------------
# Setup Backends
# --------------------------------------------------------------------------------------------------------

if (saucer_backend MATCHES "^Qt.$")
  file(GLOB_RECURSE qt_sources 
    "src/qt.*cpp"
    "private/saucer/qt.*hpp"
    "src/qt${QT_VERSION}.*cpp"
  )

  target_sources(${PROJECT_NAME} PRIVATE ${qt_sources})
  target_compile_definitions(${PROJECT_NAME} PUBLIC SAUCER_QT${QT_VERSION})

  set_target_properties(${PROJECT_NAME} PROPERTIES AUTOMOC ON)

  find_package(Qt${QT_VERSION} ${QT_REQUIRED_VERSION} COMPONENTS Widgets WebEngineWidgets WebChannel REQUIRED)
  target_link_libraries(${PROJECT_NAME} ${saucer_linkage} Qt${QT_VERSION}::Widgets Qt${QT_VERSION}::WebChannel Qt${QT_VERSION}::WebEngineWidgets)
endif()

if (saucer_backend STREQUAL "WebKit")
  enable_language(OBJCXX)

  file(GLOB_RECURSE cocoa_sources 
    "src/cocoa.*mm"
    "src/wk.*mm"
  )

  target_sources(${PROJECT_NAME} PRIVATE ${cocoa_sources})
  target_compile_definitions(${PROJECT_NAME} PUBLIC SAUCER_WEBKIT)

  find_library(cocoa  Cocoa     REQUIRED)
  find_library(webkit WebKit    REQUIRED)
  find_library(cimage CoreImage REQUIRED)

  target_link_libraries(${PROJECT_NAME} ${saucer_linkage} ${cocoa} ${webkit} ${cimage})
endif()

if (saucer_backend STREQUAL "WebKitGtk")
  file(GLOB_RECURSE gtk_sources 
    "src/gtk.*cpp"
    "src/wkg.*cpp"
  )

  target_sources(${PROJECT_NAME} PRIVATE ${gtk_sources})
  target_compile_definitions(${PROJECT_NAME} PUBLIC SAUCER_WEBKITGTK)

  find_package(PkgConfig REQUIRED)

  pkg_check_modules(gtk        REQUIRED gtk4>=4.12)
  pkg_check_modules(libadwaita REQUIRED IMPORTED_TARGET libadwaita-1)
  pkg_check_modules(json-glib  REQUIRED IMPORTED_TARGET json-glib-1.0)
  pkg_check_modules(webkitgtk  REQUIRED IMPORTED_TARGET webkitgtk-6.0)

  target_link_libraries(${PROJECT_NAME} ${saucer_linkage} PkgConfig::libadwaita PkgConfig::webkitgtk PkgConfig::json-glib)
endif()

if (saucer_backend STREQUAL "WebView2")
  file(GLOB_RECURSE win_sources 
    "src/win32.*cpp"
    "src/wv2.*cpp"
  )

  target_sources(${PROJECT_NAME} PRIVATE ${win_sources})
  target_compile_definitions(${PROJECT_NAME} PUBLIC SAUCER_WEBVIEW2)
  
  target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX)
  target_compile_definitions(${PROJECT_NAME} PUBLIC  UNICODE _UNICODE)

  include("cmake/webview2.cmake")
  target_link_libraries(${PROJECT_NAME} ${saucer_linkage} CoreMessaging RuntimeObject Wininet Shlwapi gdiplus webview2::webview2)
endif()

# --------------------------------------------------------------------------------------------------------
# Setup Serializers
# --------------------------------------------------------------------------------------------------------

if (saucer_serializer STREQUAL "Glaze")
  CPMFindPackage(
    NAME           glaze
    VERSION        5.6.0
    GIT_REPOSITORY "https://github.com/stephenberry/glaze"
  )

  file(GLOB_RECURSE glaze_sources 
    "src/glaze.*cpp"
  )

  target_sources(${PROJECT_NAME} PRIVATE ${glaze_sources})
  target_link_libraries(${PROJECT_NAME} PUBLIC glaze::glaze)
endif()

if (saucer_serializer STREQUAL "Rflpp")
  CPMFindPackage(
    NAME           reflectcpp
    VERSION        0.20.0
    GIT_REPOSITORY "https://github.com/getml/reflect-cpp"
    SYSTEM         ON
  )

  file(GLOB_RECURSE rflpp_sources 
    "src/rfl.*cpp"
  )

  target_sources(${PROJECT_NAME} PRIVATE ${rflpp_sources})
  target_link_libraries(${PROJECT_NAME} PUBLIC reflectcpp)
endif()

# --------------------------------------------------------------------------------------------------------
# Configure Config
# --------------------------------------------------------------------------------------------------------

function(configure_config)
  string(TOLOWER ${saucer_serializer} SERIALIZER)

  if (NOT saucer_serializer STREQUAL "None")
    set(INCLUDE_SERIALIZER "#include \"serializers/${SERIALIZER}/${SERIALIZER}.hpp\"")
    set(DEFAULT_SERIALIZER "serializers::${SERIALIZER}::serializer")
  else()
    set(DEFAULT_SERIALIZER "void")
  endif()

  configure_file("config/config.hpp.in" "${CMAKE_CURRENT_SOURCE_DIR}/include/saucer/config.hpp")
endfunction()

configure_config()

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
  add_subdirectory("examples/embedding")
  add_subdirectory("examples/desktop")
  add_subdirectory("examples/expose")
  add_subdirectory("examples/loop")
  add_subdirectory("examples/pdf")
endif()

# --------------------------------------------------------------------------------------------------------
# Package Config
# --------------------------------------------------------------------------------------------------------

set(saucer_blacklist "reflectcpp" "nontype_functional" "jthread")

foreach(target IN LISTS saucer_blacklist)
  if (TARGET ${target})
    message(WARNING "[saucer] Package-Config not supported with current configuration (${target})")
    return()
  endif()
endforeach()

CPMFindPackage(
  NAME           PackageProject
  VERSION        1.13.0
  GIT_REPOSITORY "https://github.com/TheLartians/PackageProject.cmake"
)

packageProject(
  NAMESPACE saucer
  NAME ${PROJECT_NAME}
  VERSION ${PROJECT_VERSION}
  BINARY_DIR ${PROJECT_BINARY_DIR}
  INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
  INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
  DEPENDENCIES "lockpp 3.2.0;coco 3.2.1;rebind 5.3.1;ereignis 6.2.0;flagpp 3.1.0;polo 1.0.0;saucer-fill 1.2.0;${SERIALIZER_DEP}"
)

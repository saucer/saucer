cmake_minimum_required(VERSION 3.25)
project(saucer LANGUAGES CXX VERSION 6.0.1)

# --------------------------------------------------------------------------------------------------------
# Library switches
# --------------------------------------------------------------------------------------------------------

option(saucer_static            "Build a static library"                            ON)
option(saucer_modules           "Enable module support"                             ON)

option(saucer_package_all       "Add all required dependencies to install target"  OFF)
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
  saucer_mark_outdated(CMAKE_CXX_COMPILER_VERSION LESS 19.38)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  saucer_mark_outdated(CMAKE_CXX_COMPILER_VERSION LESS 14)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
  saucer_mark_outdated(CMAKE_CXX_COMPILER_VERSION LESS 17)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  saucer_mark_outdated(CMAKE_CXX_COMPILER_VERSION LESS 18)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  saucer_mark_outdated(CMAKE_CXX_COMPILER_VERSION LESS 15)
endif()

if (WIN32 AND MINGW)
  message(WARNING "[saucer] WebView2 requires certain headers that are not provided by MinGW!")
  message(WARNING "[saucer] Expect to run into issues...")
endif()

if (NOT saucer_no_version_check AND saucer_cxx_outdated)
  message(FATAL_ERROR "[saucer] The used compiler seems to be outdated! You can disable this check with '-Dsaucer_no_version_check=ON'")
endif()

# --------------------------------------------------------------------------------------------------------
# Library options
# --------------------------------------------------------------------------------------------------------

set(saucer_webview2_version "1.0.2903.40"   CACHE STRING "The WebView2 version to use (Ignored when using offline packages)")
set(saucer_windows_arch     "Default"       CACHE STRING "The windows architecture (Win32, x64, ARM64) used for linking against NuGet packages")
set(saucer_backend          "Default"       CACHE STRING "The backend to use, will use the most appropiate one for the current platform by default")
set(saucer_serializer       "Glaze"         CACHE STRING "The built-in serializer to use for e.g. request parsing. Also used as the default smartview serializer")
set(saucer_polyfills        "Default"       CACHE STRING "The polyfills to use. Set to 'None' to avoid any polyfills from being used. Uses 'try_compile' by default")

# --------------------------------------------------------------------------------------------------------
# Set "saucer_prefer_remote" and "CPM_USE_LOCAL_PACKAGES" to equal values
# --------------------------------------------------------------------------------------------------------

if (saucer_package_all AND NOT saucer_prefer_remote)
  message(WARNING "[saucer] Cannot use 'saucer_package_all' without 'saucer_prefer_remote'")
  set(saucer_prefer_remote ON)
endif()

if (saucer_prefer_remote)
  message(STATUS "[saucer] Avoiding local packages as 'saucer_prefer_remote' is ON")
endif()

set(CPM_DOWNLOAD_ALL ${saucer_prefer_remote})

# --------------------------------------------------------------------------------------------------------
# Global Configuration
# --------------------------------------------------------------------------------------------------------

if (saucer_msvc_hack)
  add_compile_definitions(_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR)
endif()

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
# Determine appropiate default backend
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
  target_compile_options(${PROJECT_NAME} PUBLIC /wd5030)
else()
  target_compile_options(${PROJECT_NAME} PUBLIC -Wno-unknown-attributes)
endif()

# --------------------------------------------------------------------------------------------------------
# Setup Linkage
# └ To make module development eaiser, all system libraries will be linked publicly if module
#   support is enabled. 
# --------------------------------------------------------------------------------------------------------

set(saucer_linkage PRIVATE)

if (saucer_modules)
  set(saucer_linkage PUBLIC)
endif()

# --------------------------------------------------------------------------------------------------------
# Include directories
# --------------------------------------------------------------------------------------------------------

target_include_directories(${PROJECT_NAME} PUBLIC  "include")
target_include_directories(${PROJECT_NAME} PRIVATE "private")

target_include_directories(${PROJECT_NAME} PRIVATE "private/saucer")
target_include_directories(${PROJECT_NAME} PRIVATE "include/saucer")

# --------------------------------------------------------------------------------------------------------
# Setup feature macros
# --------------------------------------------------------------------------------------------------------

if (saucer_tests)
  target_compile_definitions(${PROJECT_NAME} PUBLIC SAUCER_TESTS)
endif()

if (saucer_private_webkit)
  target_compile_definitions(${PROJECT_NAME} PUBLIC SAUCER_WEBKIT_PRIVATE)
endif()

if (saucer_backend STREQUAL "Qt5")
  set(QT_VERSION 5)
  set(QT_REQUIRED_VERSION 5.0.0)
  target_compile_definitions(${PROJECT_NAME} PUBLIC SAUCER_QT5)
endif()

if (saucer_backend STREQUAL "Qt6")
  set(QT_VERSION 6)
  set(QT_REQUIRED_VERSION 6.7.0)
  target_compile_definitions(${PROJECT_NAME} PUBLIC SAUCER_QT6)
endif()

if (saucer_backend STREQUAL "WebKitGtk")
  target_compile_definitions(${PROJECT_NAME} PUBLIC SAUCER_WEBKITGTK)
endif()

if (saucer_backend STREQUAL "WebView2")
  target_compile_definitions(${PROJECT_NAME} PUBLIC SAUCER_WEBVIEW2)
endif()

if (saucer_backend STREQUAL "WebKit")
  target_compile_definitions(${PROJECT_NAME} PUBLIC SAUCER_WEBKIT)
endif()

# --------------------------------------------------------------------------------------------------------
# Setup Sources
# --------------------------------------------------------------------------------------------------------

target_sources(${PROJECT_NAME} PRIVATE 
    "src/request.cpp"
    "src/module/unstable.cpp"
    
    "src/app.cpp"
    "src/window.cpp"
    "src/webview.cpp"
    "src/smartview.cpp"
)

# --------------------------------------------------------------------------------------------------------
# Setup Polyfills
# └ Apple Clang (as well as libc++) is lacking behind on some features which this library and
#   some upstream dependencies rely upon. To avoid all sorts of upstream changes, we use
#   a few (very ugly) hacks to make saucer work on macOS. These can hopefully be removed once
#   Apple Clang moves to a more recent libc++. 
# --------------------------------------------------------------------------------------------------------

include("cmake/cpm.cmake")
include("cmake/polyfill.cmake")

if (saucer_polyfills STREQUAL "Default")
  saucer_determine_polyfills(saucer_polyfills)
  message(STATUS "[saucer] Recommended polyfills: ${saucer_polyfills}")
endif()

if ("jthread" IN_LIST saucer_polyfills)
  message(STATUS "[saucer] Using std::jthread polyfill")

  CPMFindPackage(
    NAME           jthread
    VERSION        24.00.1
    GIT_REPOSITORY "https://github.com/saucer/jthread"
  )

  target_link_libraries(${PROJECT_NAME} PUBLIC nonstd::jthread)
  target_precompile_headers(${PROJECT_NAME} PUBLIC "polyfill/jthread.hpp")
endif()

if ("functional" IN_LIST saucer_polyfills)
  message(STATUS "[saucer] Using std::move_only_function polyfill")

  CPMFindPackage(
    NAME           fu2
    VERSION        4.2.3
    GIT_REPOSITORY "https://github.com/saucer/function2"
  )

  target_link_libraries(${PROJECT_NAME} PUBLIC function2::function2)
  target_precompile_headers(${PROJECT_NAME} PUBLIC "polyfill/functional.hpp")
endif()

if ("expected" IN_LIST saucer_polyfills)
  message(STATUS "[saucer] Using std::expected polyfill")

  CPMFindPackage(
    NAME             tl-expected
    VERSION          1.1.0
    GIT_REPOSITORY   "https://github.com/TartanLlama/expected"
    OPTIONS          "EXPECTED_BUILD_TESTS OFF"
    EXCLUDE_FROM_ALL ON
  )

  target_link_libraries(${PROJECT_NAME} PUBLIC tl::expected)
  target_precompile_headers(${PROJECT_NAME} PUBLIC "polyfill/expected.hpp")
endif()

# --------------------------------------------------------------------------------------------------------
# Setup Dependencies
# --------------------------------------------------------------------------------------------------------

include("cmake/nuget.cmake")
include("cmake/module.cmake")

CPMFindPackage(
  NAME           lockpp
  VERSION        3.0
  GIT_REPOSITORY "https://github.com/Curve/lockpp"
)

CPMFindPackage(
  NAME           boost_callable_traits
  GIT_TAG        boost-1.87.0
  GIT_REPOSITORY "https://github.com/boostorg/callable_traits"
)

CPMFindPackage(
  NAME           ereignis
  VERSION        4.3
  GIT_REPOSITORY "https://github.com/Soundux/ereignis"
)

CPMFindPackage(
  NAME           fmt
  GIT_TAG        11.1.3
  GIT_REPOSITORY "https://github.com/fmtlib/fmt"
)

CPMFindPackage(
  NAME           flagpp
  VERSION        3.0
  GIT_REPOSITORY "https://github.com/Curve/flagpp"
)

CPMFindPackage(
  NAME           rebind
  VERSION        5.2.1
  GIT_REPOSITORY "https://github.com/Curve/rebind"
)

CPMFindPackage(
  NAME           poolparty
  VERSION        3.0.1
  GIT_REPOSITORY "https://github.com/Curve/poolparty"
)

CPMFindPackage(
  NAME           eraser
  VERSION        2.2.1
  GIT_REPOSITORY "https://github.com/Curve/eraser"
)

CPMFindPackage(
  NAME           boost_preprocessor
  GIT_TAG        boost-1.87.0
  GIT_REPOSITORY "https://github.com/boostorg/preprocessor"
)

target_link_libraries(${PROJECT_NAME} ${saucer_linkage} boost_preprocessor cr::lockpp cr::flagpp)
target_link_libraries(${PROJECT_NAME} PUBLIC            boost_callable_traits cr::ereignis fmt::fmt cr::rebind cr::poolparty cr::eraser)

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

  find_library(cocoa Cocoa REQUIRED)
  find_library(webkit WebKit REQUIRED)
  find_library(cimage CoreImage REQUIRED)

  target_link_libraries(${PROJECT_NAME} ${saucer_linkage} ${cocoa} ${webkit} ${cimage})
endif()

if (saucer_backend STREQUAL "WebKitGtk")
  file(GLOB_RECURSE gtk_sources 
    "src/gtk.*cpp"
    "src/wkg.*cpp"
  )

  target_sources(${PROJECT_NAME} PRIVATE ${gtk_sources})

  find_package(PkgConfig REQUIRED)

  pkg_check_modules(gtk        REQUIRED gtk4>=4.10)
  pkg_check_modules(libadwaita REQUIRED IMPORTED_TARGET libadwaita-1)
  pkg_check_modules(webkitgtk  REQUIRED IMPORTED_TARGET webkitgtk-6.0)

  target_link_libraries(${PROJECT_NAME} ${saucer_linkage} PkgConfig::libadwaita PkgConfig::webkitgtk)
endif()

if (saucer_backend STREQUAL "WebView2")
  file(GLOB_RECURSE win_sources 
    "src/win32.*cpp"
    "src/wv2.*cpp"
  )

  target_sources(${PROJECT_NAME} PRIVATE ${win_sources})
  target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE=1 _UNICODE=1 NOMINMAX=1)

  include("cmake/webview2.cmake")
  target_link_libraries(${PROJECT_NAME} ${saucer_linkage} Shlwapi gdiplus webview2::webview2)
endif()

# --------------------------------------------------------------------------------------------------------
# Setup Serializers
# --------------------------------------------------------------------------------------------------------

if (saucer_serializer STREQUAL "Glaze")
  file(GLOB_RECURSE glaze_sources 
    "src/glaze.*cpp"
  )

  target_sources(${PROJECT_NAME} PRIVATE ${glaze_sources})

  CPMFindPackage(
    NAME           glaze
    VERSION        4.4.2
    GIT_REPOSITORY "https://github.com/stephenberry/glaze"
  )

  target_link_libraries(${PROJECT_NAME} PUBLIC glaze::glaze)
endif()

if (saucer_serializer STREQUAL "Rflpp")
  file(GLOB_RECURSE rflpp_sources 
    "src/rfl.*cpp"
  )

  target_sources(${PROJECT_NAME} PRIVATE ${rflpp_sources})

  CPMFindPackage(
    NAME           reflectcpp
    VERSION        0.17.0
    GIT_REPOSITORY "https://github.com/getml/reflect-cpp"
    SYSTEM         ON
  )

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
  add_subdirectory("examples/json-bridge")
  add_subdirectory("examples/embedding")
  add_subdirectory("examples/desktop")
  add_subdirectory("examples/pdf")
endif()

# --------------------------------------------------------------------------------------------------------
# Setup Packaging Target
# └ We build this artifact so that people who don't use CMake can manually include all required headers
#   of our dependencies and link against our static-lib.
# --------------------------------------------------------------------------------------------------------

install(TARGETS ${PROJECT_NAME} 
        EXPORT  ${PROJECT_NAME}-artifact
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
)

install(DIRECTORY "include/saucer" TYPE INCLUDE)

if (NOT saucer_package_all)
    return()
endif()

if (saucer_backend STREQUAL "WebView2")
  install(FILES ${webview2_lib_path} DESTINATION lib)
endif()

if (saucer_polyfill_expected)
    install(DIRECTORY "$<TARGET_PROPERTY:expected,INTERFACE_INCLUDE_DIRECTORIES>/tl" TYPE INCLUDE)
endif()

install(DIRECTORY "$<TARGET_PROPERTY:boost_callable_traits,INTERFACE_INCLUDE_DIRECTORIES>/boost" TYPE INCLUDE)

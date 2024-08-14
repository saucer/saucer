cmake_minimum_required(VERSION 3.21)
project(saucer LANGUAGES CXX VERSION 3.0.0)

# --------------------------------------------------------------------------------------------------------
# Check requirements
# --------------------------------------------------------------------------------------------------------

if (MSVC AND MSVC_TOOLSET_VERSION LESS 143)
  message(WARNING "[saucer] MSVC Toolsets below 14.3 are not supported")
endif()

# --------------------------------------------------------------------------------------------------------
# Library switches
# --------------------------------------------------------------------------------------------------------

option(saucer_modules           "Enable smartview modules"                          ON)
option(saucer_polyfill          "Enable polyfills. Mainly used for macOS"          OFF)

option(saucer_package_all       "Add all required dependencies to install target"  OFF)
option(saucer_prefer_remote     "Prefer remote packages over local packages"        ON)

option(saucer_examples          "Build examples"                                   OFF)
option(saucer_msvc_hack         "Fix mutex crashes on mismatching runtimes"        OFF) # See VS2022 17.10 Changelog

option(saucer_tests             "Build tests"                                      OFF)
option(saucer_ci                "Set when running CI-Tests"                        OFF)

# --------------------------------------------------------------------------------------------------------
# Library options
# --------------------------------------------------------------------------------------------------------

set(saucer_webview2_version "1.0.2592.51"   CACHE STRING "The WebView2 version to use (Ignored when using offline packages)")
set(saucer_backend          "Default"       CACHE STRING "The backend to use, will use the most appropiate one for the current platform by default")

# --------------------------------------------------------------------------------------------------------
# Set "saucer_prefer_remote" and "CPM_USE_LOCAL_PACKAGES" to equal values
# --------------------------------------------------------------------------------------------------------

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

# --------------------------------------------------------------------------------------------------------
# Determine appropiate default backend
# --------------------------------------------------------------------------------------------------------

if (saucer_backend STREQUAL "Default")
  if (WIN32)
    set(saucer_backend WebView2)
  elseif(APPLE)
    set(saucer_backend WebKit)
  else()
    set(saucer_backend WebKitGtk)
  endif()

  message(STATUS "[saucer] Backend is 'Defaut', using ${saucer_backend}")
endif()

# --------------------------------------------------------------------------------------------------------
# Setup Library
# --------------------------------------------------------------------------------------------------------

add_library(${PROJECT_NAME} STATIC)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS saucer)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 23 CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
  
if (PROJECT_IS_TOP_LEVEL)
  if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX /external:anglebrackets /external:W0)
  else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wno-unknown-warning-option -Wno-missing-field-initializers -Wno-cast-function-type)
  endif()
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  target_compile_options(${PROJECT_NAME} PUBLIC -Wno-attributes=sc::)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  target_compile_options(${PROJECT_NAME} PUBLIC /wd5030)
else()
  target_compile_options(${PROJECT_NAME} PUBLIC -Wno-unknown-attributes)
endif()

# --------------------------------------------------------------------------------------------------------
# Include directories
# --------------------------------------------------------------------------------------------------------

target_include_directories(${PROJECT_NAME} PUBLIC  "include")
target_include_directories(${PROJECT_NAME} PRIVATE "include/saucer" "private")

# --------------------------------------------------------------------------------------------------------
# Setup feature macros
# --------------------------------------------------------------------------------------------------------

if (saucer_tests)
  if (saucer_ci)
    target_compile_definitions(${PROJECT_NAME} PUBLIC SAUCER_CI)
  endif()
  
  target_compile_definitions(${PROJECT_NAME} PUBLIC SAUCER_TESTS)
endif()

if (saucer_backend MATCHES "^Qt.$")
  string(REGEX REPLACE "[a-zA-Z]+" "" QT_VERSION "${saucer_backend}")
  target_compile_definitions(${PROJECT_NAME} PUBLIC SAUCER_QT${QT_VERSION})
endif()

if (saucer_backend STREQUAL "WebKitGtk")
  target_compile_definitions(${PROJECT_NAME} PUBLIC SAUCER_WEBKITGTK)
endif()

if (saucer_backend STREQUAL "WebView2")
  target_compile_definitions(${PROJECT_NAME} PUBLIC SAUCER_WEBVIEW2)
endif()

if (saucer_backend STREQUAL "WebKit")
  set(saucer_polyfill ON)
  target_compile_definitions(${PROJECT_NAME} PUBLIC SAUCER_WEBKIT)
endif()

# --------------------------------------------------------------------------------------------------------
# Setup Sources
# --------------------------------------------------------------------------------------------------------

target_sources(${PROJECT_NAME} PRIVATE 
    "src/webview.cpp"
    "src/smartview.cpp"
    "src/serializer.glaze.cpp"
    
    "src/latch.cpp"
    "src/requests.cpp"
)

if (saucer_modules)
  target_sources(${PROJECT_NAME} PRIVATE "src/module.cpp")
endif()

# --------------------------------------------------------------------------------------------------------
# Setup Polyfills
# └ Apple Clang (as well as libc++) is lacking behind on some features which this library and
#   some upstream dependencies rely upon. To avoid all sorts of upstream changes, we use
#   a few (very ugly) hacks to make saucer work on macOS. These can hopefully be removed once
#   Apple Clang moves to a more recent libc++. 
# --------------------------------------------------------------------------------------------------------

include("cmake/cpm.cmake")

if (saucer_polyfill)
  set(saucer_fmt_install OFF)

  CPMFindPackage(
    NAME           fu2
    VERSION        4.2.1
    GIT_REPOSITORY "https://github.com/saucer/function2"
  )

  CPMFindPackage(
    NAME           jthread
    GIT_TAG        V23.03.0
    GIT_REPOSITORY "https://github.com/StirlingLabs/jthread"
    OPTIONS        "ENABLE_JTHREAD_TESTS OFF"
  )

  link_libraries(function2::function2 jthread)

  add_definitions(-include ${jthread_SOURCE_DIR}/include/jthread)
  add_definitions(-include ${CMAKE_CURRENT_SOURCE_DIR}/polyfill/functional.hpp)
else()
  set(saucer_fmt_install ON)
endif()

# --------------------------------------------------------------------------------------------------------
# Setup Dependencies
# --------------------------------------------------------------------------------------------------------

include("cmake/nuget.cmake")

CPMFindPackage(
  NAME           lockpp
  VERSION        3.0
  GIT_REPOSITORY "https://github.com/Curve/lockpp"
)

CPMFindPackage(
  NAME           boost_callable_traits
  GIT_TAG        boost-1.85.0
  GIT_REPOSITORY "https://github.com/boostorg/callable_traits"
)

CPMFindPackage(
  NAME             tl-expected
  VERSION          1.1.0
  GIT_REPOSITORY   "https://github.com/TartanLlama/expected"
  OPTIONS          "EXPECTED_BUILD_TESTS OFF"
  EXCLUDE_FROM_ALL ON
)

CPMFindPackage(
  NAME           ereignis
  VERSION        4.1
  GIT_REPOSITORY "https://github.com/Soundux/ereignis"
)

CPMFindPackage(
  NAME           fmt
  GIT_TAG        11.0.2
  GIT_REPOSITORY "https://github.com/fmtlib/fmt"
  OPTIONS        "FMT_INSTALL ${saucer_fmt_install}"
)

CPMFindPackage(
  NAME           glaze
  VERSION        3.2.2
  GIT_REPOSITORY "https://github.com/stephenberry/glaze"
)

CPMFindPackage(
  NAME           flagpp
  VERSION        3.0
  GIT_REPOSITORY "https://github.com/Curve/flagpp"
)

CPMFindPackage(
  NAME           rebind
  VERSION        2.0.0
  GIT_REPOSITORY "https://github.com/Curve/rebind"
)

CPMFindPackage(
  NAME           poolparty
  VERSION        3.0.0
  GIT_REPOSITORY "https://github.com/Curve/poolparty"
)

CPMFindPackage(
  NAME           boost_preprocessor
  GIT_TAG        boost-1.85.0
  GIT_REPOSITORY "https://github.com/boostorg/preprocessor"
)

target_link_libraries(${PROJECT_NAME} PRIVATE boost_preprocessor cr::flagpp)
target_link_libraries(${PROJECT_NAME} PUBLIC  boost_callable_traits tl::expected glaze::glaze cr::ereignis fmt::fmt cr::rebind cr::lockpp cr::poolparty)

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
  file(GLOB qt_sources 
    "src/qt.*cpp"
    "private/qt.*hpp"
    "src/qt${QT_VERSION}.*cpp"
  )

  target_sources(${PROJECT_NAME} PRIVATE ${qt_sources})
  set_target_properties(${PROJECT_NAME} PROPERTIES AUTOMOC ON)

  find_package(Qt${QT_VERSION} COMPONENTS Widgets WebEngineWidgets WebChannel REQUIRED)
  target_link_libraries(${PROJECT_NAME} ${saucer_linkage} Qt${QT_VERSION}::Widgets Qt${QT_VERSION}::WebChannel Qt${QT_VERSION}::WebEngineWidgets)
endif()

if (saucer_backend STREQUAL "WebKit")
  enable_language(OBJCXX)

  file(GLOB cocoa_sources 
    "src/cocoa.*mm"
    "src/wk.*mm"
  )

  target_sources(${PROJECT_NAME} PRIVATE ${cocoa_sources})

  find_library(cocoa Cocoa REQUIRED)
  find_library(webkit WebKit REQUIRED)
  find_library(cimage CoreImage REQUIRED)

  target_compile_options(${PROJECT_NAME} PRIVATE "-fobjc-arc")
  target_link_libraries(${PROJECT_NAME} ${saucer_linkage} ${cocoa} ${webkit} ${cimage})
endif()

if (saucer_backend STREQUAL "WebKitGtk")
  file(GLOB gtk_sources 
    "src/gtk.*cpp"
    "src/wkg.*cpp"
  )

  target_sources(${PROJECT_NAME} PRIVATE ${gtk_sources})

  find_package(PkgConfig REQUIRED)

  pkg_check_modules(libadwaita REQUIRED IMPORTED_TARGET libadwaita-1)
  pkg_check_modules(webkitgtk  REQUIRED IMPORTED_TARGET webkitgtk-6.0)

  target_link_libraries(${PROJECT_NAME} ${saucer_linkage} PkgConfig::libadwaita PkgConfig::webkitgtk)
endif()

if (saucer_backend STREQUAL "WebView2")
  file(GLOB win_sources 
    "src/win32.*cpp"
    "src/wv2.*cpp"
  )

  target_sources(${PROJECT_NAME} PRIVATE ${win_sources})
  target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE=1 _UNICODE=1 NOMINMAX=1)

  include("cmake/webview2.cmake")
  target_link_libraries(${PROJECT_NAME} ${saucer_linkage} Shlwapi gdiplus webview2::webview2)
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

install(TARGETS ${PROJECT_NAME} 
        EXPORT  ${PROJECT_NAME}-artifact
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
)

if (saucer_prefer_remote AND saucer_backend STREQUAL "WebView2")
  install(FILES ${webview2_lib_path} DESTINATION lib)
endif()

install(DIRECTORY "include/saucer" TYPE INCLUDE)
install(DIRECTORY "$<TARGET_PROPERTY:fmt,INTERFACE_INCLUDE_DIRECTORIES>/fmt" TYPE INCLUDE)
install(DIRECTORY "$<TARGET_PROPERTY:glaze_glaze,INTERFACE_INCLUDE_DIRECTORIES>/glaze" TYPE INCLUDE)
install(DIRECTORY "$<TARGET_PROPERTY:boost_callable_traits,INTERFACE_INCLUDE_DIRECTORIES>/boost" TYPE INCLUDE)

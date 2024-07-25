cmake_minimum_required(VERSION 3.16)
project(saucer-tests LANGUAGES CXX)

# --------------------------------------------------------------------------------------------------------
# Create executable
# --------------------------------------------------------------------------------------------------------

add_executable(${PROJECT_NAME})
add_executable(saucer::tests ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20 CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)

if (NOT MSVC AND PROJECT_IS_TOP_LEVEL)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror -pedantic -pedantic-errors -Wfatal-errors)
endif()

# --------------------------------------------------------------------------------------------------------
# Add Sources
# --------------------------------------------------------------------------------------------------------

file(GLOB src "*.cpp")
target_sources(${PROJECT_NAME} PRIVATE ${src})

# --------------------------------------------------------------------------------------------------------
# Link Dependencies 
# --------------------------------------------------------------------------------------------------------

include("../cmake/cpm.cmake")

CPMFindPackage(
  NAME           ut
  VERSION        2.1.0
  GIT_REPOSITORY "https://github.com/boost-ext/ut"
  OPTIONS        "BOOST_UT_DISABLE_MODULE ON"
)

target_link_libraries(${PROJECT_NAME} PRIVATE Boost::ut saucer::saucer)

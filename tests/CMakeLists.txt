cmake_minimum_required(VERSION 3.16)
project(saucer-tests  LANGUAGES CXX)

# --------------------------------------------------------------------------------------------------------
# Create executable
# --------------------------------------------------------------------------------------------------------

add_executable(${PROJECT_NAME})
add_executable(saucer::tests ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)

# --------------------------------------------------------------------------------------------------------
# Link Dependencies and Add Sources
# --------------------------------------------------------------------------------------------------------

file(GLOB src "*.cpp")
target_sources(${PROJECT_NAME} PRIVATE ${src})

if (saucer_prefer_remote)
    option(CATCH_BUILD_STATIC_LIBRARY "" ON)
    
    FetchContent_Declare(catch2 GIT_REPOSITORY "https://github.com/catchorg/Catch2" GIT_TAG v2.13.7 OVERRIDE_FIND_PACKAGE)
    FetchContent_MakeAvailable(catch2)
endif()


find_package(catch2 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Catch2::Catch2 Catch2::Catch2WithMain saucer::saucer)
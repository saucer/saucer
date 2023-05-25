cmake_minimum_required(VERSION 3.16)
project(saucer-wrapper LANGUAGES CXX)

# --------------------------------------------------------------------------------------------------------
# Create library
# --------------------------------------------------------------------------------------------------------

add_library(${PROJECT_NAME} SHARED)
add_library(saucer::wrapper ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_VISIBILITY_PRESET default CXX_STANDARD 20 CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)

if (WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS=1)
    set_target_properties(${PROJECT_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# --------------------------------------------------------------------------------------------------------
# Link Dependencies and Add Sources
# --------------------------------------------------------------------------------------------------------

file(GLOB src "*.cpp")
target_sources(${PROJECT_NAME} PRIVATE ${src})
target_link_libraries(${PROJECT_NAME} PRIVATE saucer::saucer)
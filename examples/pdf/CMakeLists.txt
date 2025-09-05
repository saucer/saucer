cmake_minimum_required(VERSION 3.25)
project(pdf_example LANGUAGES CXX VERSION 1.0)

# --------------------------------------------------------------------------------------------------------
# Create executable
# --------------------------------------------------------------------------------------------------------

add_executable(${PROJECT_NAME} "main.cpp")
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 23 CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)

# --------------------------------------------------------------------------------------------------------
# Link libraries
# --------------------------------------------------------------------------------------------------------

CPMFindPackage(
  NAME           saucer-pdf
  VERSION        3.0.3
  GIT_REPOSITORY "https://github.com/saucer/pdf"
)

target_link_libraries(${PROJECT_NAME} PRIVATE saucer saucer::pdf)

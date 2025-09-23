set(_SAUCER_CURRENT_DIR "${CMAKE_CURRENT_LIST_DIR}")

cmake_path(GET _SAUCER_CURRENT_DIR PARENT_PATH _SAUCER_TEMPLATE_DIR)
cmake_path(APPEND _SAUCER_TEMPLATE_DIR "template")

function(embed_read FILE OUTPUT)
    file(READ "${FILE}" FILE_HEX HEX)
    file(SIZE "${FILE}" FILE_SIZE)

    string(REGEX MATCHALL "([A-Za-z0-9][A-Za-z0-9])" FILE_SPLIT "${FILE_HEX}")
    list(JOIN FILE_SPLIT ", 0x" FILE_CONTENT)
    string(PREPEND FILE_CONTENT "0x")

    set(${OUTPUT}_SIZE    "${FILE_SIZE}"    PARENT_SCOPE)
    set(${OUTPUT}_CONTENT "${FILE_CONTENT}" PARENT_SCOPE)
endfunction()

function(embed_generate OUTPUT)
    cmake_parse_arguments(PARSE_ARGV 1 generate "" "FILE;ROOT;SIZE;CONTENT;IDENTIFIER;DESTINATION" "")

    set(RELATIVE "${generate_FILE}")
    cmake_path(RELATIVE_PATH RELATIVE BASE_DIRECTORY "${generate_ROOT}")

    set(HEADER "${generate_DESTINATION}")
    cmake_path(APPEND HEADER "${RELATIVE}.hpp")

    set(SOURCE "${generate_DESTINATION}")
    cmake_path(APPEND SOURCE "${RELATIVE}.cpp")

    embed_read("${generate_FILE}" FILE)

    set(SIZE       "${FILE_SIZE}")
    set(CONTENT    "${FILE_CONTENT}")
    set(IDENTIFIER "${generate_IDENTIFIER}")

    cmake_path(GET HEADER FILENAME INCLUDE)

    configure_file("${_SAUCER_TEMPLATE_DIR}/embed.file.cpp.in" "${SOURCE}")
    configure_file("${_SAUCER_TEMPLATE_DIR}/embed.file.hpp.in" "${HEADER}")

    set(${OUTPUT}_HEADER "${HEADER}"    PARENT_SCOPE)
    set(${OUTPUT}_PATH   "/${RELATIVE}" PARENT_SCOPE)
endfunction()

function(embed_mime FILE OUTPUT)
    find_package(Python COMPONENTS Interpreter REQUIRED)

    execute_process(
        COMMAND         "${Python_EXECUTABLE}" -c "from mimetypes import guess_type; print(guess_type('${FILE}')[0] or 'text/plain')"
        OUTPUT_VARIABLE MIME_TYPE
        COMMAND_ERROR_IS_FATAL ANY
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    set(${OUTPUT} "${MIME_TYPE}" PARENT_SCOPE)
endfunction()

function(embed_target NAME DIRECTORY)
    set(TARGET "saucer_${NAME}")
    set(ALIAS  "saucer::${NAME}")

    set(GLOB "${DIRECTORY}")
    cmake_path(APPEND GLOB "*.cpp")
    file(GLOB_RECURSE SOURCES ${GLOB})

    add_library(${TARGET} STATIC)
    add_library(${ALIAS} ALIAS ${TARGET})

    target_sources(${TARGET} PRIVATE ${SOURCES})
    target_include_directories(${TARGET} PUBLIC "${DIRECTORY}")

    target_compile_features(${TARGET} PUBLIC cxx_std_23)
    set_target_properties(${TARGET} PROPERTIES CXX_STANDARD 23 CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
endfunction()

function(saucer_embed DIRECTORY)
    cmake_parse_arguments(PARSE_ARGV 1 embed "" "NAME;DESTINATION" "")

    if (NOT embed_DESTINATION)
        set(embed_DESTINATION "embedded")
    endif()

    if (NOT embed_NAME)
        set(embed_NAME "embedded")
    endif()

    cmake_path(ABSOLUTE_PATH DIRECTORY)

    set(GLOB "${DIRECTORY}")
    cmake_path(APPEND GLOB "*")
    file(GLOB_RECURSE FILES ${GLOB})

    set(output_ROOT "${embed_DESTINATION}")
    cmake_path(ABSOLUTE_PATH output_ROOT)

    set(output_HEADERS "${output_ROOT}")
    cmake_path(APPEND output_HEADERS "saucer" "embedded")

    set(output_FILES "${output_HEADERS}")
    cmake_path(APPEND output_FILES "files")

    set(generated_INCLUDES "")
    set(generated_EMBEDDED "")

    foreach(path IN LISTS FILES)
        saucer_message(STATUS "Embedding ${path}")

        cmake_path(HASH path embedded_HASH)
        string(MAKE_C_IDENTIFIER "${embedded_HASH}" embedded_NAME)

        embed_generate(embedded
            FILE        "${path}"
            ROOT        "${DIRECTORY}"
            IDENTIFIER  "${embedded_NAME}"
            DESTINATION "${output_FILES}"
        )

        cmake_path(RELATIVE_PATH embedded_HEADER BASE_DIRECTORY "${output_HEADERS}")
        list(APPEND generated_INCLUDES "#include \"${embedded_HEADER}\"")

        embed_mime("${path}" embedded_MIME)
        list(APPEND generated_EMBEDDED "{\"${embedded_PATH}\", saucer::embedded_file{.content = saucer::stash<>::view(${embedded_NAME}), .mime = \"${embedded_MIME}\"}}")
    endforeach()

    set(meta_FILE "${output_HEADERS}")
    cmake_path(APPEND meta_FILE "all.hpp")

    list(JOIN generated_INCLUDES "\n"        INCLUDES)
    list(JOIN generated_EMBEDDED ",\n\t\t\t" FILES)

    configure_file("${_SAUCER_TEMPLATE_DIR}/embed.hpp.in" "${meta_FILE}")

    if (CMAKE_SCRIPT_MODE_FILE)
        return()
    endif()

    embed_target(${embed_NAME} "${output_ROOT}")
endfunction()

if (NOT CMAKE_SCRIPT_MODE_FILE)
    return()
endif()

if (CMAKE_ARGC LESS 4)
    saucer_message(FATAL_ERROR "Usage: embed <directory> [destination]")
endif()

saucer_embed("${CMAKE_ARGV3}" "${CMAKE_ARGV4}")

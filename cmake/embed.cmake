function(saucer_read FILE OUTPUT)
    file(READ "${FILE}" FILE_HEX HEX)
    file(SIZE "${FILE}" FILE_SIZE)

    string(REGEX MATCHALL "([A-Za-z0-9][A-Za-z0-9])" FILE_SPLIT "${FILE_HEX}")
    list(JOIN FILE_SPLIT ", 0x" FILE_CONTENT)
    string(PREPEND FILE_CONTENT "0x")

    set(${OUTPUT}_SIZE    "${FILE_SIZE}"    PARENT_SCOPE)
    set(${OUTPUT}_CONTENT "${FILE_CONTENT}" PARENT_SCOPE)
endfunction()

function(saucer_generate OUTPUT)
    cmake_parse_arguments(PARSE_ARGV 1 generate "" "FILE;ROOT;SIZE;CONTENT;IDENTIFIER;DESTINATION;TEMPLATES" "")

    set(generate_RELATIVE "${generate_FILE}")
    cmake_path(RELATIVE_PATH generate_RELATIVE BASE_DIRECTORY "${generate_ROOT}")

    set(generate_HEADER "${generate_DESTINATION}")
    cmake_path(APPEND generate_HEADER "${generate_RELATIVE}.hpp")

    set(generate_SOURCE "${generate_DESTINATION}")
    cmake_path(APPEND generate_SOURCE "${generate_RELATIVE}.cpp")

    saucer_read("${generate_FILE}" FILE)

    set(SIZE       "${FILE_SIZE}")
    set(CONTENT    "${FILE_CONTENT}")
    set(IDENTIFIER "${generate_IDENTIFIER}")

    cmake_path(GET generate_HEADER FILENAME INCLUDE)

    configure_file("${generate_TEMPLATES}/embed.file.cpp.in" "${generate_SOURCE}")
    configure_file("${generate_TEMPLATES}/embed.file.hpp.in" "${generate_HEADER}")

    set(${OUTPUT}_HEADER "${generate_HEADER}"    PARENT_SCOPE)
    set(${OUTPUT}_PATH   "/${generate_RELATIVE}" PARENT_SCOPE)
endfunction()

function(saucer_mime FILE OUTPUT)
    find_package(Python COMPONENTS Interpreter REQUIRED)

    execute_process(
        COMMAND         "${Python_EXECUTABLE}" -c "from mimetypes import guess_type; print(guess_type('${FILE}')[0] or 'text/plain')"
        OUTPUT_VARIABLE FILE_MIME
        RESULT_VARIABLE EXIT_CODE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if (NOT EXIT_CODE EQUAL 0)
        message(FATAL_ERROR "[embed] Could not get mime-type for '${FILE}': ${FILE_MIME} (${EXIT_CODE})")
    endif()

    set(${OUTPUT} "${FILE_MIME}" PARENT_SCOPE)
endfunction()

function(saucer_embed_target NAME DIRECTORY)
    set(target_NAME  "saucer_${NAME}")
    set(target_ALIAS "saucer::${NAME}")

    set(target_GLOBDIR "${DIRECTORY}")
    cmake_path(APPEND target_GLOBDIR "*.cpp")
    file(GLOB_RECURSE target_SOURCES ${target_GLOBDIR})

    add_library(${target_NAME} STATIC)
    add_library(${target_ALIAS} ALIAS ${target_NAME})

    target_sources(${target_NAME} PRIVATE ${target_SOURCES})
    target_include_directories(${target_NAME} PUBLIC "${DIRECTORY}")

    target_compile_features(${target_NAME} PUBLIC cxx_std_23)
    set_target_properties(${target_NAME} PROPERTIES CXX_STANDARD 23 CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
endfunction()

function(saucer_embed DIRECTORY)
    cmake_path(ABSOLUTE_PATH DIRECTORY)
    cmake_parse_arguments(PARSE_ARGV 1 embed "" "NAME;DESTINATION" "")

    if (NOT embed_DESTINATION)
        set(embed_DESTINATION "embedded")
    endif()

    if (NOT embed_NAME)
        set(embed_NAME "embedded")
    endif()

    cmake_path(GET CMAKE_CURRENT_FUNCTION_LIST_DIR PARENT_PATH embed_TEMPLATES)
    cmake_path(APPEND embed_TEMPLATES "template")

    set(embed_GLOBDIR "${DIRECTORY}")
    cmake_path(APPEND embed_GLOBDIR "*")
    file(GLOB_RECURSE embed_FILES ${embed_GLOBDIR})

    set(DESTINATION "${embed_DESTINATION}")
    cmake_path(ABSOLUTE_PATH DESTINATION)

    set(HEADER_DESTINATION "${DESTINATION}")
    cmake_path(APPEND HEADER_DESTINATION "saucer" "embedded")

    set(FILE_DESTINATION "${HEADER_DESTINATION}")
    cmake_path(APPEND FILE_DESTINATION "files")

    set(embed_INCLUDES "")
    set(embed_EMBEDDED "")

    foreach(path IN LISTS embed_FILES)
        message(STATUS "[embed] Embedding ${path}")

        cmake_path(HASH path FILE_HASH)
        string(MAKE_C_IDENTIFIER "${FILE_HASH}" IDENTIFIER)

        saucer_generate(FILE
            FILE        "${path}"
            ROOT        "${DIRECTORY}"
            IDENTIFIER  "${IDENTIFIER}"
            TEMPLATES   "${embed_TEMPLATES}"
            DESTINATION "${FILE_DESTINATION}"
        )

        cmake_path(RELATIVE_PATH FILE_HEADER BASE_DIRECTORY "${HEADER_DESTINATION}")
        list(APPEND embed_INCLUDES "#include \"${FILE_HEADER}\"")

        saucer_mime("${path}" FILE_MIME)
        list(APPEND embed_EMBEDDED "{\"${FILE_PATH}\", saucer::embedded_file{.content = saucer::stash<>::view(${IDENTIFIER}), .mime = \"${FILE_MIME}\"}}")
    endforeach()

    set(embed_FILE "${HEADER_DESTINATION}")
    cmake_path(APPEND embed_FILE "all.hpp")

    list(JOIN embed_INCLUDES "\n"  INCLUDES)
    list(JOIN embed_EMBEDDED ",\n" FILES)

    configure_file("${embed_TEMPLATES}/embed.hpp.in" "${embed_FILE}")

    if (CMAKE_SCRIPT_MODE_FILE)
        return()
    endif()

    saucer_embed_target(${embed_NAME} "${DESTINATION}")
endfunction()

if (NOT CMAKE_SCRIPT_MODE_FILE)
    return()
endif()

if (CMAKE_ARGC LESS 4)
    message(FATAL_ERROR "[embed] Usage: embed <directory> [destination]")
endif()

saucer_embed("${CMAKE_ARGV3}" "${CMAKE_ARGV4}")

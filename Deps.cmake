include(FetchContent)

# "add_dependency" is a utility function to ease dependency management.
# Since we provide a vcpkg port, we should give the user an option to
# prefer local packages over packages acquired through FetchContent.
# Since "OVERRIDE_FIND_PACKAGE" causes issues with some CMake versions
# we use this function instead. 

function(add_dependency)
    set(argn_list "${ARGN}")
    list(POP_FRONT argn_list NAME)

    set(one_value NAME URL TAG HEADER LIB_NAME)
    cmake_parse_arguments("DEP" "" "${one_value}" "" ${ARGN})

    if (NOT DEFINED DEP_URL OR NOT DEFINED DEP_TAG
        OR DEP_URL IN_LIST DEP_KEYWORDS_MISSING_VALUES
        OR DEP_TAG IN_LIST DEP_KEYWORDS_MISSING_VALUES
    )
        message(WARNING "[Saucer] add_dependency called without url or tag")
    endif()

    if (saucer_prefer_remote)
        FetchContent_Declare(${NAME} GIT_REPOSITORY "${DEP_URL}" GIT_TAG "${DEP_TAG}" GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(${NAME})
    else()
        if (DEP_HEADER)
            find_path(INCLUDE_DIRS ${DEP_HEADER})
            if (DEP_LIB_NAME)
                add_library(${NAME} INTERFACE)
                add_library(${DEP_LIB_NAME} ALIAS ${NAME})
                target_include_directories(${NAME} INTERFACE ${INCLUDE_DIRS})
            else()
                add_library(${NAME} INTERFACE)
                target_include_directories(${NAME} INTERFACE ${INCLUDE_DIRS})
            endif()
        else()
            find_package(${NAME} CONFIG REQUIRED)
        endif()
    endif()
endfunction()
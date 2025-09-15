# --------------------------------------------------------------------------------------------------------
# Setup Meta-Library
# └ Modules can then link against this target to get access to all internal dependencies.
# --------------------------------------------------------------------------------------------------------

add_library(saucer_private  INTERFACE)
add_library(saucer::private ALIAS saucer_private)

macro(saucer_link_libraries NAME)
    target_link_libraries(${NAME} PRIVATE ${ARGN})
    target_link_libraries(saucer_private INTERFACE ${ARGN})
endmacro()

macro(saucer_include_directories NAME)
    target_include_directories(${NAME} PRIVATE ${ARGN})
    target_include_directories(saucer_private INTERFACE ${ARGN})
endmacro()

set(_saucer_backend ${saucer_backend} CACHE INTERNAL "")

# --------------------------------------------------------------------------------------------------------
# Setup Modules
# └ Defines functions that are meant to be used from modules.
# --------------------------------------------------------------------------------------------------------

function(saucer_add_module NAME)
    message(STATUS "[saucer] Adding module: ${NAME}")

    target_link_libraries(${NAME} PUBLIC  saucer::saucer)
    target_link_libraries(${NAME} PRIVATE saucer::private)

    set(saucer_backend ${_saucer_backend} PARENT_SCOPE)
endfunction()

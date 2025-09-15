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

# --------------------------------------------------------------------------------------------------------
# Setup Modules
# └ Defines functions that are meant to be used from modules.
# --------------------------------------------------------------------------------------------------------

macro(saucer_add_module NAME)
    message(STATUS "[saucer] Adding module: ${NAME}")

    target_link_libraries(${NAME}              PUBLIC  saucer::saucer)
    target_link_libraries(${NAME}              PRIVATE saucer::private)
    target_include_directories(${PROJECT_NAME} PRIVATE "${_saucer_root}/private")

    set(saucer_backend ${_saucer_backend})
endmacro()

set(_saucer_backend ${saucer_backend} CACHE INTERNAL "")
set(_saucer_root ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "")

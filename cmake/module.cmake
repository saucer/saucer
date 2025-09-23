# --------------------------------------------------------------------------------------------------------
# Setup Meta-Library
# └ Modules can then link against this target to get access to all internal dependencies.
# --------------------------------------------------------------------------------------------------------

add_library(saucer_private  INTERFACE)
add_library(saucer::private ALIAS saucer_private)

macro(saucer_link_libraries TARGET)
    target_link_libraries(${TARGET} PRIVATE ${ARGN})
    target_link_libraries(saucer_private INTERFACE ${ARGN})
endmacro()

macro(saucer_include_directories TARGET)
    target_include_directories(${TARGET} PRIVATE ${ARGN})
    target_include_directories(saucer_private INTERFACE ${ARGN})
endmacro()

set(_saucer_backend ${saucer_backend} CACHE INTERNAL "")

# --------------------------------------------------------------------------------------------------------
# Setup Modules
# └ Defines functions that are meant to be used from modules.
# --------------------------------------------------------------------------------------------------------

function(saucer_add_module TARGET)
    saucer_message(STATUS "Adding module: ${TARGET}")

    target_link_libraries(${TARGET} PUBLIC  saucer::saucer)
    target_link_libraries(${TARGET} PRIVATE saucer::private)

    set(saucer_backend ${_saucer_backend} PARENT_SCOPE)
endfunction()

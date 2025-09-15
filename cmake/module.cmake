macro(saucer_link_libraries NAME)
    list(APPEND saucer_link_targets ${ARGN})
    target_link_libraries(${NAME} PRIVATE ${ARGN})
    set(_saucer_link_targets ${saucer_link_targets} CACHE INTERNAL "")
endmacro()

macro(saucer_add_module NAME)
    message(STATUS "[saucer] Adding module: ${NAME}")

    target_link_libraries(${NAME}              PUBLIC  saucer::saucer)
    target_link_libraries(${NAME}              PRIVATE ${_saucer_link_targets})
    target_include_directories(${PROJECT_NAME} PRIVATE "${_saucer_root}/private")

    set(saucer_backend ${_saucer_backend})
endmacro()

set(_saucer_backend ${saucer_backend} CACHE INTERNAL "")
set(_saucer_root ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "")

function(saucer_add_module NAME)
    message(STATUS "[saucer] Adding module: ${NAME}")
    
    target_link_libraries(${NAME} PUBLIC saucer::saucer)
    target_include_directories(${PROJECT_NAME} PRIVATE "${_saucer_root}/private")

    set(saucer_backend ${_saucer_backend} PARENT_SCOPE)
endfunction()

set(_saucer_backend ${saucer_backend} CACHE INTERNAL "")
set(_saucer_root ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "")

function(saucer_add_module NAME)
    message(STATUS "[saucer] Adding module: ${NAME}")
    
    get_target_property(priv_includes saucer::saucer INCLUDE_DIRECTORIES)

    target_link_libraries(${NAME} PUBLIC saucer::saucer)
    target_include_directories(${NAME} PRIVATE ${priv_includes})

    set(saucer_backend ${_saucer_backend} PARENT_SCOPE)
endfunction()

set(_saucer_backend ${saucer_backend} CACHE INTERNAL "")

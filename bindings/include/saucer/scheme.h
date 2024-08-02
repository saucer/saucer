#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "stash.h"
#include "window.h"

    enum SAUCER_REQUEST_ERROR
    {
        SAUCER_REQUEST_ERROR_FAILED,
        SAUCER_REQUEST_ERROR_DENIED,
        SAUCER_REQUEST_ERROR_ABORTED,
        SAUCER_REQUEST_ERROR_BAD_URL,
        SAUCER_REQUEST_ERROR_NOT_FOUND,
    };

    struct saucer_response;

    saucer_response *saucer_response_new(saucer_stash *data, const char *mime);
    saucer_response *saucer_response_unexpected(SAUCER_REQUEST_ERROR error);

    /**
     * @note Under normal circumstances this function should not be used. Once a saucer_response is
     * returned from within a saucer_scheme_handler it is automatically deleted.
     * You may use this function to free the response in case of an exception.
     */
    void saucer_response_free(saucer_response *);

    void saucer_response_add_header(saucer_response *, const char *header, const char *value);

    struct saucer_request;

    /*[[sc::requires_free]]*/ char *saucer_request_url(saucer_request *);
    /*[[sc::requires_free]]*/ char *saucer_request_method(saucer_request *);

    /*[[sc::requires_free]]*/ saucer_stash *saucer_request_content(saucer_request *);

    /**
     * @note The arrays pointed to by @param headers and @param values will be populated with strings which are
     * themselves dynamically allocated. Both arrays will then hold @param count elements.
     *
     * To properly free the returned arrays you should:
     * - Free all strings within the headers and values array
     * - Free the array itself
     */
    void saucer_request_headers(saucer_request *, char ***headers, char ***values, size_t *count);

    typedef saucer_response *(*saucer_scheme_handler)(saucer_handle *, saucer_request *);

#ifdef __cplusplus
}
#endif

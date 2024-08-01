#include "scheme.h"
#include "scheme.hpp"

#include "memory.h"
#include "stash.hpp"

#include <ranges>
#include <cstring>

extern "C"
{
    saucer_response *saucer_response_new(saucer_stash *data, const char *mime)
    {
        return saucer_response::from({{saucer::response{data->value(), mime}}});
    }

    saucer_response *saucer_response_unexpected(SAUCER_REQUEST_ERROR error)
    {
        return saucer_response::from(tl::unexpected<saucer::request_error>{static_cast<saucer::request_error>(error)});
    }

    void saucer_response_add_header(saucer_response *handle, const char *header, const char *value)
    {
        handle->value()->headers.emplace(header, value);
    }

    char *saucer_request_url(saucer_request *handle)
    {
        auto url = handle->value()->url();

        auto *rtn = static_cast<char *>(saucer_memory_alloc(url.capacity()));
        strncpy(rtn, url.data(), url.capacity());

        return rtn;
    }

    char *saucer_request_method(saucer_request *handle)
    {
        auto method = handle->value()->method();

        auto *rtn = static_cast<char *>(saucer_memory_alloc(method.capacity()));
        strncpy(rtn, method.data(), method.capacity());

        return rtn;
    }

    saucer_stash *saucer_request_content(saucer_request *handle)
    {
        return saucer_stash::from(handle->value()->content());
    }

    void saucer_request_headers(saucer_request *handle, char ***headers, char ***values, size_t *count)
    {
        auto data = handle->value()->headers();
        *count    = data.size();

        *headers = static_cast<char **>(saucer_memory_alloc(*count * sizeof(char *)));
        *values  = static_cast<char **>(saucer_memory_alloc(*count * sizeof(char *)));

        for (const auto &[index, element] : std::views::enumerate(data))
        {
            const auto &[header, value] = element;

            (*headers)[index] = static_cast<char *>(saucer_memory_alloc(header.capacity()));
            (*values)[index]  = static_cast<char *>(saucer_memory_alloc(value.capacity()));

            strncpy((*headers)[index], header.data(), header.capacity());
            strncpy((*values)[index], value.data(), value.capacity());
        }
    }
}

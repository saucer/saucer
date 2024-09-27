#include "wkg.scheme.impl.hpp"

#include "gtk.utils.hpp"

#include <rebind/enum.hpp>

namespace saucer::scheme
{
    void scheme_handler::add_handler(WebKitWebView *id, handler handler)
    {
        m_handlers.emplace(id, std::move(handler));
    }

    void scheme_handler::remove_handler(WebKitWebView *id)
    {
        m_handlers.erase(id);
    }

    void scheme_handler::handle(WebKitURISchemeRequest *request, scheme_handler *state)
    {
        auto req               = scheme::request{{request}};
        auto *const identifier = webkit_uri_scheme_request_get_web_view(request);

        if (!state->m_handlers.contains(identifier))
        {
            return;
        }

        auto result = std::invoke(state->m_handlers[identifier], req);

        if (!result.has_value())
        {
            static auto quark = webkit_network_error_quark();

            auto error = result.error();
            auto name  = rebind::find_enum_name(error).value_or("unknown");

            auto *const err = g_error_new(quark, std::to_underlying(result.error()), "%s", std::string{name}.c_str());
            webkit_uri_scheme_request_finish_error(request, err);
            g_error_free(err);

            return;
        }

        const auto data = result->data;
        const auto size = static_cast<gssize>(data.size());

        const auto bytes  = g_bytes_ptr{g_bytes_new(data.data(), size)};
        const auto stream = g_object_ptr<GInputStream>{g_memory_input_stream_new_from_bytes(bytes.get())};

        const auto response = g_object_ptr<WebKitURISchemeResponse>{webkit_uri_scheme_response_new(stream.get(), size)};
        auto *const headers = soup_message_headers_new(SOUP_MESSAGE_HEADERS_RESPONSE);

        for (const auto &[name, value] : result->headers)
        {
            soup_message_headers_append(headers, name.c_str(), value.c_str());
        }

        webkit_uri_scheme_response_set_content_type(response.get(), result->mime.c_str());
        webkit_uri_scheme_response_set_status(response.get(), result->status, "");
        webkit_uri_scheme_response_set_http_headers(response.get(), headers);

        webkit_uri_scheme_request_finish_with_response(request, response.get());
    }
} // namespace saucer::scheme

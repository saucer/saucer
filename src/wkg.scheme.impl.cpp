#include "wkg.scheme.impl.hpp"

#include "handle.hpp"
#include "gtk.utils.hpp"

#include <rebind/enum.hpp>

namespace saucer::scheme
{
    void handler::add_callback(WebKitWebView *id, callback callback)
    {
        m_callbacks.emplace(id, std::move(callback));
    }

    void handler::del_callback(WebKitWebView *id)
    {
        m_callbacks.erase(id);
    }

    void handler::handle(WebKitURISchemeRequest *raw, handler *state)
    {
        auto request           = utils::g_object_ptr<WebKitURISchemeRequest>::ref(raw);
        auto *const identifier = webkit_uri_scheme_request_get_web_view(request.get());

        if (!state->m_callbacks.contains(identifier))
        {
            return;
        }

        auto resolve = [request](const scheme::response &response)
        {
            const auto data = response.data;
            const auto size = static_cast<gssize>(data.size());

            auto bytes  = utils::g_bytes_ptr{g_bytes_new(data.data(), size)};
            auto stream = utils::g_object_ptr<GInputStream>{g_memory_input_stream_new_from_bytes(bytes.get())};

            auto res = utils::g_object_ptr<WebKitURISchemeResponse>{webkit_uri_scheme_response_new(stream.get(), size)};
            auto *const headers = soup_message_headers_new(SOUP_MESSAGE_HEADERS_RESPONSE);

            for (const auto &[name, value] : response.headers)
            {
                soup_message_headers_append(headers, name.c_str(), value.c_str());
            }

            webkit_uri_scheme_response_set_content_type(res.get(), response.mime.c_str());
            webkit_uri_scheme_response_set_status(res.get(), response.status, "");
            webkit_uri_scheme_response_set_http_headers(res.get(), headers);

            webkit_uri_scheme_request_finish_with_response(request.get(), res.get());
        };

        auto reject = [request](const scheme::error &error)
        {
            static auto quark = webkit_network_error_quark();

            auto value = std::to_underlying(error);
            auto name  = std::string{rebind::find_enum_name(error).value_or("unknown")};
            auto err   = utils::handle<GError *, g_error_free>{g_error_new(quark, value, "%s", name.c_str())};

            webkit_uri_scheme_request_finish_error(request.get(), err.get());
        };

        auto &[app, policy, resolver] = state->m_callbacks.at(identifier);

        auto executor = scheme::executor{std::move(resolve), std::move(reject)};
        auto req      = scheme::request{{request}};

        if (policy != launch::async)
        {
            return std::invoke(resolver, std::move(req), std::move(executor));
        }

        app->pool().emplace([resolver, executor = std::move(executor), req = std::move(req)]() mutable
                            { std::invoke(resolver, std::move(req), std::move(executor)); });
    }
} // namespace saucer::scheme

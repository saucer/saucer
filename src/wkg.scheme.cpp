#include "wkg.scheme.impl.hpp"

#include "gtk.utils.hpp"

#include <rebind/enum.hpp>

namespace saucer
{
    request::request(impl data) : m_impl(std::make_unique<impl>(data)) {}

    request::~request() = default;

    std::string request::url() const
    {
        return webkit_uri_scheme_request_get_uri(m_impl->request);
    }

    std::string request::method() const
    {
        return webkit_uri_scheme_request_get_http_method(m_impl->request);
    }

    stash<> request::content() const
    {
        const auto stream = g_object_ptr<GInputStream>{webkit_uri_scheme_request_get_http_body(m_impl->request)};

        if (!stream)
        {
            return stash<>::empty();
        }

        const auto content = g_bytes_ptr{g_input_stream_read_bytes(stream.get(), G_MAXSSIZE, nullptr, nullptr)};

        if (!content)
        {
            return stash<>::empty();
        }

        gsize size{};
        const auto *data = reinterpret_cast<const std::uint8_t *>(g_bytes_get_data(content.get(), &size));

        return stash<>::from({data, data + size});
    }

    std::map<std::string, std::string> request::headers() const
    {
        auto *const headers = webkit_uri_scheme_request_get_http_headers(m_impl->request);

        std::map<std::string, std::string> rtn;

        soup_message_headers_foreach(
            headers, [](const auto *name, const auto *value, gpointer data)
            { reinterpret_cast<decltype(rtn) *>(data)->emplace(name, value); }, &rtn);

        return rtn;
    }

    void scheme_state::handle(WebKitURISchemeRequest *request, scheme_state *state)
    {
        auto req = saucer::request{{request}};

        if (!state->handler)
        {
            return;
        }

        auto result = std::invoke(state->handler, req);

        if (!result.has_value())
        {
            static auto quark = webkit_network_error_quark();
            auto error        = result.error();

            auto meta = rebind::enum_value(error);
            auto name = meta ? meta->name : "unknown";

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
} // namespace saucer
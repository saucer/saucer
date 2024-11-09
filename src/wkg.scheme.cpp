#include "wkg.scheme.impl.hpp"

#include "gtk.utils.hpp"

namespace saucer::scheme
{
    request::request(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    request::request(const request &other) : m_impl(std::make_unique<impl>(*other.m_impl)) {}

    request::request(request &&other) noexcept : m_impl(std::move(other.m_impl)) {}

    request::~request() = default;

    std::string request::url() const
    {
        return webkit_uri_scheme_request_get_uri(m_impl->request.get());
    }

    std::string request::method() const
    {
        return webkit_uri_scheme_request_get_http_method(m_impl->request.get());
    }

    stash<> request::content() const
    {
        auto stream = utils::g_object_ptr<GInputStream>{webkit_uri_scheme_request_get_http_body(m_impl->request.get())};

        if (!stream)
        {
            return stash<>::empty();
        }

        auto content = utils::g_bytes_ptr{g_input_stream_read_bytes(stream.get(), G_MAXSSIZE, nullptr, nullptr)};

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
        auto *const headers = webkit_uri_scheme_request_get_http_headers(m_impl->request.get());

        std::map<std::string, std::string> rtn;

        soup_message_headers_foreach(
            headers, [](const auto *name, const auto *value, gpointer data)
            { reinterpret_cast<decltype(rtn) *>(data)->emplace(name, value); }, &rtn);

        return rtn;
    }
} // namespace saucer::scheme

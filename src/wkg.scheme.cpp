#include "wkg.scheme.impl.hpp"

#include "error.impl.hpp"

namespace saucer::scheme
{
    request::request(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    request::request(const request &other) : request(*other.m_impl) {}

    request::request(request &&) noexcept = default;

    request::~request() = default;

    url request::url() const
    {
        return unwrap_safe(url::parse(webkit_uri_scheme_request_get_uri(m_impl->request.get())));
    }

    std::string request::method() const
    {
        return webkit_uri_scheme_request_get_http_method(m_impl->request.get());
    }

    stash request::content() const
    {
        auto stream = utils::g_object_ptr<GInputStream>{webkit_uri_scheme_request_get_http_body(m_impl->request.get())};

        if (!stream)
        {
            return stash::empty();
        }

        static constexpr auto chunk_size = 4096;

        std::vector<std::uint8_t> content;
        content.reserve(chunk_size);

        gssize read{};
        guint8 buffer[chunk_size];

        while ((read = g_input_stream_read(stream.get(), buffer, chunk_size, nullptr, nullptr)) > 0)
        {
            content.insert(content.end(), buffer, buffer + read);
        }

        if (read == -1)
        {
            return stash::empty();
        }

        return stash::from(std::move(content));
    }

    std::map<std::string, std::string> request::headers() const
    {
        auto *const headers = webkit_uri_scheme_request_get_http_headers(m_impl->request.get());
        auto rtn            = std::map<std::string, std::string>{};

        auto emplace = [](const auto *name, const auto *value, gpointer data)
        {
            reinterpret_cast<decltype(rtn) *>(data)->emplace(name, value);
        };
        soup_message_headers_foreach(headers, emplace, &rtn);

        return rtn;
    }
} // namespace saucer::scheme

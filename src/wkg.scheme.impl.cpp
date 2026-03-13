#include "wkg.scheme.impl.hpp"

#include "handle.hpp"
#include "modules/stable/webkitgtk.hpp"

#include <rebind/utils/enum.hpp>

namespace saucer::scheme
{
    stash_stream::stash_stream(int read, int write) : platform(std::make_shared<native>())
    {
        platform->read_fd  = read;
        platform->write_fd = write;
        platform->stream   = G_UNIX_INPUT_STREAM(g_unix_input_stream_new(read, true));
    }

    std::size_t stash_stream::type() const
    {
        return id_of<stash_stream>();
    }

    std::unique_ptr<stash::impl> stash_stream::clone() const
    {
        return std::make_unique<stash_stream>(*this);
    }

    stash::span stash_stream::data() const
    {
        return {};
    }

    stash_stream::native::~native()
    {
        close(write_fd);
    }

    std::pair<utils::g_object_ptr<GInputStream>, gint64> stream_of(const stash &stash)
    {
        if (auto *const stream = stash.native().stream; stream)
        {
            return {stream, -1};
        }

        const auto data  = stash.data();
        const auto bytes = utils::g_bytes_ptr{g_bytes_new(data.data(), data.size())};

        return {g_memory_input_stream_new_from_bytes(bytes.get()), static_cast<gint64>(data.size())};
    }

    void handler::add_callback(WebKitWebView *id, scheme::resolver callback)
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
            const auto stash = response.data;

            auto [stream, size] = stream_of(stash);
            auto res            = utils::g_object_ptr<WebKitURISchemeResponse>{webkit_uri_scheme_response_new(stream.get(), size)};

            auto *const headers = soup_message_headers_new(SOUP_MESSAGE_HEADERS_RESPONSE);

            for (const auto &[name, value] : response.headers)
            {
                soup_message_headers_append(headers, name.c_str(), value.c_str());
            }

            webkit_uri_scheme_response_set_content_type(res.get(), response.mime.c_str());
            webkit_uri_scheme_response_set_status(res.get(), response.status, nullptr);
            webkit_uri_scheme_response_set_http_headers(res.get(), headers);

            webkit_uri_scheme_request_finish_with_response(request.get(), res.get());
        };

        auto reject = [request](const scheme::error &error)
        {
            static auto quark = webkit_network_error_quark();

            auto value = std::to_underlying(error);
            auto name  = std::string{rebind::utils::find_enum_name(error).value_or("unknown")};
            auto err   = utils::handle<GError *, g_error_free>{g_error_new(quark, value, "%s", name.c_str())};

            webkit_uri_scheme_request_finish_error(request.get(), err.get());
        };

        auto executor = scheme::executor{std::move(resolve), std::move(reject)};
        auto req      = scheme::request{{request}};

        return state->m_callbacks[identifier](std::move(req), std::move(executor));
    }
} // namespace saucer::scheme

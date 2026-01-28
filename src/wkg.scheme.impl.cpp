#include "wkg.scheme.impl.hpp"

#include "handle.hpp"

#include <rebind/utils/enum.hpp>

#include <unistd.h>
#include <gio/gunixinputstream.h>

namespace saucer::scheme
{
    stream_writer::stream_writer(std::shared_ptr<impl> impl) : m_impl(std::move(impl)) {}
    stream_writer::stream_writer(const stream_writer &) = default;
    stream_writer::stream_writer(stream_writer &&) noexcept = default;
    stream_writer::~stream_writer() = default;

    void stream_writer::start(const stream_response &response)
    {
        if (!m_impl || m_impl->started.exchange(true) || m_impl->finished)
        {
            return;
        }

        int fds[2];
        if (pipe(fds) == -1)
        {
            m_impl->started = false;
            return;
        }

        m_impl->write_fd = fds[1];

        auto stream = utils::g_object_ptr<GInputStream>{g_unix_input_stream_new(fds[0], TRUE)};
        auto res    = utils::g_object_ptr<WebKitURISchemeResponse>{webkit_uri_scheme_response_new(stream.get(), -1)};

        auto *const headers = soup_message_headers_new(SOUP_MESSAGE_HEADERS_RESPONSE);

        for (const auto &[name, value] : response.headers)
        {
            soup_message_headers_append(headers, name.c_str(), value.c_str());
        }

        webkit_uri_scheme_response_set_content_type(res.get(), response.mime.c_str());
        webkit_uri_scheme_response_set_status(res.get(), response.status, nullptr);
        webkit_uri_scheme_response_set_http_headers(res.get(), headers);

        webkit_uri_scheme_request_finish_with_response(m_impl->request.get(), res.get());
    }

    void stream_writer::write(stash data)
    {
        if (!m_impl || !m_impl->started || m_impl->finished || m_impl->write_fd < 0)
        {
            return;
        }

        const auto *ptr       = data.data();
        std::size_t remaining = data.size();

        while (remaining > 0)
        {
            auto written = ::write(m_impl->write_fd, ptr, remaining);

            if (written < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                break;
            }

            ptr += written;
            remaining -= static_cast<std::size_t>(written);
        }
    }

    void stream_writer::finish()
    {
        if (!m_impl || !m_impl->started || m_impl->finished.exchange(true))
        {
            return;
        }

        if (m_impl->write_fd >= 0)
        {
            close(m_impl->write_fd);
            m_impl->write_fd = -1;
        }
    }

    void stream_writer::reject(error err)
    {
        if (!m_impl || m_impl->finished.exchange(true))
        {
            return;
        }

        if (m_impl->write_fd >= 0)
        {
            close(m_impl->write_fd);
            m_impl->write_fd = -1;
        }

        if (!m_impl->started)
        {
            static auto quark = webkit_network_error_quark();
            auto value        = std::to_underlying(err);
            auto name         = std::string{rebind::utils::find_enum_name(err).value_or("unknown")};
            auto error        = utils::handle<GError *, g_error_free>{g_error_new(quark, value, "%s", name.c_str())};

            webkit_uri_scheme_request_finish_error(m_impl->request.get(), error.get());
        }
    }

    bool stream_writer::valid() const
    {
        return m_impl && !m_impl->finished;
    }

    void stream_handler::add_callback(WebKitWebView *id, scheme::stream_resolver callback)
    {
        m_callbacks.emplace(id, std::move(callback));
    }

    void stream_handler::del_callback(WebKitWebView *id)
    {
        m_callbacks.erase(id);
    }

    void stream_handler::handle(WebKitURISchemeRequest *raw, stream_handler *state)
    {
        auto request           = utils::g_object_ptr<WebKitURISchemeRequest>::ref(raw);
        auto *const identifier = webkit_uri_scheme_request_get_web_view(request.get());

        if (!state->m_callbacks.contains(identifier))
        {
            return;
        }

        auto writer_impl     = std::make_shared<stream_writer::impl>();
        writer_impl->request = request;

        auto writer = stream_writer{writer_impl};
        auto req    = scheme::request{{request}};

        return state->m_callbacks[identifier](std::move(req), std::move(writer));
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
            const auto data = response.data;
            const auto size = static_cast<gssize>(data.size());

            auto bytes  = utils::g_bytes_ptr{g_bytes_new(data.data(), size)};
            auto stream = utils::g_object_ptr<GInputStream>{g_memory_input_stream_new_from_bytes(bytes.get())};

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

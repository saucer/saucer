#include "webview.hpp"

#include "uri.hpp"
#include "request.hpp"

#include <format>
#include <algorithm>

namespace saucer
{
    bool webview::on_message(std::string_view message)
    {
        using traits = modules::traits<webview>;

        if (std::ranges::any_of(modules(), [&](auto &module) { return module.template invoke<traits::on_message>(message); }))
        {
            return true;
        }

        if (!m_attributes)
        {
            return false;
        }

        auto request = request::parse(message);

        if (!request)
        {
            return false;
        }

        overload visitor = {
            [this](const request::start_resize &data) { start_resize(static_cast<window_edge>(data.edge)); },
            [this](const request::start_drag &) { start_drag(); },
            [this](const request::maximize &data) { set_maximized(data.value); },
            [this](const request::minimize &data) { set_minimized(data.value); },
            [this](const request::close &) { close(); },
            [this](const request::maximized &data) { resolve(data.id, std::format("{}", maximized())); },
            [this](const request::minimized &data) { resolve(data.id, std::format("{}", minimized())); },
        };

        std::visit(visitor, request.value());

        return true;
    }

    void webview::reject(std::uint64_t id, std::string_view reason)
    {
        execute(std::format(
            R"(
                window.saucer.internal.rpc[{0}].reject({1});
                delete window.saucer.internal.rpc[{0}];
            )",
            id, reason));
    }

    void webview::resolve(std::uint64_t id, std::string_view result)
    {
        execute(std::format(
            R"(
                window.saucer.internal.rpc[{0}].resolve({1});
                delete window.saucer.internal.rpc[{0}];
            )",
            id, result));
    }

    void webview::set_url(const std::string &url)
    {
        auto parsed = uri::parse(url);

        if (!parsed.has_value())
        {
            return;
        }

        set_url(parsed.value());
    }

    void webview::serve(fs::path file)
    {
        set_url(uri::make({.scheme = "saucer", .host = "embedded", .path = std::move(file)}));
    }

    void webview::embed(embedded_files files)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, files = std::move(files)]() mutable { return embed(std::move(files)); });
        }

        m_embedded_files.merge(std::move(files));

        auto func = [this](const scheme::request &request) -> std::expected<scheme::response, scheme::error>
        {
            auto url = request.url();

            if (url.scheme() != "saucer" || url.host() != "embedded")
            {
                return std::unexpected{scheme::error::invalid};
            }

            auto file = url.path();

            if (!m_embedded_files.contains(file))
            {
                return std::unexpected{scheme::error::not_found};
            }

            const auto &data = m_embedded_files.at(file);

            return scheme::response{
                .data    = data.content,
                .mime    = data.mime,
                .headers = {{"Access-Control-Allow-Origin", "*"}},
            };
        };

        handle_scheme("saucer", func);
    }

    void webview::clear_embedded()
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this] { return clear_embedded(); });
        }

        m_embedded_files.clear();
        remove_scheme("saucer");
    }

    void webview::clear_embedded(const fs::path &file)
    {
        if (!m_parent->thread_safe())
        {
            return m_parent->dispatch([this, file] { return clear_embedded(file); });
        }

        m_embedded_files.erase(file);
    }
} // namespace saucer

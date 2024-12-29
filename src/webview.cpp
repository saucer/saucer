#include "webview.hpp"

#include "requests.hpp"

#include <algorithm>

#include <fmt/core.h>

namespace saucer
{
    bool webview::on_message(const std::string &message)
    {
        if (std::ranges::any_of(modules(), [&message](auto &module) { return module.template invoke<0>(message); }))
        {
            return true;
        }

        auto request = requests::parse(message);

        if (!request)
        {
            return false;
        }

        if (std::holds_alternative<requests::resize>(request.value()))
        {
            const auto data = std::get<requests::resize>(request.value());
            start_resize(static_cast<window_edge>(data.edge));

            return true;
        }

        if (std::holds_alternative<requests::drag>(request.value()))
        {
            start_drag();
            return true;
        }

        return false;
    }

    void webview::embed(embedded_files files, launch policy)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, files = std::move(files)]() mutable { return embed(std::move(files)); });
        }

        m_embedded_files.merge(std::move(files));

        auto func = [this](const auto &request) -> std::expected<scheme::response, scheme::error>
        {
            static constexpr std::string_view prefix = "/embedded/";

            const auto url   = request.url();
            const auto start = url.find(prefix) + prefix.size();

            if (start >= url.size())
            {
                return std::unexpected{scheme::error::invalid};
            }

            const auto file = url.substr(start, url.find_first_of("#?") - start);

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

        handle_scheme("saucer", func, policy);
    }

    void webview::serve(const std::string &file)
    {
        set_url(fmt::format("saucer://embedded/{}", file));
    }

    void webview::clear_embedded()
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this] { return clear_embedded(); });
        }

        m_embedded_files.clear();
        remove_scheme("saucer");
    }

    void webview::clear_embedded(const std::string &file)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, file] { return clear_embedded(file); });
        }

        m_embedded_files.erase(file);
    }
} // namespace saucer

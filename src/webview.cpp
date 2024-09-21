#include "webview.hpp"

#include <fmt/core.h>

namespace saucer
{
    webview::impl *webview::native() const
    {
        return m_impl.get();
    }

    void webview::embed(embedded_files files)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, files = std::move(files)]() mutable { return embed(std::move(files)); });
        }

        m_embedded_files.merge(std::move(files));

        auto handler = [this](const auto &request) -> scheme_handler::result_type
        {
            static constexpr std::string_view prefix = "/embedded/";

            const auto url   = request.url();
            const auto start = url.find(prefix) + prefix.size();

            if (start >= url.size())
            {
                return tl::unexpected{request_error::invalid};
            }

            const auto file = url.substr(start, url.find_first_of("#?") - start);

            if (!m_embedded_files.contains(file))
            {
                return tl::unexpected{request_error::not_found};
            }

            const auto &data = m_embedded_files.at(file);

            return response{
                .data    = data.content,
                .mime    = data.mime,
                .headers = {{"Access-Control-Allow-Origin", "*"}},
            };
        };

        handle_scheme("saucer", handler);
    }

    void webview::serve(const std::string &file)
    {
        set_url(fmt::format("saucer://embedded/{}", file));
    }

    void webview::clear_embedded()
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this]() { return clear_embedded(); });
        }

        m_embedded_files.clear();
        remove_scheme("saucer");
    }

    void webview::clear_embedded(const std::string &file)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, file]() { return clear_embedded(file); });
        }

        m_embedded_files.erase(file);
    }
} // namespace saucer

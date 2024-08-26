#include "webview.hpp"

#include <fmt/core.h>

namespace saucer
{
    void webview::embed(embedded_files files)
    {
        {
            const auto locked = m_embedded_files.write();
            locked->merge(std::move(files));
        }

        auto handler = [this](const auto &request) -> scheme_handler::result_type
        {
            static constexpr std::string_view prefix = "/embedded/";

            const auto url   = request.url();
            const auto start = url.find(prefix) + prefix.size();

            if (start >= url.size())
            {
                return tl::unexpected{request_error::invalid};
            }

            const auto file   = url.substr(start, url.find_first_of("#?") - start);
            const auto locked = m_embedded_files.read();

            if (!locked->contains(file))
            {
                return tl::unexpected{request_error::not_found};
            }

            const auto &data = locked->at(file);

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
        {
            const auto locked = m_embedded_files.write();
            locked->clear();
        }
        remove_scheme("saucer");
    }

    void webview::clear_embedded(const std::string &file)
    {
        const auto locked = m_embedded_files.write();
        locked->erase(file);
    }
} // namespace saucer

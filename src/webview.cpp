#include "webview.hpp"

namespace saucer
{
    void webview::embed(embedded_files &&files)
    {
        auto locked = m_embedded_files.write();
        locked->merge(files);

        auto handler = [this](const auto &request) -> scheme_handler::result_type
        {
            auto url   = request.url();
            auto start = url.find('/') + 1;

            if (start == std::string::npos)
            {
                return tl::unexpected{request_error::bad_url};
            }

            auto file   = url.substr(start, url.find_first_of("#?") - start);
            auto locked = m_embedded_files.read();

            if (!locked->contains(file))
            {
                return tl::unexpected{request_error::not_found};
            }

            const auto &data = locked->at(file);

            return response{
                .mime = data.mime,
                .data = data.content,
            };
        };

        handle_scheme("saucer", handler);
    }

    void webview::clear_embedded()
    {
        auto locked = m_embedded_files.write();

        locked->clear();
        remove_scheme("saucer");
    }
} // namespace saucer

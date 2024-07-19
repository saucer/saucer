#pragma once

#include <map>
#include <string>
#include <saucer/webview.hpp>

#include "assets/logo.png.hpp"
#include "src/index.html.hpp"
#include "src/index.js.hpp"
#include "style/style.css.hpp"

namespace saucer::embedded
{
    inline auto all()
    {
        std::map<std::string, embedded_file> rtn;

        rtn.emplace("assets/logo.png", embedded_file{"image/png", stash<const std::uint8_t>::view(assets_logo_png)});
        rtn.emplace("src/index.html", embedded_file{"text/html", stash<const std::uint8_t>::view(src_index_html)});
        rtn.emplace("src/index.js", embedded_file{"application/javascript", stash<const std::uint8_t>::view(src_index_js)});
        rtn.emplace("style/style.css", embedded_file{"text/css", stash<const std::uint8_t>::view(style_style_css)});

        return rtn;
    }
} // namespace saucer::embedded

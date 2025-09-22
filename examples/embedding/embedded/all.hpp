#pragma once

#include <filesystem>
#include <unordered_map>

#include <saucer/webview.hpp>

#include "assets/logo.png.hpp"
#include "src/index.html.hpp"
#include "src/index.js.hpp"
#include "style/style.css.hpp"

namespace saucer::embedded
{
    inline auto all()
    {
        std::unordered_map<std::filesystem::path, embedded_file> rtn;

        rtn.emplace("/assets/logo.png", embedded_file{saucer::stash<>::view(assets_logo_png), "image/png"});
        rtn.emplace("/src/index.html", embedded_file{saucer::stash<>::view(src_index_html), "text/html"});
        rtn.emplace("/src/index.js", embedded_file{saucer::stash<>::view(src_index_js), "application/javascript"});
        rtn.emplace("/style/style.css", embedded_file{saucer::stash<>::view(style_style_css), "text/css"});

        return rtn;
    }
} // namespace saucer::embedded

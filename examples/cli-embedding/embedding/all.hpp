#pragma once
#include <map>
#include <tuple>
#include <string>
#include <saucer/webview.hpp>

#include "style/style.css.hpp"
#include "assets/logo.png.hpp"
#include "src/index.js.hpp"
#include "src/index.html.hpp"

namespace embedded {
	inline auto get_all_files() {
		std::map<const std::string, const saucer::embedded_file> rtn;
		rtn.emplace("style/style.css", saucer::embedded_file{"text/css", 97, embedded_style_style_css});
		rtn.emplace("assets/logo.png", saucer::embedded_file{"image/png", 133033, embedded_assets_logo_png});
		rtn.emplace("src/index.js", saucer::embedded_file{"application/javascript", 150, embedded_src_index_js});
		rtn.emplace("src/index.html", saucer::embedded_file{"text/html", 318, embedded_src_index_html});
		return rtn;
	}
} // namespace embedded
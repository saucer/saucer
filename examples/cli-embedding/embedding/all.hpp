#pragma once
#include <map>
#include <tuple>
#include <string>
#include <saucer/webview.hpp>

#include "logo.png.hpp"
#include "index.js.hpp"
#include "index.html.hpp"
#include "style.css.hpp"

namespace embedded {
	inline auto get_all_files() {
		std::map<const std::string, const saucer::embedded_file> rtn;
		rtn.emplace("logo.png", saucer::embedded_file{"image/png", 133033, embedded_logo_png});
		rtn.emplace("index.js", saucer::embedded_file{"application/javascript", 150, embedded_index_js});
		rtn.emplace("index.html", saucer::embedded_file{"text/html", 299, embedded_index_html});
		rtn.emplace("style.css", saucer::embedded_file{"text/css", 97, embedded_style_css});
		return rtn;
	}
} // namespace embedded
#pragma once

#include "webview.hpp"

#include "utils.gtk.hpp"
#include "scheme.webkit.impl.hpp"

#include <vector>
#include <unordered_map>

#include <webkit/webkit.h>

namespace saucer
{
    using script_ptr = object_ptr<WebKitUserScript, webkit_user_script_ref, webkit_user_script_unref>;

    struct webview::impl
    {
        WebKitWebView *web_view;

      public:
        bool context_menu;
        std::vector<script_ptr> scripts;

      public:
        bool dom_loaded{false};
        std::vector<std::string> pending;

      public:
        object_ptr<WebKitSettings> settings;
        std::unordered_map<std::string, std::unique_ptr<scheme_state>> schemes;

      public:
        static const std::string &inject_script();
        static constinit std::string_view ready_script;

      public:
        static WebKitSettings *make_settings(const saucer::options &options);
    };
} // namespace saucer

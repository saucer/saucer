#pragma once

#include "webview.hpp"
#include "utils.gtk.hpp"

#include <vector>
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
        static const std::string &inject_script();
        static constinit std::string_view ready_script;
    };
} // namespace saucer

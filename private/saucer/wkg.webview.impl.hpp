#pragma once

#include "webview.hpp"

#include "gtk.utils.hpp"
#include "wkg.scheme.impl.hpp"

#include <vector>
#include <unordered_map>

#include <webkit/webkit.h>

namespace saucer
{
    using script_ptr = utils::ref_ptr<WebKitUserScript, webkit_user_script_ref, webkit_user_script_unref>;

    struct webview::impl
    {
        WebKitWebView *web_view;

      public:
        gulong msg_received;
        WebKitUserContentManager *manager;

      public:
        bool context_menu{true};
        std::vector<std::pair<script_ptr, bool>> scripts;

      public:
        bool dom_loaded{false};
        std::vector<std::string> pending;

      public:
        utils::g_object_ptr<WebKitSettings> settings;

      public:
        template <web_event>
        void setup(webview *);

      public:
        static const std::string &inject_script();
        static WebKitSettings *make_settings(const preferences &);

      public:
        static constinit std::string_view ready_script;
        static inline std::unordered_map<std::string, std::unique_ptr<scheme::handler>> schemes;
    };
} // namespace saucer

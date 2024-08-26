#pragma once

#include "webview.hpp"

#include "gtk.utils.hpp"
#include "wkg.scheme.impl.hpp"

#include <vector>
#include <unordered_map>

#include <webkit/webkit.h>

namespace saucer
{
    using script_ptr = ref_ptr<WebKitUserScript, webkit_user_script_ref, webkit_user_script_unref>;

    struct webview::impl
    {
        g_object_ptr<WebKitWebView> web_view;

      public:
        gulong message_received;
        WebKitUserContentManager *manager;

      public:
        bool context_menu{false};
        std::vector<std::pair<script_ptr, bool>> scripts;

      public:
        bool dom_loaded{false};
        std::vector<std::string> pending;

      public:
        g_object_ptr<WebKitSettings> settings;
        std::unordered_map<std::string, std::unique_ptr<scheme_state>> schemes;

      public:
        template <web_event>
        void setup(webview *);

      public:
        static const std::string &inject_script();
        static constinit std::string_view ready_script;

      public:
        static WebKitSettings *make_settings(const saucer::options &);
    };
} // namespace saucer

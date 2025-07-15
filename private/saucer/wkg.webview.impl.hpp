#pragma once

#include "webview.impl.hpp"

#include "gtk.utils.hpp"
#include "wkg.scheme.impl.hpp"

#include <vector>
#include <string_view>

#include <webkit/webkit.h>

namespace saucer
{
    using script_ptr = utils::ref_ptr<WebKitUserScript, webkit_user_script_ref, webkit_user_script_unref>;

    struct wkg_script
    {
        script_ptr ref;
        bool permanent;
    };

    struct webview::impl::native
    {
        WebKitWebView *web_view;

      public:
        gulong msg_received;
        WebKitUserContentManager *manager;

      public:
        bool context_menu{true};

      public:
        std::uint64_t id_counter{0};
        std::map<std::uint64_t, wkg_script> scripts;

      public:
        bool dom_loaded{false};
        std::vector<std::string> pending;

      public:
        utils::g_object_ptr<WebKitSettings> settings;

      public:
        template <event>
        void setup(impl *);

      public:
        static std::string inject_script();
        static WebKitSettings *make_settings(const options &);

      public:
        static constinit std::string_view ready_script;
        static inline std::unordered_map<std::string, std::unique_ptr<scheme::handler>> schemes;
    };
} // namespace saucer

#pragma once

#include <saucer/scheme.hpp>

#include "gtk.utils.hpp"
#include "stash.impl.hpp"

#include <webkit/webkit.h>
#include <gio/gunixinputstream.h>

namespace saucer::scheme
{
    struct request::impl
    {
        utils::g_object_ptr<WebKitURISchemeRequest> request;
    };

    struct stash_stream : stash::impl
    {
        struct native;

      public:
        std::shared_ptr<native> platform;

      public:
        stash_stream(int, int);

      public:
        [[nodiscard]] stash::span data() const override;

      public:
        [[nodiscard]] std::size_t type() const override;
        [[nodiscard]] std::unique_ptr<impl> clone() const override;
    };

    struct stash_stream::native
    {
        ~native();

      public:
        int read_fd, write_fd;
        utils::g_object_ptr<GUnixInputStream> stream;
    };

    class handler
    {
        std::unordered_map<WebKitWebView *, scheme::resolver> m_callbacks;

      public:
        void add_callback(WebKitWebView *, scheme::resolver);
        void del_callback(WebKitWebView *);

      public:
        static void handle(WebKitURISchemeRequest *, handler *);
    };
} // namespace saucer::scheme

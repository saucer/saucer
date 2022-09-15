#pragma once
#include "../constants.hpp"

#include <string>

namespace saucer
{
    class smartview;
    template <backend_type Backend> class module
    {
        friend class smartview;

      protected:
        struct webview_impl;
        struct window_impl;

      protected:
        webview_impl *m_webview_impl;
        window_impl *m_window_impl;
        smartview &m_smartview;

      public:
        virtual ~module() = default;
        module(smartview &, webview_impl *, window_impl *);

      protected:
        virtual std::string get_name() const = 0;
        virtual std::string get_version() const = 0;
    };
} // namespace saucer

#include "module.inl"
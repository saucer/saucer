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
        virtual void load(smartview &, webview_impl *, window_impl *) = 0;

      public:
        virtual ~module() = default;

      protected:
        virtual std::string get_name() const = 0;
        virtual std::string get_version() const = 0;
    };
} // namespace saucer
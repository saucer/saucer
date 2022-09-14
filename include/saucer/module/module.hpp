#pragma once
#include "../constants.hpp"

#include <memory>
#include <string>

namespace saucer
{
    class smartview;
    class module
    {
        friend class smartview;

      protected:
        template <backend_type Backend> struct webview_impl;
        template <backend_type Backend> struct window_impl;

      protected:
        smartview &m_smartview;

      public:
        virtual ~module();
        module(smartview &);

      public:
        virtual void load() = 0;

      public:
        virtual std::string get_name() const = 0;
        virtual std::string get_version() const = 0;
    };

    template <typename T> struct is_implementation_module;
    template <typename T> inline constexpr bool is_implementation_module_v = is_implementation_module<T>::value;
} // namespace saucer

#include "module.inl"
#pragma once

#include "webview.hpp"
#include "config.hpp"

#include <string_view>

#include <memory>
#include <string>

#include <coco/promise/promise.hpp>

namespace saucer
{
    struct smartview_base : webview
    {
        struct impl;

      protected:
        std::unique_ptr<impl> m_impl;

      protected:
        smartview_base(webview &&, std::unique_ptr<serializer_core>);

      public:
        smartview_base(smartview_base &&) noexcept;

      public:
        ~smartview_base();

      protected:
        void add_function(std::string, serializer_core::function &&);
        void add_evaluation(serializer_core::resolver &&, std::string_view);

      public:
        [[sc::thread_safe]] void unexpose();
        [[sc::thread_safe]] void unexpose(const std::string &name);
    };

    template <Serializer Serializer>
    class basic_smartview : public smartview_base
    {
        basic_smartview(webview &&);

      public:
        basic_smartview(basic_smartview &&) noexcept;

      public:
        static result<basic_smartview> create(const options &);

      public:
        template <typename T>
        [[sc::thread_safe]] void expose(std::string name, T &&func);

      public:
        template <typename... Ts>
        [[sc::thread_safe]] void execute(format_string<Serializer, Ts...> code, Ts &&...params);

      public:
        template <typename R, typename... Ts>
        [[sc::thread_safe]] [[nodiscard]] auto evaluate(format_string<Serializer, Ts...> code, Ts &&...params);
    };

    using smartview = basic_smartview<default_serializer>;
} // namespace saucer

#include "smartview.inl"

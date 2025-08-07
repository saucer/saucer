#pragma once

#include "webview.hpp"
#include "config.hpp"

#include <optional>
#include <string_view>

#include <memory>
#include <string>

#include <coco/promise/promise.hpp>

namespace saucer
{
    struct smartview_core : webview
    {
        struct impl;

      protected:
        std::unique_ptr<impl> m_impl;

      protected:
        smartview_core(webview &&, std::unique_ptr<serializer_core>);

      public:
        smartview_core(smartview_core &&) noexcept;

      public:
        ~smartview_core();

      protected:
        void add_function(std::string, serializer_core::function &&);
        void add_evaluation(serializer_core::resolver &&, std::string_view);

      public:
        [[sc::thread_safe]] void unexpose();
        [[sc::thread_safe]] void unexpose(const std::string &name);
    };

    template <Serializer Serializer = default_serializer>
    class smartview : public smartview_core
    {
        smartview(webview &&);

      public:
        smartview(smartview &&) noexcept;

      public:
        static result<smartview> create(const options &);

      public:
        template <typename T>
        [[sc::thread_safe]] void expose(std::string name, T &&func);

      public:
        template <typename... Ts>
        [[sc::thread_safe]] void execute(format_string<Serializer, Ts...> code, Ts &&...params);

      public:
        template <typename R, typename... Ts>
        [[sc::thread_safe]] [[nodiscard]] coco::future<R> evaluate(format_string<Serializer, Ts...> code, Ts &&...params);
    };
} // namespace saucer

#include "smartview.inl"

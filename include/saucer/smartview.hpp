#pragma once

#include "webview.hpp"
#include "config.hpp"

#include <atomic>
#include <memory>

#include <string>
#include <string_view>

#include <coco/promise/promise.hpp>

namespace saucer
{
    class smartview_core : public webview
    {
        struct impl;

      private:
        std::unique_ptr<impl> m_impl;

      protected:
        std::atomic_uint64_t m_id_counter{0};

      protected:
        smartview_core(std::unique_ptr<serializer_core>, const preferences &);

      public:
        ~smartview_core() override;

      protected:
        bool on_message(std::string_view) override;

      protected:
        void call(std::unique_ptr<function_data>);
        void resolve(std::unique_ptr<result_data>);

      protected:
        void add_function(std::string, serializer_core::function &&);
        void add_evaluation(serializer_core::resolver &&, std::string_view);

      public:
        [[sc::thread_safe]] void clear_exposed();
        [[sc::thread_safe]] void clear_exposed(const std::string &name);
    };

    template <Serializer Serializer = default_serializer>
    struct smartview : public smartview_core
    {
        smartview(const preferences &);

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

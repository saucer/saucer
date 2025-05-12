#pragma once

#include "webview.hpp"
#include "config.hpp"

#include <atomic>
#include <string>
#include <memory>

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
        smartview_core(std::unique_ptr<serializer>, const preferences &);

      public:
        ~smartview_core() override;

      protected:
        bool on_message(const std::string &) override;

      protected:
        void call(std::unique_ptr<function_data>);
        void resolve(std::unique_ptr<result_data>);

      protected:
        void add_function(std::string, serializer::function &&, launch);
        void add_evaluation(serializer::resolver &&, const std::string &);

      public:
        [[sc::thread_safe]] void clear_exposed();
        [[sc::thread_safe]] void clear_exposed(const std::string &name);
    };

    template <Serializer Serializer = default_serializer>
    struct smartview : public smartview_core
    {
        smartview(const preferences &);

      public:
        template <typename Function>
        [[sc::thread_safe]] void expose(std::string name, Function &&func, launch policy = launch::sync);

      public:
        template <typename... Params>
        [[sc::thread_safe]] void execute(std::string_view code, Params &&...params);

      public:
        template <typename Return, typename... Params>
        [[sc::thread_safe]] [[nodiscard]] coco::future<Return> evaluate(std::string_view code, Params &&...params);
    };
} // namespace saucer

#include "smartview.inl"

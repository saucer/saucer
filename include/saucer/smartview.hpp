#pragma once

#include "webview.hpp"
#include "serializers/glaze/glaze.hpp"

#include <tuple>
#include <future>
#include <atomic>
#include <memory>
#include <string>

namespace saucer
{
    enum class launch
    {
        sync,
        async,
    };

    class smartview_core : public webview
    {
        struct impl;

      private:
        std::unique_ptr<impl> m_impl;

      protected:
        std::atomic_uint64_t m_id_counter{0};

      protected:
        smartview_core(std::unique_ptr<serializer>, const preferences & = {});

      public:
        ~smartview_core() override;

      protected:
        bool on_message(const std::string &) override;

      protected:
        [[sc::thread_safe]] void call(std::unique_ptr<message_data>);
        [[sc::thread_safe]] void resolve(std::unique_ptr<message_data>);

      protected:
        [[sc::thread_safe]] void add_function(std::string, serializer::function &&, launch);
        [[sc::thread_safe]] void add_evaluation(serializer::resolver &&, const std::string &);

      protected:
        [[sc::thread_safe]] void reject(std::uint64_t, const std::string &);
        [[sc::thread_safe]] void resolve(std::uint64_t, const std::string &);

      public:
        [[sc::thread_safe]] void clear_exposed();
        [[sc::thread_safe]] void clear_exposed(const std::string &name);
    };

    using default_serializer = serializers::glaze::serializer;

    template <Serializer Serializer = default_serializer, Module... Modules>
    class smartview : public smartview_core
    {
        std::tuple<Modules...> m_modules;

      public:
        smartview(const preferences & = {});

      public:
        template <typename Function>
        [[sc::thread_safe]] void expose(std::string name, Function &&func, launch policy = launch::sync);

      public:
        template <typename Return, typename... Params>
        [[sc::thread_safe]] [[nodiscard]] std::future<Return> evaluate(const std::string &code, Params &&...params);

      public:
        template <Module T>
        auto &module();
    };
} // namespace saucer

#include "smartview.inl"

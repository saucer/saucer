#pragma once

#include "webview.hpp"

#include "modules/module.hpp"
#include "serializers/glaze/glaze.hpp"

#include <future>
#include <atomic>
#include <memory>
#include <string>

#include <lockpp/lock.hpp>

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
        smartview_core(std::unique_ptr<serializer>, const options & = {});

      public:
        ~smartview_core() override;

      protected:
        bool on_message(const std::string &) override;

      protected:
        [[sc::thread_safe]] void call(function_data &, const serializer::function &);

      protected:
        [[sc::thread_safe]] void add_function(std::string, serializer::function &&, bool);
        [[sc::thread_safe]] void add_evaluation(serializer::resolver &&, const std::string &);

      protected:
        [[sc::thread_safe]] void reject(std::uint64_t, serializer_error);
        [[sc::thread_safe]] void resolve(std::uint64_t, const std::string &);
    };

    using default_serializer = serializers::glaze;

    template <Serializer Serializer = default_serializer, Module... Modules>
    class smartview : public smartview_core, public Modules...
    {
      public:
        smartview(const options & = {});

      public:
        template <typename Function>
        [[sc::thread_safe]] void expose(std::string name, const Function &func, bool async = false);

      public:
        template <typename Return, typename... Params>
        [[sc::thread_safe]] [[nodiscard]] std::future<Return> evaluate(const std::string &code, Params &&...params);
    };
} // namespace saucer

#include "smartview.inl"

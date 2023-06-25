#pragma once
#include "webview.hpp"
#include "serializers/serializer.hpp"

#include <future>
#include <atomic>
#include <memory>
#include <string>
#include <lockpp/lock.hpp>

namespace saucer
{
#include "meta/annotations.hpp"
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
        void on_message(const std::string &) override;

      protected:
        [[thread_safe]] void call(function_data &, const serializer::function &);

      protected:
        [[thread_safe]] void add_evaluation(serializer::promise &&, const std::string &);
        [[thread_safe]] void add_function(const std::string &, serializer::function &&, bool);

      protected:
        [[thread_safe]] void reject(std::uint64_t, const std::string &);
        [[thread_safe]] void resolve(std::uint64_t, const std::string &);
    };

    template <Serializer Serializer> class smartview : public smartview_core
    {
      public:
        smartview(const options & = {});

      public:
        template <typename Function>
        [[thread_safe]] void expose(const std::string &name, const Function &func, bool async = false);

      public:
        template <typename Return, typename... Params>
        [[thread_safe]] [[nodiscard]] std::future<Return> evaluate(const std::string &code, Params &&...params);
    };
#include "meta/annotations.hpp" //NOLINT
} // namespace saucer

#include "smartview.inl"
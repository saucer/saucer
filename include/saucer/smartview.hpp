#pragma once
#include "webview.hpp"
#include "plugin/plugin.hpp"
#include "module/module.hpp"
#include "serializers/serializer.hpp"

#include <future>
#include <typeindex>
#include <lockpp/lock.hpp>

namespace saucer
{
#include "annotations.hpp"
    class smartview : public webview
    {
        struct callback_t;
        struct eval_t;

      private:
        using resolve_callback = serializer::resolve_callback;
        using eval_callback = serializer::eval_callback;

      private:
        lockpp::lock<std::map<std::size_t, std::shared_ptr<std::future<void>>>> m_futures;

      protected:
        lockpp::lock<std::map<std::type_index, std::unique_ptr<serializer>>> m_serializers;
        lockpp::lock<std::vector<std::unique_ptr<plugin>>> m_plugins;

      protected:
        lockpp::lock<std::map<std::string, callback_t>> m_callbacks;
        lockpp::lock<std::map<std::size_t, eval_t>> m_evals;

      protected:
        std::atomic<std::size_t> m_id_counter{0};

      public:
        smartview(const webview_options & = {});

      public:
        ~smartview() override;

      protected:
        void on_message(const std::string &) override;

      protected:
        [[thread_safe]] void resolve(function_data &, const resolve_callback &);

      protected:
        [[thread_safe]] void add_eval(std::type_index, eval_callback &&, const std::string &);
        [[thread_safe]] void add_callback(std::type_index, const std::string &, resolve_callback &&, bool);

      protected:
        [[thread_safe]] void reject(std::size_t, const std::string &);
        [[thread_safe]] void resolve(std::size_t, const std::string &);

      public:
        template <typename Plugin> //
        [[thread_safe]] Plugin &add_plugin();

      public:
        template <typename Serializer, typename Function>
        [[thread_safe]] void expose(const std::string &name, const Function &func, bool async = false);

      public:
        template <typename Return, typename Serializer, typename... Params>
        [[thread_safe]] [[nodiscard]] std::future<Return> eval(const std::string &code, Params &&...params);
    };
#include "annotations.hpp" //NOLINT

    template <typename DefaultSerializer> class simple_smartview;
    template <template <backend_type> class... Modules> class modularized_smartview;
    template <typename DefaultSerializer, template <backend_type> class... Modules> class modularized_simple_smartview;
} // namespace saucer

#include "smartview.inl"